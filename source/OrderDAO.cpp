#include "OrderDAO.h"
#include <chrono>
#include <format>
#include <sstream>
#include <stdexcept>
#include <iomanip>

OrderDAO::OrderDAO(Database& db) : db_(db) {}

namespace {
	// Database::query 把所有列值以字符串返回，需稳健转换
	double toDouble(const nlohmann::json& v) {
		if (v.is_number()) return v.get<double>();
		if (v.is_string()) { try { return std::stod(v.get<std::string>()); } catch (...) {} }
		return 0.0;
	}
	std::int64_t toInt64(const nlohmann::json& v) {
		if (v.is_number()) return v.get<std::int64_t>();
		if (v.is_string()) { try { return std::stoll(v.get<std::string>()); } catch (...) {} }
		return 0;
	}
	std::int32_t toInt32(const nlohmann::json& v) {
		return static_cast<std::int32_t>(toInt64(v));
	}
	std::string toStr(const nlohmann::json& v) {
		if (v.is_string()) return v.get<std::string>();
		if (v.is_number()) return std::to_string(v.get<double>());
		return {};
	}

	// 当前本地时间字符串，用于订单 created_at
	std::string nowString() {
		using namespace std::chrono;
		const auto now = system_clock::now();
		const auto t = system_clock::to_time_t(now);
		std::tm tm{};
#ifdef _MSC_VER
		localtime_s(&tm, &t);
#else
		tm = *std::localtime(&t);
#endif
		std::ostringstream ss;
		ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}
}

std::int64_t OrderDAO::placeOrder(const std::vector<CartItem>& items,
								  std::int64_t                  userId,
								  double                       discount,
								  double& originalTotalOut,
								  double& finalTotalOut,
								  std::string& failReasonOut) {
	if (items.empty()) return -1;

	try {
		db_.execute("BEGIN;");

		// 1. 累计原价，并校验每个商品的库存足够
		double originalTotal = 0.0;
		for (const auto& c : items) {
			if (c.qty <= 0) {
				db_.execute("ROLLBACK;");
				failReasonOut = std::format("商品「{}」数量非法", c.name);
				return -1;
			}
			auto rows = db_.queryStmt(
				"SELECT stock FROM products WHERE id=?;",
				{ static_cast<std::int64_t>(c.productId) });
			if (rows.empty()) {
				db_.execute("ROLLBACK;");
				failReasonOut = std::format("商品「{}」不存在或已下架", c.name);
				return -1;
			}
			const std::int64_t stock = toInt64(rows.front()["stock"]);
			if (stock < c.qty) {
				db_.execute("ROLLBACK;");
				failReasonOut = std::format("商品「{}」库存不足：需要 {} 件，仅剩 {} 件", c.name, c.qty, stock);
				return -1;  // 库存不足
			}
			originalTotal += c.subtotal();
		}
		// 防御：折扣不能让金额变负
		if (discount < 0.0) discount = 0.0;
		if (discount > originalTotal) discount = originalTotal;
		const double finalTotal = originalTotal - discount;

		// 2. 生成订单主表（原价 + 折扣 + 实付 + 状态0=正常 + user_id）
		db_.executeStmt(
			"INSERT INTO orders (user_id, total, discount, final_total, status, created_at) VALUES (?, ?, ?, ?, 0, ?);",
			{ userId, originalTotal, discount, finalTotal, nowString() });
		// 3. 拿订单自增 ID
		std::int64_t orderId = 0;
		{
			auto idRows = db_.query("SELECT last_insert_rowid() AS id;");
			if (!idRows.empty()) orderId = toInt64(idRows.front()["id"]);
		}
		if (orderId <= 0) { db_.execute("ROLLBACK;"); return -1; }

		// 4. 写明细 + 条件扣库存（受影响行数=0 说明并发下被抢光，回滚整单）
		for (const auto& c : items) {
			db_.executeStmt(
				"INSERT INTO order_items (order_id, product_id, qty, price, returned_qty) VALUES (?, ?, ?, ?, 0);",
				{ orderId, static_cast<std::int64_t>(c.productId), static_cast<std::int64_t>(c.qty), c.price });

			const int affected = db_.executeStmt(
				"UPDATE products SET stock = stock - ? WHERE id = ? AND stock >= ?;",
				{ static_cast<std::int64_t>(c.qty), static_cast<std::int64_t>(c.productId), static_cast<std::int64_t>(c.qty) });
			if (affected == 0) {
				db_.execute("ROLLBACK;");
				failReasonOut = std::format("商品「{}」库存不足：需要 {} 件，已被其他订单抢购", c.name, c.qty);
				return -1;
			}
		}

		db_.execute("COMMIT;");
		originalTotalOut = originalTotal;
		finalTotalOut = finalTotal;
		return orderId;
	} catch (const std::exception&) {
		try { db_.execute("ROLLBACK;"); } catch (...) {}
		return -1;
	}
}

