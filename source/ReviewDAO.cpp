#include "ReviewDAO.h"

ReviewDAO::ReviewDAO(Database& db) : db_(db) {}

namespace {
	std::int64_t toInt64(const nlohmann::json& v) {
		if (v.is_number()) return v.get<std::int64_t>();
		if (v.is_string()) { try { return std::stoll(v.get<std::string>()); } catch (...) {} }
		return 0;
	}
	int toInt32(const nlohmann::json& v) {
		if (v.is_number()) return v.get<int>();
		if (v.is_string()) { try { return std::stoi(v.get<std::string>()); } catch (...) {} }
		return 0;
	}
	double toDouble(const nlohmann::json& v) {
		if (v.is_number()) return v.get<double>();
		if (v.is_string()) { try { return std::stod(v.get<std::string>()); } catch (...) {} }
		return 0.0;
	}
	std::string toStr(const nlohmann::json& v) {
		if (v.is_string()) return v.get<std::string>();
		if (v.is_number()) return v.dump();
		return {};
	}
}

Review ReviewDAO::mapRow(const nlohmann::json& row) {
	Review r;
	r.id        = toInt64(row["id"]);
	r.orderId   = toInt64(row["order_id"]);
	r.productId = toInt32(row["product_id"]);
	r.userId    = toInt64(row["user_id"]);
	r.username  = toStr(row["username"]);
	r.rating    = toInt32(row["rating"]);
	r.comment   = toStr(row["comment"]);
	r.createdAt = toStr(row["created_at"]);
	return r;
}

bool ReviewDAO::createReview(std::int64_t orderId, std::int32_t productId,
                             std::int64_t userId, int rating, const std::string& comment) {
	if (rating < 1 || rating > 5) return false;
	if (hasReviewed(orderId, productId)) return false;

	// 取评价者用户名（展示用）
	std::string username;
	auto uRows = db_.queryStmt(
		"SELECT username FROM users WHERE id=?;",
		{ userId });
	if (!uRows.empty()) username = toStr(uRows.front()["username"]);

	// created_at 用 SQLite CURRENT_TIMESTAMP
	return db_.executeStmt(
		"INSERT INTO reviews (order_id, product_id, user_id, username, rating, comment, created_at) "
		"VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP);",
		{ orderId, static_cast<std::int64_t>(productId), userId,
		  username, static_cast<std::int64_t>(rating), comment }) > 0;
}

bool ReviewDAO::hasReviewed(std::int64_t orderId, std::int32_t productId) {
	return !db_.queryStmt(
		"SELECT 1 FROM reviews WHERE order_id=? AND product_id=? LIMIT 1;",
		{ orderId, static_cast<std::int64_t>(productId) }).empty();
}

std::vector<Review> ReviewDAO::findByProductId(std::int32_t productId) {
	auto rows = db_.queryStmt(
		"SELECT id, order_id, product_id, user_id, username, rating, comment, created_at"
		" FROM reviews WHERE product_id=? ORDER BY id ASC;",
		{ static_cast<std::int64_t>(productId) });
	std::vector<Review> result;
	result.reserve(rows.size());
	for (const auto& row : rows) result.push_back(mapRow(row));
	return result;
}

std::vector<Review> ReviewDAO::findByUserId(std::int64_t userId) {
	auto rows = db_.queryStmt(
		"SELECT id, order_id, product_id, user_id, username, rating, comment, created_at"
		" FROM reviews WHERE user_id=? ORDER BY id ASC;",
		{ userId });
	std::vector<Review> result;
	result.reserve(rows.size());
	for (const auto& row : rows) result.push_back(mapRow(row));
	return result;
}

ReviewDAO::ProductRating ReviewDAO::getProductRating(std::int32_t productId) {
	auto rows = db_.queryStmt(
		"SELECT AVG(rating) AS avg, COUNT(*) AS cnt FROM reviews WHERE product_id=?;",
		{ static_cast<std::int64_t>(productId) });
	ProductRating r{0.0, 0};
	if (rows.empty()) return r;
	const auto& row = rows.front();
	r.avg   = toDouble(row["avg"]);
	r.count = toInt32(row["cnt"]);
	return r;
}
