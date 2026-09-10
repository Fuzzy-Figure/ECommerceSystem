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

private:
    struct Task {
        std::shared_ptr<sf::TcpSocket> socket;
        nlohmann::json                 request;
    };

    Database&     db_;
    ProductDAO    productDao_;
    OrderDAO      orderDao_;
    ServerView    view_;

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
    // 结算：{code:1003, items:[{productId,qty}]} → 2003
    nlohmann::json handleCheckout(const nlohmann::json& req);
};
