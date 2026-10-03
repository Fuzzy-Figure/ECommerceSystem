#pragma once
// 客户端 Controller：事件分发 + 网络收发 + 协调 Model/View
// 接收线程独立运行，主线程在 update() 中消费消息队列更新 Model。
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <json.hpp>
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <string>

#include "ClientModel.h"
#include "ClientView.h"
#include "Protocol.h"

class ClientController {
public:
	ClientController(sf::RenderWindow& window, ClientModel& model, ClientView& view);
	~ClientController();

	ClientController(const ClientController&) = delete;
	ClientController& operator=(const ClientController&) = delete;

	// 连接服务端；成功返回 true
	bool connect(const std::string& ip, unsigned short port);

	// 断开连接并停止接收线程
	void disconnect();

	// 主动发起商品列表请求
	void requestProductList();

	// 提交购物车结算请求：把本地 cart 序列化为 items 上传
	void requestCheckout();

	// 拉取历史订单列表（用户拉自己的订单，用于"我的订单"面板）
	void requestListOrders();

	// 商家拉取全部订单列表（含下单用户名）
	void requestMerchantListOrders();

	// 商家发货：{orderId}（待发货→已发货）
	void requestMerchantShipOrder(std::int64_t orderId);

	// 用户确认收货：{orderId}（已发货→已完成）
	void requestUserConfirmReceive(std::int64_t orderId);

	// 发起售后退货：把指定订单内某 productId 的 qty 件退货
	void requestAfterSale(std::int64_t orderId, std::int32_t productId, std::int32_t qty);

	// 发起登录请求：取登录面板输入框的用户名/密码上传
	void requestLogin();

	// 发起注册请求：取登录面板输入框的用户名/密码上传
	void requestRegister();

	// 商家：拉取全部商品（含下架）
	void requestMerchantListProducts();

	// 商家：上架/下架指定商品；onSale=true 上架，false 下架
	void requestMerchantSetOnSale(std::int32_t productId, bool onSale);

	// 商家：把指定商品库存调整为 currentStock + delta（delta 可正可负，结果不低于 0）
	void requestMerchantUpdateStock(std::int32_t productId, std::int32_t delta);

	// 商家：用 MerchantCreate 表单输入提交新增商品
	void requestMerchantCreateProduct();

	// 商家：删除指定商品
	void requestMerchantDeleteProduct(std::int32_t productId);

	// 商家：用 MerchantEdit 表单输入提交编辑商品
	void requestMerchantUpdateProduct(std::int32_t productId);

	// === 商家促销管理 ===
	// 拉取全部促销规则
	void requestMerchantListPromotions();
	// 启用/禁用指定促销规则
	void requestMerchantSetPromotionEnabled(std::int32_t promotionId, bool enabled);
	// 新建促销规则（type + params，已由视图拼装校验）
	void requestMerchantCreatePromotion(const std::string& type, const nlohmann::json& params);
	// 修改促销规则参数（仅 params，type 不可改）
	void requestMerchantUpdatePromotion(std::int32_t promotionId, const nlohmann::json& params);
	// 删除促销规则
	void requestMerchantDeletePromotion(std::int32_t promotionId);

	// === 管理员用户管理 ===
	// 拉取全部用户列表
	void requestAdminListUsers();

	// 处理 SFML 事件（按键、关闭、文本输入等）
	void handleEvent(const sf::Event& event);

	// 主循环每帧调用：消费接收队列、更新 Model
	void update();

private:
	sf::RenderWindow& window_;
	ClientModel& model_;
	ClientView& view_;

	std::shared_ptr<sf::TcpSocket> socket_;
	std::thread          recvThread_;
	std::atomic<bool>    running_{ false };

	// 接收线程把消息投递到此队列，主线程在 update() 中消费
	std::queue<nlohmann::json> pendingMsgs_;
	std::mutex            msgMtx_;

	void recvLoop();
	void processMessage(nlohmann::json& msg);
};
