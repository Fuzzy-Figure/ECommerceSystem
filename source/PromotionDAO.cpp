#include "PromotionDAO.h"

PromotionDAO::PromotionDAO(Database& db) : db_(db) {}

namespace {
	// 从 query 行稳健读取 string 字段
	std::string getStr(const nlohmann::json& row, const char* key) {
		if (!row.contains(key)) return {};
		const auto& v = row[key];
		if (v.is_string()) return v.get<std::string>();
		if (v.is_number()) return std::to_string(v.get<std::int64_t>());
		return {};
	}

	// 从 query 行稳健读取 int32
	std::int32_t getInt(const nlohmann::json& row, const char* key) {
		if (!row.contains(key)) return 0;
		const auto& v = row[key];
		if (v.is_number_integer()) return v.get<std::int32_t>();
		if (v.is_string()) { try { return static_cast<std::int32_t>(std::stoi(v.get<std::string>())); } catch (...) {} }
		return 0;
	}

	// 解析 params 列（TEXT 存 JSON 字符串）
	nlohmann::json parseParams(const nlohmann::json& row) {
		if (!row.contains("params")) return nlohmann::json::object();
		const auto& v = row["params"];
		if (v.is_string()) {
			try { return nlohmann::json::parse(v.get<std::string>()); }
			catch (...) { return nlohmann::json::object(); }
		}
		if (v.is_object()) return v;
		return nlohmann::json::object();
	}
}

std::vector<PromotionDAO::Config> PromotionDAO::findEnabled() {
	std::vector<Config> result;
	auto rows = db_.query("SELECT id, type, params, enabled FROM promotions WHERE enabled = 1 ORDER BY id ASC;");
	result.reserve(rows.size());
	for (const auto& row : rows) {
		Config cfg;
		cfg.id = getInt(row, "id");
		cfg.type = getStr(row, "type");
		cfg.params = parseParams(row);
		cfg.enabled = (getInt(row, "enabled") != 0);
		if (!cfg.type.empty()) result.push_back(std::move(cfg));
	}
	return result;
}

std::vector<PromotionDAO::Config> PromotionDAO::findAll() {
	std::vector<Config> result;
	auto rows = db_.query("SELECT id, type, params, enabled FROM promotions ORDER BY id ASC;");
	result.reserve(rows.size());
	for (const auto& row : rows) {
		Config cfg;
		cfg.id = getInt(row, "id");
		cfg.type = getStr(row, "type");
		cfg.params = parseParams(row);
		cfg.enabled = (getInt(row, "enabled") != 0);
		if (!cfg.type.empty()) result.push_back(std::move(cfg));
	}
	return result;
}

bool PromotionDAO::setEnabled(std::int32_t id, bool enabled) {
	return db_.executeStmt(
		"UPDATE promotions SET enabled = ? WHERE id = ?;",
		{ static_cast<std::int64_t>(enabled ? 1 : 0), static_cast<std::int64_t>(id) }) > 0;
}

bool PromotionDAO::updateParams(std::int32_t id, const nlohmann::json& params) {
	return db_.executeStmt(
		"UPDATE promotions SET params = ? WHERE id = ?;",
		{ params.dump(), static_cast<std::int64_t>(id) }) > 0;
}

std::int32_t PromotionDAO::create(const std::string& type, const nlohmann::json& params) {
	db_.executeStmt(
		"INSERT INTO promotions (type, params, enabled) VALUES (?, ?, 1);",
		{ type, params.dump() });
	// 取刚插入的自增 id
	auto rows = db_.query("SELECT last_insert_rowid() AS id;");
	if (rows.empty()) return 0;
	return getInt(rows.front(), "id");
}

bool PromotionDAO::remove(std::int32_t id) {
	return db_.executeStmt(
		"DELETE FROM promotions WHERE id = ?;",
		{ static_cast<std::int64_t>(id) }) > 0;
}