void OrderDAO::loadItemsOf(Order& order) {
	auto rows = db_.queryStmt(
		"SELECT product_id, qty, price, returned_qty FROM order_items WHERE order_id=? ORDER BY id ASC;",
		{ order.id });
	order.items.clear();
	order.items.reserve(rows.size());
	for (const auto& r : rows) {
		OrderItem it;
		it.productId = toInt32(r["product_id"]);
		it.qty = toInt32(r["qty"]);
		it.price = toDouble(r["price"]);
		it.returnedQty = toInt32(r["returned_qty"]);
		// 查商品名（若商品已删则留空）
		auto prows = db_.queryStmt(
			"SELECT name FROM products WHERE id=?;",
			{ static_cast<std::int64_t>(it.productId) });
		if (!prows.empty()) it.name = toStr(prows.front()["name"]);
		order.items.push_back(std::move(it));
	}
}

std::vector<Order> OrderDAO::findAllWithItems(std::int64_t userId) {
	// 查订单按 id 倒序（最新在前）；userId > 0 只查该用户订单
	std::vector<nlohmann::json> rows;
	if (userId > 0) {
		rows = db_.queryStmt(
			"SELECT id, total, discount, final_total, status, ship_status, created_at FROM orders WHERE user_id=? ORDER BY id DESC;",
			{ userId });
	}
	else {
		rows = db_.query(
			"SELECT id, total, discount, final_total, status, ship_status, created_at FROM orders ORDER BY id DESC;");
	}
	std::vector<Order> result;
	result.reserve(rows.size());
	for (const auto& r : rows) {
		Order o;
		o.id = toInt64(r["id"]);
		o.originalTotal = toDouble(r["total"]);
		o.discount = toDouble(r["discount"]);
		o.finalTotal = toDouble(r["final_total"]);
		o.status = toInt32(r["status"]);
		o.shipStatus = toInt32(r["ship_status"]);
		o.createdAt = toStr(r["created_at"]);
		loadItemsOf(o);
		result.push_back(std::move(o));
	}
	return result;
}

std::vector<Order> OrderDAO::findAllWithItemsForMerchant() {
	// 商家看全部订单 + 联查 users 拿下单用户名
	std::ostringstream q;
	q << "SELECT o.id, o.total, o.discount, o.final_total, o.status, o.ship_status, o.created_at, "
		"COALESCE(u.username, '未知用户') AS username "
		"FROM orders o LEFT JOIN users u ON o.user_id = u.id ORDER BY o.id DESC;";
	auto rows = db_.query(q.str());
	std::vector<Order> result;
	result.reserve(rows.size());
	for (const auto& r : rows) {
		Order o;
		o.id = toInt64(r["id"]);
		o.originalTotal = toDouble(r["total"]);
		o.discount = toDouble(r["discount"]);
		o.finalTotal = toDouble(r["final_total"]);
		o.status = toInt32(r["status"]);
		o.shipStatus = toInt32(r["ship_status"]);
		o.createdAt = toStr(r["created_at"]);
		o.username = toStr(r["username"]);
		loadItemsOf(o);
		result.push_back(std::move(o));
	}
	return result;
}

