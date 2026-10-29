#pragma once
// 客户端 Model：本地数据状态
// 持有商品列表快照 + 本地购物车（A 方案胖客户端，购物车仅存内存）+ 历史订单快照
// + 当前登录用户身份（userId/username/loggedIn）。
// UI 不直接依赖网络层。
#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <json.hpp>
#include "Product.h"
#include "User.h"
#include "CartItem.h"
#include "Order.h"
#include "Review.h"

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
	User::Role    role{ User::Role::consumer };
};

// 商家销售统计
struct MerchantStats {
	int    rangeOrders{0};   // 当前范围订单数
	double rangeRevenue{0.0};// 当前范围销售额
	int    totalOrders{0};
	double totalRevenue{0.0};
	std::string rangeLabel;  // "今日"/"本周"/"本月"/"全部"
};

// 购物车报价（服务端促销链计算结果，购物车面板展示）
struct CartQuote {
	bool valid{ false };    // 是否持有有效报价（购物车变化后即失效，等下次请求）
	double originalTotal{ 0.0 };
	std::vector<std::pair<std::string, double>> promotions;  // {促销描述, 优惠金额}，按链应用顺序
	double totalDiscount{ 0.0 };
	double finalTotal{ 0.0 };
};

// 热销商品
struct TopProduct {
	std::int32_t productId{};
	std::string  name;
	std::int32_t qtySold{};
	double       revenue{};
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
	// 指定商品当前在购物车中的总件数（未加购返回 0）
	std::int32_t cartQtyOf(std::int32_t productId) const;
	// 加入购物车：若已存在同 productId 则累加 qty
	void addToCart(std::int32_t productId, std::int32_t qty = 1);
	// 从购物车减少指定商品 1 件（减到 0 移除该项）；未加购返回 false（无操作）
	bool decreaseFromCart(std::int32_t productId);
	// 移除指定 productId 的购物车项
	void removeFromCart(std::int32_t productId);
	// 清空购物车（结算成功后调用）
	void clearCart() noexcept { cart_.clear(); }
	const std::vector<CartItem>& cart() const noexcept { return cart_; }
	// 购物车总额（各项小计之和）
	double cartTotal() const;

	// === 购物车报价（服务端促销计算结果）===
	void setQuote(CartQuote quote) { quote_ = std::move(quote); quote_.valid = true; }
	const CartQuote& quote() const noexcept { return quote_; }
	// 报价失效（购物车变化/清空后调用，等待下次请求）
	void clearQuote() noexcept { quote_.valid = false; quote_.promotions.clear(); }

	// === 历史订单 ===
	void setOrders(std::vector<Order> orders) { orders_ = std::move(orders); }
	const std::vector<Order>& orders() const noexcept { return orders_; }
	// 清空订单缓存（登出时调用，防止下个用户看到上一个用户的订单）
	void clearOrders() noexcept { orders_.clear(); }

	// === 评价缓存（当前用户的全部评价，按 orderId+productId 查）===
	void setReviews(std::vector<Review> reviews) { reviews_ = std::move(reviews); }
	const std::vector<Review>& reviews() const noexcept { return reviews_; }
	void clearReviews() noexcept { reviews_.clear(); }
	// 查找某订单某商品的评价；未评价返回 nullptr
	const Review* findReview(std::int64_t orderId, std::int32_t productId) const;

	// === 商家促销规则 ===
	void setPromotions(std::vector<Promotion> promotions) { promotions_ = std::move(promotions); }
	const std::vector<Promotion>& promotions() const noexcept { return promotions_; }
	void clearPromotions() noexcept { promotions_.clear(); }

	// === 管理员用户列表 ===
	void setUsers(std::vector<SimpleUser> users) { users_ = std::move(users); }
	const std::vector<SimpleUser>& users() const noexcept { return users_; }
	void clearUsers() noexcept { users_.clear(); }

	// === 商家销售统计 ===
	void setStats(MerchantStats s) { stats_ = s; }
	const MerchantStats& stats() const noexcept { return stats_; }
	void setTopProducts(std::vector<TopProduct> v) { topProducts_ = std::move(v); }
	const std::vector<TopProduct>& topProducts() const noexcept { return topProducts_; }
	void clearStats() noexcept { stats_ = {}; topProducts_.clear(); }
	int  statsRange() const noexcept { return statsRange_; }
	void setStatsRange(int r) { statsRange_ = r; }
	int  topSortBy() const noexcept { return topSortBy_; }
	void setTopSortBy(int s) { topSortBy_ = s; }

	// === 当前登录用户身份（业务状态，非 UI 状态）===
	std::int64_t         currentUserId() const noexcept { return currentUserId_; }
	const std::string& currentUsername() const noexcept { return currentUsername_; }
	bool                 loggedIn() const noexcept { return loggedIn_; }
	User::Role           currentRole() const noexcept { return currentRole_; }
	bool                 isMerchant() const noexcept { return currentRole_ == User::Role::admin; }
	// 登录/注册成功后调用，写入服务端返回的 id + username + role
	void setUser(std::int64_t id, std::string username, User::Role role) {
		currentUserId_ = id;
		currentUsername_ = std::move(username);
		currentRole_ = role;
		loggedIn_ = true;
	}
	// 登出
	void clearUser() noexcept {
		currentUserId_ = 0;
		currentUsername_.clear();
		currentRole_ = User::Role::consumer;
		loggedIn_ = false;
	}

private:
	std::vector<Product>   products_;
	std::vector<CartItem>  cart_;
	std::vector<Order>     orders_;
	std::vector<Review>    reviews_;
	std::vector<Promotion> promotions_;
	std::vector<SimpleUser> users_;
	MerchantStats           stats_;
	std::vector<TopProduct> topProducts_;
	int                     statsRange_{0};  // 0=今日/1=本周/2=本月/3=全部
	int                     topSortBy_{0};   // 0=按销售额/1=按销量
	std::wstring           status_;  // 当前状态/提示信息
	CartQuote              quote_;   // 购物车促销报价（valid=false 表示无效）

	std::int64_t          currentUserId_{ 0 };
	std::string           currentUsername_;
	User::Role            currentRole_{ User::Role::consumer };
	bool                  loggedIn_{ false };
};
