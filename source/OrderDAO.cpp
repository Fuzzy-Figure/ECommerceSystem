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

std::int64_t OrderDAO::placeOrder(const std::vector<std::pair<std::int32_t, std::int32_t>>& items,
                                  double& totalOut) {
    if (items.empty()) return -1;

    try {
        db_.execute("BEGIN;");

        // 1. 查每个商品的当前价格 + 库存，校验库存足够，累计总价
        double total = 0.0;
        // (productId, qty, price) 缓存，避免二次查询
        std::vector<std::tuple<std::int32_t, std::int32_t, double>> details;
        details.reserve(items.size());

        for (const auto& [pid, qty] : items) {
            if (qty <= 0) { db_.execute("ROLLBACK;"); return -1; }
            std::ostringstream q;
            q << "SELECT price, stock FROM products WHERE id=" << pid << ";";
            auto rows = db_.query(q.str());
            if (rows.empty()) { db_.execute("ROLLBACK;"); return -1; }
            const auto& row = rows.front();
            const double price = toDouble(row["price"]);
            const std::int64_t stock = toInt64(row["stock"]);
            if (stock < qty) {
                db_.execute("ROLLBACK;");
                return -1;  // 库存不足
            }
            total += price * static_cast<double>(qty);
            details.emplace_back(pid, qty, price);
        }

        // 2. 生成订单主表
        {
            std::ostringstream ins;
            ins << "INSERT INTO orders (total, created_at) VALUES ("
                << total << ", '" << nowString() << "');";
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
        for (const auto& [pid, qty, price] : details) {
            std::ostringstream ins;
            ins << "INSERT INTO order_items (order_id, product_id, qty, price) VALUES ("
                << orderId << ", " << pid << ", " << qty << ", " << price << ");";
            db_.execute(ins.str());

            std::ostringstream upd;
            upd << "UPDATE products SET stock = stock - " << qty
                << " WHERE id = " << pid << " AND stock >= " << qty << ";";
            const int affected = db_.execute(upd.str());
            if (affected == 0) {
                db_.execute("ROLLBACK;");
                return -1;
            }
        }

        db_.execute("COMMIT;");
        totalOut = total;
        return orderId;
    } catch (const std::exception&) {
        // 异常路径下尽力回滚；ROLLBACK 自身失败无能为力，吞掉
        try { db_.execute("ROLLBACK;"); } catch (...) {}
        return -1;
    }
}
