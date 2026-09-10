#pragma once
// 订单持久层：事务化下单 + 售后退货 + 历史查询。
// 任意一步失败回滚整单，保证不会"扣了库存没下单"或"下了单没扣库存"。
//
// 设计：折扣由业务层（ServerController）算好传入，OrderDAO 只负责持久化，
// 避免持久层反向依赖 Promotion 策略（保持分层职责清晰）。
#include "Database.h"
#include "CartItem.h"
#include "Order.h"
#include <cstdint>
#include <vector>

class OrderDAO {
public:
	explicit OrderDAO(Database& db);

	// 下单：items 含 price（由业务层准备）；discount 为已应用促销链后的折扣金额
	// 成功返回订单 ID（>0）并填 originalTotalOut/finalTotalOut；失败返回 -1
	std::int64_t placeOrder(const std::vector<CartItem>& items,
							double                       discount,
							double& originalTotalOut,
							double& finalTotalOut);

	// 拉取所有历史订单（含明细 + 已退数量），按 id 倒序（最新在前）
	std::vector<Order> findAllWithItems();

	// 售后退货：把指定订单内某 productId 的 returnQty 件退货
	// 成功返回 true，refundOut 填退款金额（按 finalTotal/originalTotal 比例分摊）
	// 失败原因（订单不存在/已全退/数量超限）返回 false
	bool placeReturn(std::int64_t       orderId,
					 std::int32_t       productId,
					 std::int32_t       returnQty,
					 double& refundOut);

private:
	Database& db_;

	// 内部：单条订单的明细拉取（供 findAllWithItems 复用）
	void loadItemsOf(Order& order);
};
