#pragma once
// 用户数据结构：登录/注册返回用。
#include <cstdint>
#include <string>
#include <json.hpp>

struct User {
    std::int64_t id{};
    std::string  username;
    std::int32_t role{ 0 };  // 0=普通用户，1=商家

    nlohmann::json toJson() const {
        return { {"id", id}, {"username", username}, {"role", role} };
    }
    static User fromJson(const nlohmann::json& j) {
        User u;
        u.id       = j.value("id",       std::int64_t{});
        u.username = j.value("username", std::string{});
        u.role     = j.value("role",     std::int32_t{});
        return u;
    }
};
