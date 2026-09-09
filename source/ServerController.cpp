#include "../header/ServerController.h"
#include <iostream>
#include <utility>

ServerController::ServerController(Database& db)
    : db_(db), productDao_(db), worker_(&ServerController::workerLoop, this) {
    std::cout << "[ServerController] 业务层已启动，工作线程就绪" << std::endl;
}

ServerController::~ServerController() {
    shutdown();
}

void ServerController::shutdown() {
    if (!running_.exchange(false)) return;
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    std::cout << "[ServerController] 已关闭" << std::endl;
}

void ServerController::submit(std::shared_ptr<sf::TcpSocket> socket, nlohmann::json request) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        queue_.push(Task{std::move(socket), std::move(request)});
    }
    cv_.notify_one();
}

void ServerController::workerLoop() {
    while (running_.load()) {
        Task task;
        {
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait(lk, [this] { return !running_.load() || !queue_.empty(); });
            if (!running_.load() && queue_.empty()) return;
            if (queue_.empty()) continue;
            task = std::move(queue_.front());
            queue_.pop();
        }
        try {
            handle(task.socket, task.request);
        } catch (const std::exception& e) {
            view_.showError(std::string{"处理请求异常："} + e.what());
        }
    }
}

void ServerController::handle(std::shared_ptr<sf::TcpSocket> socket, const nlohmann::json& request) {
    const auto remote = socket->getRemoteAddress();
    const std::string addr = remote ? remote->toString() : std::string{"unknown"};
    view_.showRequest(addr, request.dump());

    const auto code = request.value("code", 0);

    nlohmann::json response;
    switch (code) {
        case static_cast<int>(proto::RequestCode::ListProducts):
            response = handleListProducts(request);
            break;
        default:
            response = {
                {"code",    static_cast<int>(proto::ResponseCode::Error)},
                {"message", "未知请求码：" + std::to_string(code)}
            };
    }

    if (!proto::sendJson(*socket, response)) {
        view_.showError("回送响应失败，连接可能已断开");
    } else {
        view_.showResponse(addr, response.dump());
    }
}

nlohmann::json ServerController::handleListProducts(const nlohmann::json& /*req*/) {
    auto products = productDao_.findAll();
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& p : products) arr.push_back(p.toJson());
    return {
        {"code",     static_cast<int>(proto::ResponseCode::ProductList)},
        {"products", arr}
    };
}
