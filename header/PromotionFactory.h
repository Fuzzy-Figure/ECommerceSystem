#pragma once
// 促销工厂（PPT 第5页"反射+策略+装饰"中的反射部分用 C++ map<string, function> 模拟）：
//   - createSingle(type, params)：按类型字符串创建单个 Promotion
//   - buildChain(configs)：按固定顺序组装装饰器链
//
// 固定顺序（A 方案）：Reduction → Discount → TieredDiscount → FreeItem → Coupon
// 即满减 → 统一折 → 阶梯 → 免单 → 券。变更顺序需修改 buildChain 内 priority。
#include "Promotion.h"
#include "PromotionDAO.h"
#include <memory>
#include <string>
#include <vector>

class PromotionFactory {
public:
	// 按类型字符串创建单个促销；type 不识别返回 nullptr
	static std::unique_ptr<Promotion> createSingle(const std::string& type,
												   const nlohmann::json& params);

	// 按固定顺序组装装饰器链：返回链头（最外层）；configs 为空返回 nullptr
	// 链方向：Reduction(最内) → Discount → Tiered → FreeItem → Coupon(最外)
	//         apply 调用时：先算 Reduction 折扣，Coupon 最后叠加券
	static std::unique_ptr<Promotion> buildChain(const std::vector<PromotionDAO::Config>& configs);

	// 便捷：直接从 DAO 读取启用的促销并组装链
	static std::unique_ptr<Promotion> buildChainFromDB(PromotionDAO& dao);
};
