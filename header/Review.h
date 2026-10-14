#pragma once
// 评价模型：用户对已完成订单中某商品的评价。
// rating 1-5 星，comment 文本，created_at 为提交时间。
#include <string>
#include <cstdint>
#include <json.hpp>

struct Review {
	std::int64_t  id{};
	std::int64_t  orderId{};
	std::int32_t  productId{};
	std::int64_t  userId{};
	std::string   username;     // 评价者用户名（展示用）
	int           rating{};     // 1-5 星
	std::string   comment;      // 评价文本
	std::string   createdAt;    // 提交时间

	nlohmann::json toJson() const {
		return {
			{"id",        id},
			{"orderId",   orderId},
			{"productId", productId},
			{"userId",    userId},
			{"username",  username},
			{"rating",    rating},
			{"comment",   comment},
			{"createdAt", createdAt},
		};
	}
	static Review fromJson(const nlohmann::json& j) {
		Review r;
		r.id        = j.value("id", std::int64_t{});
		r.orderId   = j.value("orderId", std::int64_t{});
		r.productId = j.value("productId", std::int32_t{});
		r.userId    = j.value("userId", std::int64_t{});
		r.username  = j.value("username", std::string{});
		r.rating    = j.value("rating", 0);
		r.comment   = j.value("comment", std::string{});
		r.createdAt = j.value("createdAt", std::string{});
		return r;
	}
};
