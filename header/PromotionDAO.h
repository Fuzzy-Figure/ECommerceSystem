#pragma once
// 促销持久层：从 promotions 表读取启用的促销配置。
// 仅读，不写（写入由 ServerMain 种子数据或后续管理接口负责）。
#include "Database.h"
#include <string>
#include <vector>
#include <json.hpp>

class PromotionDAO {
public:
	explicit PromotionDAO(Database& db);

	// 单条促销配置
	struct Config {
		std::string        type;    // 'discount'/'tiered'/'freeitem'/'reduction'/'coupon'
		nlohmann::json     params;  // 类型相关参数，如 {"rate":0.8} 或 {"threshold":100,"reduce":20}
	};

	// 读取所有 enabled=1 的促销配置
	std::vector<Config> findEnabled();

private:
	Database& db_;
};
