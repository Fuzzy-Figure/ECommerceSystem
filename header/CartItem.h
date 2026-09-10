#pragma once
// 购物车项：客户端本地维护（A 方案胖客户端），结算时一次性上传服务端。
// 不持久化到磁盘，进程退出即失效。
#include <string>
#include <cstdint>
#include <json.hpp>

struct CartItem {
    std::int32_t productId{};
    std::string  name;
    double       price{};
    std::int32_t qty{};

    // 小计
    double subtotal() const noexcept { return price * static_cast<double>(qty); }

    nlohmann::json toJson() const {
        return {
            {"productId", productId},
            {"name",      name},
            {"price",     price},
            {"qty",       qty},
        };
    }

    static CartItem fromJson(const nlohmann::json& j) {
        CartItem c;
        c.productId = j.value("productId", std::int32_t{});
        c.name      = j.value("name",      std::string{});
        c.price     = j.value("price",     0.0);
        c.qty       = j.value("qty",       std::int32_t{});
        return c;
    }
};
