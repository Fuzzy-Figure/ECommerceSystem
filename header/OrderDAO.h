#pragma once
// 订单持久层：事务化下单——生成订单主表 + 明细 + 扣库存（原子）。
// 任意一步失败回滚整单，保证不会"扣了库存没下单"或"下了单没扣库存"。
#include "Database.h"
#include <cstdint>
#include <utility>
#include <vector>

class OrderDAO {
public:
    explicit OrderDAO(Database& db);

    // 下单：items 是 (productId, qty) 列表
    // 成功返回订单 ID（>0）并填 totalOut；失败（库存不足/商品不存在/异常）返回 -1
    std::int64_t placeOrder(const std::vector<std::pair<std::int32_t, std::int32_t>>& items,
                            double& totalOut);

private:
    Database& db_;
};
