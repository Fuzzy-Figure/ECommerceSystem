#include "../header/OrderDAO.h"
#include <chrono>
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
        const auto t  = system_clock::to_time_t(now);
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
                                  double                       discount,
                                  double&                     originalTotalOut,
                                  double&                     finalTotalOut) {
    if (items.empty()) return -1;

    try {
        db_.execute("BEGIN;");

        // 1. 累计原价，并校验每个商品的库存足够
        double originalTotal = 0.0;
        for (const auto& c : items) {
            if (c.qty <= 0) { db_.execute("ROLLBACK;"); return -1; }
            std::ostringstream q;
            q << "SELECT stock FROM products WHERE id=" << c.productId << ";";
            auto rows = db_.query(q.str());
            if (rows.empty()) { db_.execute("ROLLBACK;"); return -1; }
            const std::int64_t stock = toInt64(rows.front()["stock"]);
            if (stock < c.qty) {
                db_.execute("ROLLBACK;");
                return -1;  // 库存不足
            }
            originalTotal += c.subtotal();
        }
        // 防御：折扣不能让金额变负
        if (discount < 0.0) discount = 0.0;
        if (discount > originalTotal) discount = originalTotal;
        const double finalTotal = originalTotal - discount;

        // 2. 生成订单主表（原价 + 折扣 + 实付 + 状态0=正常）
        {
            std::ostringstream ins;
            ins << "INSERT INTO orders (total, discount, final_total, status, created_at) VALUES ("
                << originalTotal << ", " << discount << ", " << finalTotal
                << ", 0, '" << nowString() << "');";
            db_.execute(ins.str());
        }
        // 3. 拿订单自增 ID
        std::int64_t orderId = 0;
        {
            auto idRows = db_.query("SELECT last_insert_rowid() AS id;");
            if (!idRows.empty()) orderId = toInt64(idRows.front()["id"]);
        }
        if (orderId <= 0) { db_.execute("ROLLBACK;"); return -1; }

        // 4. 写明细 + 条件扣库存（受影响行数=0 说明并发下被抢光，回滚整单）
        for (const auto& c : items) {
            std::ostringstream ins;
            ins << "INSERT INTO order_items (order_id, product_id, qty, price, returned_qty) VALUES ("
                << orderId << ", " << c.productId << ", " << c.qty << ", " << c.price << ", 0);";
            db_.execute(ins.str());

            std::ostringstream upd;
            upd << "UPDATE products SET stock = stock - " << c.qty
                << " WHERE id = " << c.productId << " AND stock >= " << c.qty << ";";
            const int affected = db_.execute(upd.str());
            if (affected == 0) {
                db_.execute("ROLLBACK;");
                return -1;
            }
        }

        db_.execute("COMMIT;");
        originalTotalOut = originalTotal;
        finalTotalOut   = finalTotal;
        return orderId;
    } catch (const std::exception&) {
        try { db_.execute("ROLLBACK;"); } catch (...) {}
        return -1;
    }
}

void OrderDAO::loadItemsOf(Order& order) {
    std::ostringstream q;
    q << "SELECT product_id, qty, price, returned_qty FROM order_items WHERE order_id="
      << order.id << " ORDER BY id ASC;";
    auto rows = db_.query(q.str());
    order.items.clear();
    order.items.reserve(rows.size());
    for (const auto& r : rows) {
        OrderItem it;
        it.productId   = toInt32(r["product_id"]);
        it.qty         = toInt32(r["qty"]);
        it.price       = toDouble(r["price"]);
        it.returnedQty = toInt32(r["returned_qty"]);
        // 查商品名（若商品已删则留空）
        std::ostringstream pq;
        pq << "SELECT name FROM products WHERE id=" << it.productId << ";";
        auto prows = db_.query(pq.str());
        if (!prows.empty()) it.name = toStr(prows.front()["name"]);
        order.items.push_back(std::move(it));
    }
}

