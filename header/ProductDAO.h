#pragma once
// 持久层 DAO：商品表的数据访问对象
// 完成 PPTX 第9页"持久层映射"职责；将关系行映射为 Product 对象。
#include "Database.h"
#include "Product.h"
#include <optional>
#include <vector>

class ProductDAO {
public:
	explicit ProductDAO(Database& db);

	// 查询全部商品（用于客户端商品展示）
	std::vector<Product> findAll();

	// 按 id 查询；不存在返回 nullopt
	std::optional<Product> findById(std::int32_t id);

	// 扣减库存（已售出或加购物车确认下单时调用）
	// 返回 true 表示扣减成功（库存足够）
	bool reduceStock(std::int32_t id, std::int32_t qty);

private:
	// 把一行 JSON 映射为 Product
	static Product mapRow(const nlohmann::json& row);

	Database& db_;
};
