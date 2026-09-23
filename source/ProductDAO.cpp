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
	p.id = getInt("id");
	p.name = getStr("name");
	p.description = getStr("description");
	p.price = getDouble("price");
	p.stock = getInt("stock");
	p.imagePath = getStr("imagePath");
	p.onSale = getInt("on_sale") != 0;
	return p;
}

std::vector<Product> ProductDAO::findAll() {
	auto rows = db_.query("SELECT id, name, description, price, stock, imagePath, on_sale FROM products WHERE on_sale = 1 ORDER BY id ASC;");
	std::vector<Product> result;
	result.reserve(rows.size());
	for (const auto& row : rows) result.push_back(mapRow(row));
	return result;
}

std::vector<Product> ProductDAO::findAllForMerchant() {
	auto rows = db_.query("SELECT id, name, description, price, stock, imagePath, on_sale FROM products ORDER BY id ASC;");
	std::vector<Product> result;
	result.reserve(rows.size());
	for (const auto& row : rows) result.push_back(mapRow(row));
	return result;
}

std::optional<Product> ProductDAO::findById(std::int32_t id) {
	std::ostringstream ss;
	ss << "SELECT id, name, description, price, stock, imagePath, on_sale FROM products WHERE id = " << id << ";";
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

bool ProductDAO::setOnSale(std::int32_t id, bool onSale) {
	std::ostringstream ss;
	ss << "UPDATE products SET on_sale = " << (onSale ? 1 : 0)
		<< " WHERE id = " << id << ";";
	return db_.execute(ss.str()) > 0;
}

bool ProductDAO::updateStock(std::int32_t id, std::int32_t newStock) {
	if (newStock < 0) return false;
	std::ostringstream ss;
	ss << "UPDATE products SET stock = " << newStock
		<< " WHERE id = " << id << ";";
	return db_.execute(ss.str()) > 0;
}

// 字符串转义：单引号 → ''
static std::string escapeSql(const std::string& s) {
	std::string out;
	out.reserve(s.size());
	for (char c : s) out += (c == '\'' ? "''" : std::string(1, c));
	return out;
}

std::int32_t ProductDAO::createProduct(const std::string& name,
										const std::string& description,
										double             price,
										std::int32_t       stock,
										const std::string& imagePath) {
	if (name.empty() || price < 0 || stock < 0) return 0;
	const std::string img = imagePath.empty() ? "images/placeholder.png" : imagePath;
	std::ostringstream ss;
	ss << "INSERT INTO products (name, description, price, stock, imagePath, on_sale) VALUES ('"
		<< escapeSql(name) << "', '"
		<< escapeSql(description) << "', "
		<< price << ", "
		<< stock << ", '"
		<< escapeSql(img) << "', 1);";
	if (db_.execute(ss.str()) <= 0) return 0;
	auto idRows = db_.query("SELECT last_insert_rowid() AS id;");
	if (idRows.empty()) return 0;
	const auto& v = idRows.front()["id"];
	std::int32_t newId = 0;
	if (v.is_number()) newId = v.get<std::int32_t>();
	else if (v.is_string()) { try { newId = std::stoi(v.get<std::string>()); } catch (...) {} }
	return newId;
}

bool ProductDAO::deleteProduct(std::int32_t id) {
	std::ostringstream ss;
	ss << "DELETE FROM products WHERE id = " << id << ";";
	return db_.execute(ss.str()) > 0;
}

bool ProductDAO::updateProduct(std::int32_t id, const std::string& name, const std::string& description,
								double price, std::int32_t stock, const std::string& imagePath) {
	if (id <= 0 || name.empty() || price < 0 || stock < 0) return false;
	const std::string img = imagePath.empty() ? "images/placeholder.png" : imagePath;
	std::ostringstream ss;
	ss << "UPDATE products SET name='" << escapeSql(name)
		<< "', description='" << escapeSql(description)
		<< "', price=" << price
		<< ", stock=" << stock
		<< ", imagePath='" << escapeSql(img)
		<< "' WHERE id=" << id << ";";
	return db_.execute(ss.str()) > 0;
}
