#include "../header/ServerController.h"
#include <iostream>
#include <utility>

ServerController::ServerController(Database& db)
    : db_(db), productDao_(db), orderDao_(db), promotionDao_(db),
      worker_(&ServerController::workerLoop, this) {
    // 启动时从 DB 加载促销规则并组装装饰器链
    reloadPromotions();
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
        case static_cast<int>(proto::RequestCode::Checkout):
            response = handleCheckout(request);
            break;
        case static_cast<int>(proto::RequestCode::ListOrders):
            response = handleListOrders(request);
            break;
        case static_cast<int>(proto::RequestCode::AfterSale):
            response = handleAfterSale(request);
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

void ServerController::reloadPromotions() {
    auto chain = PromotionFactory::buildChainFromDB(promotionDao_);
    std::lock_guard<std::mutex> lk(promotionMtx_);
    promotionChain_ = std::move(chain);
    std::cout << "[ServerController] 促销链已"
              << (promotionChain_ ? "加载" : "为空（无启用规则）")
              << std::endl;
}

nlohmann::json ServerController::handleCheckout(const nlohmann::json& req) {
    // 1. 解析购物车项：[{productId, qty}, ...]
    if (!req.contains("items") || !req["items"].is_array() || req["items"].empty()) {
        return {
            {"code",    static_cast<int>(proto::ResponseCode::CheckoutResult)},
            {"success", false},
            {"message", "结算失败：购物车为空"}
        };
    }
    std::vector<CartItem> cart;
    cart.reserve(req["items"].size());
    for (const auto& it : req["items"]) {
        const auto pid = it.value("productId", 0);
        const auto qty = it.value("qty",       0);
        if (pid <= 0 || qty <= 0) {
            return {
                {"code",    static_cast<int>(proto::ResponseCode::CheckoutResult)},
                {"success", false},
                {"message", "结算失败：商品ID或数量非法"}
            };
        }
        // 查商品信息取名称 + 价格（用于促销计算 + 订单明细）
        const auto opt = productDao_.findById(pid);
        if (!opt.has_value()) {
            return {
                {"code",    static_cast<int>(proto::ResponseCode::CheckoutResult)},
                {"success", false},
                {"message", "结算失败：商品不存在"}
            };
        }
        const auto& p = opt.value();
        cart.push_back(CartItem{ p.id, p.name, p.price, qty });
    }

    // 2. 业务层算原价 + 应用促销链算折扣（PPT 第8页"业务层：促销策略"）
    double originalTotal = 0.0;
    for (const auto& c : cart) originalTotal += c.subtotal();
    double discount = 0.0;
    {
        std::lock_guard<std::mutex> lk(promotionMtx_);
        if (promotionChain_) {
            discount = promotionChain_->apply(cart, originalTotal);
        }
    }
    if (discount < 0.0) discount = 0.0;
    if (discount > originalTotal) discount = originalTotal;
    const double finalTotal = originalTotal - discount;

    // 3. 调持久层下单（事务内写入订单 + 明细 + 扣库存）
    double outOriginal = 0.0, outFinal = 0.0;
    const auto orderId = orderDao_.placeOrder(cart, discount, outOriginal, outFinal);
    if (orderId <= 0) {
        return {
            {"code",    static_cast<int>(proto::ResponseCode::CheckoutResult)},
            {"success", false},
            {"message", "结算失败：库存不足或服务端异常"}
        };
    }
    std::cout << "[ServerController] 订单 #" << orderId
              << " 原价 " << outOriginal
              << " 折扣 " << discount
              << " 实付 " << outFinal << std::endl;
    return {
        {"code",         static_cast<int>(proto::ResponseCode::CheckoutResult)},
        {"success",      true},
        {"orderId",      orderId},
        {"originalTotal", outOriginal},
        {"discount",     discount},
        {"total",        outFinal},  // 兼容客户端原字段名
        {"message",      "结算成功"}
    };
}

nlohmann::json ServerController::handleListOrders(const nlohmann::json& /*req*/) {
    auto orders = orderDao_.findAllWithItems();
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& o : orders) arr.push_back(o.toJson());
    std::cout << "[ServerController] 返回 " << orders.size() << " 条历史订单" << std::endl;
    return {
        {"code",   static_cast<int>(proto::ResponseCode::OrderList)},
        {"orders", arr}
    };
}

nlohmann::json ServerController::handleAfterSale(const nlohmann::json& req) {
    const auto orderId   = req.value("orderId",   std::int64_t{});
    const auto productId = req.value("productId", std::int32_t{});
    const auto qty       = req.value("qty",       std::int32_t{});
    if (orderId <= 0 || productId <= 0 || qty <= 0) {
        return {
            {"code",    static_cast<int>(proto::ResponseCode::AfterSaleResult)},
            {"success", false},
            {"message", "退货失败：参数非法"}
        };
    }
    double refund = 0.0;
    if (!orderDao_.placeReturn(orderId, productId, qty, refund)) {
        return {
            {"code",    static_cast<int>(proto::ResponseCode::AfterSaleResult)},
            {"success", false},
            {"message", "退货失败：订单不存在/已全退/数量超限"}
        };
    }
    std::cout << "[ServerController] 订单 #" << orderId
              << " 退货 product=" << productId
              << " qty=" << qty
              << " 退款 ¥" << refund << std::endl;
    return {
        {"code",    static_cast<int>(proto::ResponseCode::AfterSaleResult)},
        {"success", true},
        {"refund",  refund},
        {"message", "退货成功"}
    };
}
