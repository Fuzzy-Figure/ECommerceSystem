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
#include "PromotionDAO.h"
#include "PromotionFactory.h"
#include "ServerView.h"
#include "Protocol.h"

class ServerController {
public:
    explicit ServerController(Database& db);
    ~ServerController();

    ServerController(const ServerController&)            = delete;
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

    Database&      db_;
    ProductDAO     productDao_;
    OrderDAO       orderDao_;
    PromotionDAO   promotionDao_;
    // 促销链头：nullptr 表示无促销；多线程读需持 promotionMtx_
    std::unique_ptr<Promotion> promotionChain_;
    std::mutex     promotionMtx_;
    ServerView     view_;

    std::queue<Task>        queue_;
    std::mutex             mtx_;
    std::condition_variable cv_;
    std::atomic<bool>      running_{true};
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
};
