#include "PromotionDAO.h"

PromotionDAO::PromotionDAO(Database& db) : db_(db) {}

namespace {
	// 转义 SQL 字符串中的单引号，防注入
	std::string escapeSql(const std::string& s) {
		std::string out;
		out.reserve(s.size() + 8);
		for (char c : s) {
			if (c == '\'') out += "''";
			else out.push_back(c);
		}
		return out;
	}

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
	const std::string sql =
		"UPDATE promotions SET enabled = " + std::to_string(enabled ? 1 : 0) +
		" WHERE id = " + std::to_string(id) + ";";
	return db_.execute(sql) > 0;
}

bool PromotionDAO::updateParams(std::int32_t id, const nlohmann::json& params) {
	const std::string paramsStr = escapeSql(params.dump());
	const std::string sql =
		"UPDATE promotions SET params = '" + paramsStr +
		"' WHERE id = " + std::to_string(id) + ";";
	return db_.execute(sql) > 0;
}

std::int32_t PromotionDAO::create(const std::string& type, const nlohmann::json& params) {
	const std::string typeStr = escapeSql(type);
	const std::string paramsStr = escapeSql(params.dump());
	const std::string sql =
		"INSERT INTO promotions (type, params, enabled) VALUES ('"
		+ typeStr + "', '" + paramsStr + "', 1);";
	db_.execute(sql);
	// 取刚插入的自增 id
	auto rows = db_.query("SELECT last_insert_rowid() AS id;");
	if (rows.empty()) return 0;
	return getInt(rows.front(), "id");
}

bool PromotionDAO::remove(std::int32_t id) {
	const std::string sql = "DELETE FROM promotions WHERE id = " + std::to_string(id) + ";";
	return db_.execute(sql) > 0;
}
