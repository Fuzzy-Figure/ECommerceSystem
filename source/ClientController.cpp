#include "../header/ClientController.h"
#include "../header/utils.h"
#include "../header/Product.h"
#include <iostream>

ClientController::ClientController(sf::RenderWindow& window, ClientModel& model, ClientView& view)
    : window_(window), model_(model), view_(view) {}

ClientController::~ClientController() {
    disconnect();
}

bool ClientController::connect(const std::string& ip, unsigned short port) {
    socket_ = std::make_shared<sf::TcpSocket>();
    // IpAddress 在 SFML 3 通过 fromString 工厂构造
    auto addrOpt = sf::IpAddress::fromString(ip);
    if (!addrOpt) {
        model_.setStatus(L"无效的服务器地址：" + ec::string::to_utf16(ip));
        return false;
    }
    const auto status = socket_->connect(*addrOpt, port, sf::seconds(5));
    if (status != sf::Socket::Status::Done) {
        model_.setStatus(L"连接服务器失败，请检查服务端是否已启动");
        socket_.reset();
        return false;
    }

    running_.store(true);
    recvThread_ = std::thread(&ClientController::recvLoop, this);
    model_.setStatus(L"已连接服务器 " + ec::string::to_utf16(ip) + L"，按 R 刷新商品列表");
    return true;
}

void ClientController::disconnect() {
    running_.store(false);
    if (socket_) socket_->disconnect();
    if (recvThread_.joinable()) recvThread_.join();
    socket_.reset();
}

void ClientController::requestProductList() {
    if (!socket_) {
        model_.setStatus(L"未连接服务器，无法发送请求");
        return;
    }
    const nlohmann::json req = {
        {"code", static_cast<int>(proto::RequestCode::ListProducts)}
    };
    if (!proto::sendJson(*socket_, req)) {
        model_.setStatus(L"发送请求失败，连接可能已断开");
        return;
    }
    model_.setStatus(L"已请求商品列表，等待服务器响应...");
}

void ClientController::handleEvent(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        window_.close();
        return;
    }
    // SFML 3 中按键事件类型为 KeyPressed
    if (event.is<sf::Event::KeyPressed>()) {
        const auto* kp = event.getIf<sf::Event::KeyPressed>();
        if (kp == nullptr) return;
        const auto key = kp->code;
        if (key == sf::Keyboard::Key::R) {
            requestProductList();
        } else if (key == sf::Keyboard::Key::Escape) {
            window_.close();
        }
    }
}

void ClientController::update() {
    std::queue<nlohmann::json> local;
    {
        std::lock_guard<std::mutex> lk(msgMtx_);
        local.swap(pendingMsgs_);
    }
    while (!local.empty()) {
        auto msg = std::move(local.front());
        local.pop();
        processMessage(msg);
    }
}

void ClientController::recvLoop() {
    while (running_.load() && socket_) {
        auto msg = proto::recvJson(*socket_);
        if (!msg) {
            // 连接断开
            std::lock_guard<std::mutex> lk(msgMtx_);
            pendingMsgs_.push(nlohmann::json{
                {"code", static_cast<int>(proto::ResponseCode::Error)},
                {"message", "连接已断开"}
            });
            break;
        }
        {
            std::lock_guard<std::mutex> lk(msgMtx_);
            pendingMsgs_.push(std::move(*msg));
        }
    }
}

void ClientController::processMessage(nlohmann::json& msg) {
    const auto code = msg.value("code", 0);
    switch (code) {
        case static_cast<int>(proto::ResponseCode::ProductList): {
            std::vector<Product> products;
            if (msg.contains("products") && msg["products"].is_array()) {
                for (const auto& pj : msg["products"]) {
                    products.push_back(Product::fromJson(pj));
                }
            }
            model_.setProducts(std::move(products));
            std::wostringstream ss;
            ss << L"已加载 " << model_.products().size() << L" 件商品，按 R 刷新";
            model_.setStatus(ss.str());
            break;
        }
        case static_cast<int>(proto::ResponseCode::Error): {
            const auto m = msg.value("message", std::string{"未知错误"});
            model_.setStatus(L"错误：" + ec::string::to_utf16(m));
            break;
        }
        default:
            model_.setStatus(L"收到未知响应码：" + std::to_wstring(code));
    }
}
