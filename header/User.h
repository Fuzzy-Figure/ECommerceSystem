#pragma once
// 用户数据结构：登录/注册返回用。
#include <cstdint>
#include <string>
#include <json.hpp>

struct User {
	// 用户角色：与历史存储值兼容（DB/JSON 中 consumer=0，admin=1）
	enum class Role : std::int32_t {
		consumer = 0,  // 普通用户（DB 默认值）
		admin    = 1,  // 商家
	};

	std::int64_t id{};
	std::string  username;
	Role         role{ Role::consumer };

	nlohmann::json toJson() const {
		return { {"id", id}, {"username", username}, {"role", static_cast<std::int32_t>(role)} };
	}
	static User fromJson(const nlohmann::json& j) {
		User u;
		u.id = j.value("id", std::int64_t{});
		u.username = j.value("username", std::string{});
		u.role = static_cast<Role>(j.value("role", std::int32_t{ 0 }));
		return u;
	}
};
