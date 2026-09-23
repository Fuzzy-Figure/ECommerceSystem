#pragma once
// 订单数据结构：客户端"我的订单"面板展示用。
// 含主表字段 + 明细项；明细项保留 returnedQty 用于退货 UI。
#include <cstdint>
#include <string>
#include <vector>
#include <json.hpp>

struct OrderItem {
	std::int32_t productId{};
	std::string  name;
	double       price{};        // 下单时单价
	std::int32_t qty{};          // 购买数量
	std::int32_t returnedQty{};  // 已退数量

	nlohmann::json toJson() const {
		return {
			{"productId",   productId},
			{"name",        name},
			{"price",       price},
			{"qty",         qty},
			{"returnedQty", returnedQty},
		};
	}
	static OrderItem fromJson(const nlohmann::json& j) {
		OrderItem o;
		o.productId = j.value("productId", std::int32_t{});
		o.name = j.value("name", std::string{});
		o.price = j.value("price", 0.0);
		o.qty = j.value("qty", std::int32_t{});
		o.returnedQty = j.value("returnedQty", std::int32_t{});
		return o;
	}
};

struct Order {
	std::int64_t id{};
	double       originalTotal{};  // 原价合计
	double       discount{};       // 折扣
	double       finalTotal{};     // 实付
	std::string  createdAt;
	int          status{};         // 0=正常, 1=部分退货, 2=全部退货
	std::vector<OrderItem> items;

	nlohmann::json toJson() const {
		nlohmann::json arr = nlohmann::json::array();
		for (const auto& it : items) arr.push_back(it.toJson());
		return {
			{"id",           id},
			{"originalTotal",originalTotal},
			{"discount",     discount},
			{"finalTotal",   finalTotal},
			{"createdAt",    createdAt},
			{"status",       status},
			{"items",        arr},
		};
	}
	static Order fromJson(const nlohmann::json& j) {
		Order o;
		o.id = j.value("id", std::int64_t{});
		o.originalTotal = j.value("originalTotal", 0.0);
		o.discount = j.value("discount", 0.0);
		o.finalTotal = j.value("finalTotal", 0.0);
		o.createdAt = j.value("createdAt", std::string{});
		o.status = j.value("status", 0);
		if (j.contains("items") && j["items"].is_array()) {
			for (const auto& ij : j["items"]) o.items.push_back(OrderItem::fromJson(ij));
		}
		return o;
	}
};
