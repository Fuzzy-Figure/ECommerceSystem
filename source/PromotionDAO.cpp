#include "../header/PromotionDAO.h"

PromotionDAO::PromotionDAO(Database& db) : db_(db) {}

std::vector<PromotionDAO::Config> PromotionDAO::findEnabled() {
	std::vector<Config> result;
	auto rows = db_.query(
		"SELECT type, params FROM promotions WHERE enabled = 1 ORDER BY id ASC;"
	);
	result.reserve(rows.size());
	for (const auto& row : rows) {
		Config cfg;
		// Database::query 把所有列值以字符串返回，需稳健转换
		if (row.contains("type")) {
			const auto& v = row["type"];
			if (v.is_string()) cfg.type = v.get<std::string>();
			else if (v.is_number()) cfg.type = std::to_string(v.get<int64_t>());
		}
		if (row.contains("params")) {
			const auto& v = row["params"];
			if (v.is_string()) {
				// params 列以 TEXT 存 JSON 字符串
				try { cfg.params = nlohmann::json::parse(v.get<std::string>()); } catch (...) { cfg.params = nlohmann::json::object(); }
			}
			else if (v.is_object()) {
				cfg.params = v;
			}
			else {
				cfg.params = nlohmann::json::object();
			}
		}
		if (!cfg.type.empty()) result.push_back(std::move(cfg));
	}
	return result;
}
