#pragma once
// 客户端 Controller：事件分发 + 网络收发 + 协调 Model/View
// 接收线程独立运行，主线程在 update() 中消费消息队列更新 Model。
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <json.hpp>
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

    ClientController(const ClientController&)            = delete;
    ClientController& operator=(const ClientController&) = delete;

    // 连接服务端；成功返回 true
    bool connect(const std::string& ip, unsigned short port);

    // 断开连接并停止接收线程
    void disconnect();

    // 主动发起商品列表请求（PPTX 协议码 1001）
    void requestProductList();

    // 处理 SFML 事件（按键、关闭等）
    void handleEvent(const sf::Event& event);

    // 主循环每帧调用：消费接收队列、更新 Model
    void update();

private:
    sf::RenderWindow& window_;
    ClientModel&       model_;
    ClientView&        view_;

    std::shared_ptr<sf::TcpSocket> socket_;
    std::thread          recvThread_;
    std::atomic<bool>    running_{false};

    // 接收线程把消息投递到此队列，主线程在 update() 中消费
    std::queue<nlohmann::json> pendingMsgs_;
    std::mutex            msgMtx_;

    void recvLoop();
    void processMessage(nlohmann::json& msg);
};
