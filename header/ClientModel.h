#pragma once
// 客户端 Model：本地数据状态
// 持有商品列表快照 + 本地购物车（A 方案胖客户端，购物车仅存内存）+ 历史订单快照
// + 当前登录用户身份（userId/username/loggedIn）。
// UI 不直接依赖网络层。
#include <cstdint>
#include <vector>
#include <string>
#include <json.hpp>
#include "Product.h"
#include "CartItem.h"
#include "Order.h"

// 促销规则（客户端展示用）
struct Promotion {
	std::int32_t  id{ 0 };
	std::string   type;       // reduction/discount/tiered/freeitem/coupon
	nlohmann::json params;
	bool          enabled{ true };
};

// 用户列表项（管理员查看用）
struct SimpleUser {
	std::int64_t  id{ 0 };
	std::string   username;
	std::int32_t  role{ 0 };  // 0=普通用户，1=商家
};

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

	// === 历史订单 ===
	void setOrders(std::vector<Order> orders) { orders_ = std::move(orders); }
	const std::vector<Order>& orders() const noexcept { return orders_; }
	// 清空订单缓存（登出时调用，防止下个用户看到上一个用户的订单）
	void clearOrders() noexcept { orders_.clear(); }

	// === 商家促销规则 ===
	void setPromotions(std::vector<Promotion> promotions) { promotions_ = std::move(promotions); }
	const std::vector<Promotion>& promotions() const noexcept { return promotions_; }
	void clearPromotions() noexcept { promotions_.clear(); }

	// === 管理员用户列表 ===
	void setUsers(std::vector<SimpleUser> users) { users_ = std::move(users); }
	const std::vector<SimpleUser>& users() const noexcept { return users_; }
	void clearUsers() noexcept { users_.clear(); }

	// === 当前登录用户身份（业务状态，非 UI 状态）===
	std::int64_t         currentUserId() const noexcept { return currentUserId_; }
	const std::string& currentUsername() const noexcept { return currentUsername_; }
	bool                 loggedIn() const noexcept { return loggedIn_; }
	std::int32_t         currentRole() const noexcept { return currentRole_; }
	bool                 isMerchant() const noexcept { return currentRole_ == 1; }
	// 登录/注册成功后调用，写入服务端返回的 id + username + role
	void setUser(std::int64_t id, std::string username, std::int32_t role) {
		currentUserId_ = id;
		currentUsername_ = std::move(username);
		currentRole_ = role;
		loggedIn_ = true;
	}
	// 登出
	void clearUser() noexcept {
		currentUserId_ = 0;
		currentUsername_.clear();
		currentRole_ = 0;
		loggedIn_ = false;
	}

private:
	std::vector<Product>   products_;
	std::vector<CartItem>  cart_;
	std::vector<Order>     orders_;
	std::vector<Promotion> promotions_;
	std::vector<SimpleUser> users_;
	std::wstring           status_;  // 当前状态/提示信息

	std::int64_t          currentUserId_{ 0 };
	std::string           currentUsername_;
	std::int32_t          currentRole_{ 0 };  // 0=普通用户，1=商家
	bool                  loggedIn_{ false };
};
