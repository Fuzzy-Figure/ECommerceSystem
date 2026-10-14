#include "ReviewDAO.h"
#include <sstream>

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
	std::string escapeSql(const std::string& s) {
		std::string out;
		out.reserve(s.size());
		for (char c : s) out += (c == '\'' ? "''" : std::string(1, c));
		return out;
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
	std::ostringstream uq;
	uq << "SELECT username FROM users WHERE id=" << userId << ";";
	auto uRows = db_.query(uq.str());
	std::string username;
	if (!uRows.empty()) username = toStr(uRows.front()["username"]);

	// created_at 用 SQLite CURRENT_TIMESTAMP
	std::ostringstream ins;
	ins << "INSERT INTO reviews (order_id, product_id, user_id, username, rating, comment, created_at) VALUES ("
		<< orderId << ", "
		<< productId << ", "
		<< userId << ", '"
		<< escapeSql(username) << "', "
		<< rating << ", '"
		<< escapeSql(comment) << "', CURRENT_TIMESTAMP);";
	return db_.execute(ins.str()) > 0;
}

bool ReviewDAO::hasReviewed(std::int64_t orderId, std::int32_t productId) {
	std::ostringstream q;
	q << "SELECT 1 FROM reviews WHERE order_id=" << orderId
	  << " AND product_id=" << productId << " LIMIT 1;";
	return !db_.query(q.str()).empty();
}

std::vector<Review> ReviewDAO::findByProductId(std::int32_t productId) {
	std::ostringstream q;
	q << "SELECT id, order_id, product_id, user_id, username, rating, comment, created_at"
	  << " FROM reviews WHERE product_id=" << productId
	  << " ORDER BY id ASC;";
	auto rows = db_.query(q.str());
	std::vector<Review> result;
	result.reserve(rows.size());
	for (const auto& row : rows) result.push_back(mapRow(row));
	return result;
}

std::vector<Review> ReviewDAO::findByUserId(std::int64_t userId) {
	std::ostringstream q;
	q << "SELECT id, order_id, product_id, user_id, username, rating, comment, created_at"
	  << " FROM reviews WHERE user_id=" << userId
	  << " ORDER BY id ASC;";
	auto rows = db_.query(q.str());
	std::vector<Review> result;
	result.reserve(rows.size());
	for (const auto& row : rows) result.push_back(mapRow(row));
	return result;
}

ReviewDAO::ProductRating ReviewDAO::getProductRating(std::int32_t productId) {
	std::ostringstream q;
	q << "SELECT AVG(rating) AS avg, COUNT(*) AS cnt FROM reviews WHERE product_id="
	  << productId << ";";
	auto rows = db_.query(q.str());
	ProductRating r{0.0, 0};
	if (rows.empty()) return r;
	const auto& row = rows.front();
	r.avg   = toDouble(row["avg"]);
	r.count = toInt32(row["cnt"]);
	return r;
}
