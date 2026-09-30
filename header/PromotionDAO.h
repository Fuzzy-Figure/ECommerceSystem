#pragma once
// 促销持久层：从 promotions 表读写促销配置。
// 商家可通过管理接口增删改查促销规则；写操作后服务端需调用 reloadPromotions 重组装饰器链。
#include "Database.h"
#include <string>
#include <vector>
#include <json.hpp>

class PromotionDAO {
public:
	explicit PromotionDAO(Database& db);

	// 单条促销配置
	struct Config {
		std::int32_t     id{ 0 };        // promotions.id
		std::string      type;           // 'discount'/'tiered'/'freeitem'/'reduction'/'coupon'
		nlohmann::json   params;         // 类型相关参数，如 {"rate":0.8} 或 {"threshold":100,"reduce":20}
		bool             enabled{ true };// 是否启用
	};

	// 读取所有 enabled=1 的促销配置（结算时用）
	std::vector<Config> findEnabled();

	// 读取全部促销配置（商家管理用，含禁用项）
	std::vector<Config> findAll();

	// 启用/禁用指定促销
	bool setEnabled(std::int32_t id, bool enabled);

	// 更新促销参数（params 为 JSON 对象）
	bool updateParams(std::int32_t id, const nlohmann::json& params);

	// 新增促销，返回新 id（失败返回 0）
	std::int32_t create(const std::string& type, const nlohmann::json& params);

	// 删除促销
	bool remove(std::int32_t id);

private:
	Database& db_;
};
