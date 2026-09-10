#pragma once
// 协议层：消息码 + 帧编解码
// 帧格式：[4字节载荷长度][JSON载荷]，定长包头 + 不定长 JSON 载荷。
#include <SFML/Network.hpp>
#include <json.hpp>
#include <optional>
#include <cstdint>

namespace proto {
    // 请求码（PPTX 第12页）
    enum class RequestCode : std::int32_t {
        ListProducts = 1001,  // 拉取商品列表
        // 注：A 方案胖客户端下 AddToCart 不启用——购物车在客户端本地维护，
        // 结算时一次性发 Checkout 上传全部 items。保留枚举值以贴合 PPT 协议码示例。
        AddToCart    = 1002,  // 加入购物车：{productId, qty}（未启用）
        Checkout     = 1003,  // 结算当前购物车：{items:[{productId, qty}]}
        AfterSale   = 1004,  // 售后：{productId, qty}（未启用）
    };

    // 应答码（2xxx 表示服务端响应）
    enum class ResponseCode : std::int32_t {
        ProductList     = 2001,  // 商品列表
        AddToCartResult = 2002,  // 加购结果（未启用，对应 AddToCart）
        CheckoutResult  = 2003,  // 结算结果：{success, orderId, total, message}
        AfterSaleResult = 2004,  // 售后结果（未启用）
        Error           = 2099,  // 错误：{message}
    };

    // 阻塞发送一条 JSON 消息；处理 Partial 直至全部发出。
    bool sendJson(sf::TcpSocket& socket, const nlohmann::json& payload);

    // 阻塞接收一条 JSON 消息；连接断开或格式错误返回 nullopt。
    std::optional<nlohmann::json> recvJson(sf::TcpSocket& socket);
}
