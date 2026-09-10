#pragma once
// 商品模型：服务端持久层产出、客户端模型层缓存、协议层传输。
// 不依赖具体持久化实现，使用 JSON 作为交换格式。
#include <string>
#include <cstdint>
#include <json.hpp>

struct Product {
	std::int32_t id{};
	std::string  name;
	std::string  description;
	double       price{};
	std::int32_t stock{};
	std::string  imagePath;  // 相对路径，客户端用 ImageManager 加载

	nlohmann::json toJson() const;
	static Product fromJson(const nlohmann::json& j);
};
