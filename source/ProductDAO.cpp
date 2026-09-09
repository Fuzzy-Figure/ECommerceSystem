#include "../header/ProductDAO.h"
#include <sstream>
#include <stdexcept>

ProductDAO::ProductDAO(Database& db) : db_(db) {}

Product ProductDAO::mapRow(const nlohmann::json& row) {
    Product p;
    // SQLite 回调把所有列值都按字符串返回，需要按目标类型稳健转换。
    auto getInt = [&](const char* k) -> std::int32_t {
        if (!row.contains(k)) return 0;
        const auto& v = row[k];
        if (v.is_number()) return v.get<std::int32_t>();
        if (v.is_string()) {
            try { return std::stoi(v.get<std::string>()); } catch (...) { return 0; }
        }
        return 0;
    };
    auto getDouble = [&](const char* k) -> double {
        if (!row.contains(k)) return 0.0;
        const auto& v = row[k];
        if (v.is_number()) return v.get<double>();
        if (v.is_string()) {
            try { return std::stod(v.get<std::string>()); } catch (...) { return 0.0; }
        }
        return 0.0;
    };
    auto getStr = [&](const char* k) -> std::string {
        if (!row.contains(k)) return {};
        const auto& v = row[k];
        if (v.is_string()) return v.get<std::string>();
        if (v.is_number()) return v.dump();
        return {};
    };
    p.id          = getInt("id");
    p.name        = getStr("name");
    p.description = getStr("description");
    p.price       = getDouble("price");
    p.stock       = getInt("stock");
    p.imagePath   = getStr("imagePath");
    return p;
}

std::vector<Product> ProductDAO::findAll() {
    auto rows = db_.query("SELECT id, name, description, price, stock, imagePath FROM products ORDER BY id ASC;");
    std::vector<Product> result;
    result.reserve(rows.size());
    for (const auto& row : rows) result.push_back(mapRow(row));
    return result;
}

std::optional<Product> ProductDAO::findById(std::int32_t id) {
    std::ostringstream ss;
    ss << "SELECT id, name, description, price, stock, imagePath FROM products WHERE id = " << id << ";";
    auto rows = db_.query(ss.str());
    if (rows.empty()) return std::nullopt;
    return mapRow(rows.front());
}

bool ProductDAO::reduceStock(std::int32_t id, std::int32_t qty) {
    if (qty <= 0) return false;
    // 用 SQLite 的原子更新带条件，避免超扣：UPDATE products SET stock = stock - qty WHERE id = ? AND stock >= qty
    std::ostringstream ss;
    ss << "UPDATE products SET stock = stock - " << qty
       << " WHERE id = " << id << " AND stock >= " << qty << ";";
    const int affected = db_.execute(ss.str());
    return affected > 0;
}
