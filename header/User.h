#pragma once
// 用户数据结构：登录/注册返回用。
#include <cstdint>
#include <string>
#include <json.hpp>

struct User {
    std::int64_t id{};
    std::string  username;

    nlohmann::json toJson() const {
        return { {"id", id}, {"username", username} };
    }
    static User fromJson(const nlohmann::json& j) {
        User u;
        u.id       = j.value("id",       std::int64_t{});
        u.username = j.value("username", std::string{});
        return u;
    }
};