bool OrderDAO::updateShipStatus(std::int64_t orderId, int newStatus) {
	if (orderId <= 0 || newStatus < 0 || newStatus > 2) return false;
	return db_.executeStmt(
		"UPDATE orders SET ship_status=? WHERE id=?;",
		{ static_cast<std::int64_t>(newStatus), orderId }) > 0;
}

bool OrderDAO::findOrderShipStatus(std::int64_t orderId, int& statusOut, std::int64_t& userIdOut) {
	if (orderId <= 0) return false;
	auto rows = db_.queryStmt(
		"SELECT ship_status, user_id FROM orders WHERE id=?;",
		{ orderId });
	if (rows.empty()) return false;
	statusOut = toInt32(rows.front()["ship_status"]);
	userIdOut = toInt64(rows.front()["user_id"]);
	return true;
}

bool OrderDAO::placeReturn(std::int64_t       orderId,
						   std::int32_t       productId,
						   std::int32_t       returnQty,
						   double& refundOut) {
	if (orderId <= 0 || productId <= 0 || returnQty <= 0) return false;

	try {
		db_.execute("BEGIN;");

		// 1. 查订单 + 该商品明细，校验状态/数量
		auto orows = db_.queryStmt(
			"SELECT total, discount, final_total, status FROM orders WHERE id=?;",
			{ orderId });
		if (orows.empty()) { db_.execute("ROLLBACK;"); return false; }
		const auto& orow = orows.front();
		const double originalTotal = toDouble(orow["total"]);
		const double finalTotal = toDouble(orow["final_total"]);
		const int    status = toInt32(orow["status"]);
		if (status == 2) { db_.execute("ROLLBACK;"); return false; }  // 已全部退货

		auto irows = db_.queryStmt(
			"SELECT qty, price, returned_qty FROM order_items WHERE order_id=? AND product_id=?;",
			{ orderId, static_cast<std::int64_t>(productId) });
		if (irows.empty()) { db_.execute("ROLLBACK;"); return false; }
		const auto& irow = irows.front();
		const std::int32_t qty = toInt32(irow["qty"]);
		const double       unitPrice = toDouble(irow["price"]);
		const std::int32_t returnedQty = toInt32(irow["returned_qty"]);
		const std::int32_t returnable = qty - returnedQty;
		if (returnQty > returnable) { db_.execute("ROLLBACK;"); return false; }

		// 2. 计算退款（Y 方案：按 finalTotal/originalTotal 比例分摊）
		double ratio = (originalTotal > 0.0) ? (finalTotal / originalTotal) : 1.0;
		double refund = unitPrice * static_cast<double>(returnQty) * ratio;
		if (refund < 0.0) refund = 0.0;

		// 3. 更新明细：累加 returned_qty；若已退完则该明细视为"全退"
		const std::int32_t newReturned = returnedQty + returnQty;
		db_.executeStmt(
			"UPDATE order_items SET returned_qty=? WHERE order_id=? AND product_id=?;",
			{ static_cast<std::int64_t>(newReturned), orderId, static_cast<std::int64_t>(productId) });
		// 4. 回库存
		db_.executeStmt(
			"UPDATE products SET stock = stock + ? WHERE id=?;",
			{ static_cast<std::int64_t>(returnQty), static_cast<std::int64_t>(productId) });
		// 5. 更新订单实付金额 + 状态
		const double newFinal = finalTotal - refund;
		int newStatus = status;
		if (newFinal <= 0.0) newStatus = 2;            // 全退
		else if (newReturned >= qty) newStatus = 1;     // 该项全退（未必全单全退）
		// 检查是否全部明细都全退
		{
			auto crows = db_.queryStmt(
				"SELECT COUNT(*) AS cnt FROM order_items WHERE order_id=? AND returned_qty < qty;",
				{ orderId });
			if (!crows.empty()) {
				const int remain = toInt32(crows.front()["cnt"]);
				if (remain == 0) newStatus = 2;  // 全部明细都退完
			}
		}
		db_.executeStmt(
			"UPDATE orders SET final_total=?, status=? WHERE id=?;",
			{ newFinal, static_cast<std::int64_t>(newStatus), orderId });

		db_.execute("COMMIT;");
		refundOut = refund;
		return true;
	} catch (const std::exception&) {
		try { db_.execute("ROLLBACK;"); } catch (...) {}
		return false;
	}
}

