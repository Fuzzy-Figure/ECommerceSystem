#pragma once
// 客户端 Model：本地数据状态
// 持有商品列表快照 + 本地购物车（A 方案胖客户端，购物车仅存内存）。
// UI 不直接依赖网络层。
#include <vector>
#include <string>
#include "Product.h"
#include "CartItem.h"

class ClientModel {
public:
	// 服务端推送商品列表后调用
	void setProducts(std::vector<Product> products);

	const std::vector<Product>& products() const noexcept { return products_; }
	// 按商品ID查找商品（用于加购时获取名称/价格）
	const Product* findProduct(std::int32_t productId) const;

	// 连接/错误提示信息（由 Controller 写入，View 渲染）
	void setStatus(std::wstring status) { status_ = std::move(status); }
	const std::wstring& status() const noexcept { return status_; }

	// === 购物车操作 ===
	// 加入购物车：若已存在同 productId 则累加 qty
	void addToCart(std::int32_t productId, std::int32_t qty = 1);
	// 移除指定 productId 的购物车项
	void removeFromCart(std::int32_t productId);
	// 清空购物车（结算成功后调用）
	void clearCart() noexcept { cart_.clear(); }
	const std::vector<CartItem>& cart() const noexcept { return cart_; }
	// 购物车总额（各项小计之和）
	double cartTotal() const;

private:
	std::vector<Product>  products_;
	std::vector<CartItem> cart_;
	std::wstring          status_;  // 当前状态/提示信息
};