std::vector<Order> OrderDAO::findAllWithItems() {
    // 查所有订单，按 id 倒序（最新在前）
    auto rows = db_.query(
        "SELECT id, total, discount, final_total, status, created_at "
        "FROM orders ORDER BY id DESC;"
    );
    std::vector<Order> result;
    result.reserve(rows.size());
    for (const auto& r : rows) {
        Order o;
        o.id            = toInt64(r["id"]);
        o.originalTotal = toDouble(r["total"]);
        o.discount      = toDouble(r["discount"]);
        o.finalTotal    = toDouble(r["final_total"]);
        o.status        = toInt32(r["status"]);
        o.createdAt     = toStr(r["created_at"]);
        loadItemsOf(o);
        result.push_back(std::move(o));
    }
    return result;
}

bool OrderDAO::placeReturn(std::int64_t       orderId,
                           std::int32_t       productId,
                           std::int32_t       returnQty,
                           double&            refundOut) {
    if (orderId <= 0 || productId <= 0 || returnQty <= 0) return false;

    try {
        db_.execute("BEGIN;");

        // 1. 查订单 + 该商品明细，校验状态/数量
        std::ostringstream oq;
        oq << "SELECT total, discount, final_total, status FROM orders WHERE id="
           << orderId << ";";
        auto orows = db_.query(oq.str());
        if (orows.empty()) { db_.execute("ROLLBACK;"); return false; }
        const auto& orow = orows.front();
        const double originalTotal = toDouble(orow["total"]);
        const double finalTotal   = toDouble(orow["final_total"]);
        const int    status       = toInt32(orow["status"]);
        if (status == 2) { db_.execute("ROLLBACK;"); return false; }  // 已全部退货

        std::ostringstream iq;
        iq << "SELECT qty, price, returned_qty FROM order_items "
              "WHERE order_id=" << orderId << " AND product_id=" << productId << ";";
        auto irows = db_.query(iq.str());
        if (irows.empty()) { db_.execute("ROLLBACK;"); return false; }
        const auto& irow = irows.front();
        const std::int32_t qty         = toInt32(irow["qty"]);
        const double       unitPrice   = toDouble(irow["price"]);
        const std::int32_t returnedQty = toInt32(irow["returned_qty"]);
        const std::int32_t returnable  = qty - returnedQty;
        if (returnQty > returnable) { db_.execute("ROLLBACK;"); return false; }

        // 2. 计算退款（Y 方案：按 finalTotal/originalTotal 比例分摊）
        double ratio = (originalTotal > 0.0) ? (finalTotal / originalTotal) : 1.0;
        double refund = unitPrice * static_cast<double>(returnQty) * ratio;
        if (refund < 0.0) refund = 0.0;

        // 3. 更新明细：累加 returned_qty；若已退完则该明细视为"全退"
        const std::int32_t newReturned = returnedQty + returnQty;
        {
            std::ostringstream upd;
            upd << "UPDATE order_items SET returned_qty=" << newReturned
                << " WHERE order_id=" << orderId
                << " AND product_id=" << productId << ";";
            db_.execute(upd.str());
        }
        // 4. 回库存
        {
            std::ostringstream upd;
            upd << "UPDATE products SET stock = stock + " << returnQty
                << " WHERE id=" << productId << ";";
            db_.execute(upd.str());
        }
        // 5. 更新订单实付金额 + 状态
        const double newFinal  = finalTotal - refund;
        int newStatus = status;
        if (newFinal <= 0.0) newStatus = 2;            // 全退
        else if (newReturned >= qty) newStatus = 1;     // 该项全退（未必全单全退）
        // 检查是否全部明细都全退
        {
            std::ostringstream cq;
            cq << "SELECT COUNT(*) AS cnt FROM order_items "
                  "WHERE order_id=" << orderId << " AND returned_qty < qty;";
            auto crows = db_.query(cq.str());
            if (!crows.empty()) {
                const int remain = toInt32(crows.front()["cnt"]);
                if (remain == 0) newStatus = 2;  // 全部明细都退完
            }
        }
        {
            std::ostringstream upd;
            upd << "UPDATE orders SET final_total=" << newFinal
                << ", status=" << newStatus
                << " WHERE id=" << orderId << ";";
            db_.execute(upd.str());
        }

        db_.execute("COMMIT;");
        refundOut = refund;
        return true;
    } catch (const std::exception&) {
        try { db_.execute("ROLLBACK;"); } catch (...) {}
        return false;
    }
}
