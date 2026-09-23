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

	// 查询全部【上架】商品（用户端展示用，下架商品不可见不可买）
	std::vector<Product> findAll();

	// 查询全部商品（含下架，商家管理用）
	std::vector<Product> findAllForMerchant();

	// 按 id 查询；不存在返回 nullopt
	std::optional<Product> findById(std::int32_t id);

	// 扣减库存（已售出或加购物车确认下单时调用）
	// 返回 true 表示扣减成功（库存足够）
	bool reduceStock(std::int32_t id, std::int32_t qty);

	// 设置上架/下架状态：onSale=true 上架，false 下架
	bool setOnSale(std::int32_t id, bool onSale);

	// 直接设置库存为 newStock（商家调整库存用）
	bool updateStock(std::int32_t id, std::int32_t newStock);

	// 商家新增商品：返回新商品 id（>0 表示成功）；imagePath 为空时用默认占位图
	std::int32_t createProduct(const std::string& name,
								const std::string& description,
								double             price,
								std::int32_t       stock,
								const std::string& imagePath);

	// 商家删除商品：返回 true 表示成功
	bool deleteProduct(std::int32_t id);

private:
	// 把一行 JSON 映射为 Product
	static Product mapRow(const nlohmann::json& row);

	Database& db_;
};
