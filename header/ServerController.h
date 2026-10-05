#pragma once
// 服务端 Controller（业务层）：消费消息队列，处理请求，回送响应。
// 异步模型：每客户端一线程投递任务，工作线程消费队列处理。
// PPTX 第8页"服务端-业务层（异步通信）"。
#include <SFML/Network.hpp>
#include <json.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

#include "Database.h"
#include "ProductDAO.h"
#include "OrderDAO.h"
#include "UserDAO.h"
#include "PromotionDAO.h"
#include "PromotionFactory.h"
#include "ServerView.h"
#include "Protocol.h"

class ServerController {
public:
	explicit ServerController(Database& db);
	~ServerController();

	ServerController(const ServerController&) = delete;
	ServerController& operator=(const ServerController&) = delete;

	// 投递一条待处理任务；线程安全
	void submit(std::shared_ptr<sf::TcpSocket> socket, nlohmann::json request);

	// 优雅关闭：唤醒等待中的工作线程并 join
	void shutdown();

	// 重新加载促销链（从 DB 读取启用的促销规则并组装装饰器链）
	// 用于促销配置变更后热更新；线程安全（用 future 同步到工作线程上下文）
	void reloadPromotions();

private:
	struct Task {
		std::shared_ptr<sf::TcpSocket> socket;
		nlohmann::json                 request;
	};

	Database& db_;
	ProductDAO     productDao_;
	OrderDAO       orderDao_;
	UserDAO        userDao_;
	PromotionDAO   promotionDao_;
	// 促销链头：nullptr 表示无促销；多线程读需持 promotionMtx_
	std::unique_ptr<Promotion> promotionChain_;
	std::mutex     promotionMtx_;
	ServerView     view_;

	std::queue<Task>        queue_;
	std::mutex             mtx_;
	std::condition_variable cv_;
	std::atomic<bool>      running_{ true };
	std::thread            worker_;

	void workerLoop();
	// 在工作线程中处理单条任务（含回送响应）
	void handle(std::shared_ptr<sf::TcpSocket> socket, const nlohmann::json& request);

	// 各请求处理：返回应答 JSON（不含回送）
	nlohmann::json handleListProducts(const nlohmann::json& req);
	// 结算：{code, items:[{productId,qty}]} → CheckoutResult
	// 业务层职责：查价格 → 组装 CartItem → 应用促销链算折扣 → 调 OrderDAO 下单
	nlohmann::json handleCheckout(const nlohmann::json& req);
	// 拉取历史订单：→ OrderList
	nlohmann::json handleListOrders(const nlohmann::json& req);
	// 售后退货：{orderId, productId, qty} → AfterSaleResult
	nlohmann::json handleAfterSale(const nlohmann::json& req);
	// 登录：{username, password} → LoginResult
	nlohmann::json handleLogin(const nlohmann::json& req);
	// 注册：{username, password} → RegisterResult
	nlohmann::json handleRegister(const nlohmann::json& req);
	// 修改个人信息：{userId, newUsername, oldPassword, newPassword} → ProfileResult
	nlohmann::json handleUpdateProfile(const nlohmann::json& req);
	// 商家拉取全部商品（含下架）：{userId} → MerchantProductList
	nlohmann::json handleMerchantListProducts(const nlohmann::json& req);
	// 商家上架/下架：{userId, productId, onSale} → MerchantActionResult
	nlohmann::json handleMerchantSetOnSale(const nlohmann::json& req);
	// 商家调整库存：{userId, productId, stock} → MerchantActionResult
	nlohmann::json handleMerchantUpdateStock(const nlohmann::json& req);
	// 商家新增商品：{userId, name, description, price, stock, imagePath} → MerchantActionResult
	nlohmann::json handleMerchantCreateProduct(const nlohmann::json& req);
	// 商家删除商品：{userId, productId} → MerchantActionResult
	nlohmann::json handleMerchantDeleteProduct(const nlohmann::json& req);
	// 商家编辑商品：{userId, productId, name, description, price, stock, imagePath} → MerchantActionResult
    nlohmann::json handleMerchantUpdateProduct(const nlohmann::json& req);
    // 商家拉取全部订单（含用户名）：{userId} → MerchantOrderList
    nlohmann::json handleMerchantListOrders(const nlohmann::json& req);
    // 商家发货：{userId, orderId} → OrderStatusUpdateResult（待发货→已发货）
    nlohmann::json handleMerchantShipOrder(const nlohmann::json& req);
    // 用户确认收货：{userId, orderId} → OrderStatusUpdateResult（已发货→已完成）
    nlohmann::json handleUserConfirmReceive(const nlohmann::json& req);
    // 商家拉取全部促销规则：{userId} → MerchantPromotionList
    nlohmann::json handleMerchantListPromotions(const nlohmann::json& req);
    // 商家启用/禁用促销：{userId, id, enabled} → MerchantPromotionResult
    nlohmann::json handleMerchantSetPromotionEnabled(const nlohmann::json& req);
    // 商家编辑促销参数：{userId, id, params} → MerchantPromotionResult
    nlohmann::json handleMerchantUpdatePromotion(const nlohmann::json& req);
    // 商家新增促销：{userId, type, params} → MerchantPromotionResult
    nlohmann::json handleMerchantCreatePromotion(const nlohmann::json& req);
    // 商家删除促销：{userId, id} → MerchantPromotionResult
    nlohmann::json handleMerchantDeletePromotion(const nlohmann::json& req);
    // 管理员拉取全部用户列表：{userId} → AdminUserList{users:[{id,username,role}]}
    nlohmann::json handleAdminListUsers(const nlohmann::json& req);
    // 商家拉取销售统计：{userId} → MerchantStats{todayOrders,todayRevenue,totalOrders,totalRevenue,topProducts:[{id,name,qtySold,revenue}]}
    nlohmann::json handleMerchantGetStats(const nlohmann::json& req);
	// 校验请求发起者是否为商家（role=1），不是则返回错误应答
	// 成功返回 nullopt，失败返回已构造好的错误 JSON
	std::optional<nlohmann::json> requireMerchant(const nlohmann::json& req);
};