MerchantStats OrderDAO::getMerchantStats(int rangeType) {
	MerchantStats s;
	// 当前范围 WHERE 子句
	std::string rangeFilter;
	switch (rangeType) {
		case 1:  rangeFilter = " AND strftime('%Y-%W', created_at)=strftime('%Y-%W','now','localtime')"; break;
		case 2:  rangeFilter = " AND strftime('%Y-%m', created_at)=strftime('%Y-%m','now','localtime')"; break;
		case 3:  rangeFilter = ""; break; // 全部
		default: rangeFilter = " AND date(created_at)=date('now','localtime')"; break; // 今日
	}
	// 当前范围统计
	{
		std::ostringstream q;
		q << "SELECT COUNT(*) AS c, COALESCE(SUM(final_total),0) AS r "
			"FROM orders WHERE status<2" << rangeFilter << ";";
		auto rows = db_.query(q.str());
		if (!rows.empty()) {
			s.todayOrders  = toInt32(rows[0]["c"]);
			s.todayRevenue = toDouble(rows[0]["r"]);
		}
	}
	// 总计（始终全部）
	auto totalRows = db_.query(
		"SELECT COUNT(*) AS c, COALESCE(SUM(final_total),0) AS r "
		"FROM orders WHERE status<2;");
	if (!totalRows.empty()) {
		s.totalOrders  = toInt32(totalRows[0]["c"]);
		s.totalRevenue = toDouble(totalRows[0]["r"]);
	}
	return s;
}

std::vector<TopProduct> OrderDAO::getTopProducts(int limit, int rangeType) {
	std::vector<TopProduct> result;
	// 当前范围 WHERE 子句（用于 JOIN orders 拿 created_at）
	std::string rangeFilter;
	switch (rangeType) {
		case 1:  rangeFilter = " AND strftime('%Y-%W', o.created_at)=strftime('%Y-%W','now','localtime')"; break;
		case 2:  rangeFilter = " AND strftime('%Y-%m', o.created_at)=strftime('%Y-%m','now','localtime')"; break;
		case 3:  rangeFilter = ""; break;
		default: rangeFilter = " AND date(o.created_at)=date('now','localtime')"; break;
	}
	// rangeFilter 是代码常量片段（无用户输入），允许拼接；limit 走绑定参数
	std::string sql =
		"SELECT p.id AS pid, p.name AS pname, "
		"SUM(oi.qty - oi.returned_qty) AS sold, "
		"SUM((oi.qty - oi.returned_qty) * oi.price) AS rev "
		"FROM order_items oi JOIN products p ON oi.product_id = p.id "
		"JOIN orders o ON oi.order_id = o.id "
		"WHERE o.status<2" + rangeFilter + " "
		"GROUP BY p.id HAVING sold > 0 "
		"ORDER BY rev DESC";
	std::vector<nlohmann::json> rows;
	if (limit > 0) {
		sql += " LIMIT ?;";
		rows = db_.queryStmt(sql, { static_cast<std::int64_t>(limit) });
	}
	else {
		sql += ";";
		rows = db_.query(sql);
	}
	result.reserve(rows.size());
	for (const auto& r : rows) {
		TopProduct t;
		t.productId = toInt32(r["pid"]);
		t.name      = toStr(r["pname"]);
		t.qtySold   = toInt32(r["sold"]);
		t.revenue   = toDouble(r["rev"]);
		result.push_back(std::move(t));
	}
	return result;
}
