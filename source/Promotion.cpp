#include "../header/Promotion.h"
#include <algorithm>
#include <sstream>

// === Promotion 基类 ===
double Promotion::apply(const std::vector<CartItem>& cart, double currentTotal) const {
	// 先算内层装饰的累计折扣（叶子策略 inner_ 为 nullptr）
	const double prev = inner_ ? inner_->apply(cart, currentTotal) : 0.0;
	const double afterPrev = currentTotal - prev;
	const double mine = myDiscount(cart, afterPrev);
	// 防御：单步折扣不能让金额变负
	const double safeMine = (mine > afterPrev) ? afterPrev : mine;
	return prev + safeMine;
}

// === 1. 统一折扣 ===
Discount::Discount(double rate) : rate_(rate) {}

std::string Discount::description() const {
	std::ostringstream ss;
	ss << "全场" << (rate_ * 10.0) << "折";
	return ss.str();
}

double Discount::myDiscount(const std::vector<CartItem>& /*cart*/, double afterPrevTotal) const {
	if (rate_ <= 0.0 || rate_ >= 1.0) return 0.0;  // 无效或无折扣
	return afterPrevTotal * (1.0 - rate_);
}

// === 2. 阶梯折扣：同一商品按件数递增折扣 ===
TieredDiscount::TieredDiscount(std::vector<std::pair<int, double>> tiers) : tiers_(std::move(tiers)) {
	// 按"件数"升序排序，方便后续匹配
	std::sort(tiers_.begin(), tiers_.end(),
			  [](const auto& a, const auto& b) { return a.first < b.first; });
}

std::string TieredDiscount::description() const {
	std::ostringstream ss;
	ss << "阶梯折：";
	for (std::size_t i = 0; i < tiers_.size(); ++i) {
		if (i) ss << "、";
		ss << "第" << tiers_[i].first << "件" << (tiers_[i].second * 10.0) << "折";
	}
	return ss.str();
}

double TieredDiscount::myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const {
	if (tiers_.empty()) return 0.0;
	// afterPrevTotal 是整单已应用前序促销后的总额，按原 cart 总额占比分摊到每件
	const double originalTotal = [cart]() {
		double t = 0.0;
		for (const auto& c : cart) t += c.subtotal();
		return t;
	}();
	if (originalTotal <= 0.0) return 0.0;

	// 对每个 cart 项，按件数阶梯计算该商品折后小计
	double newTotal = 0.0;
	for (const auto& c : cart) {
		const double unitPrice = c.subtotal() / static_cast<double>(c.qty);
		for (int n = 1; n <= c.qty; ++n) {
			// 找该件适用的最大阶梯（件数 <= n 的最大档）
			double rate = 1.0;
			for (const auto& [thN, thRate] : tiers_) {
				if (n >= thN) rate = thRate;
			}
			newTotal += unitPrice * rate;
		}
	}
	// afterPrevTotal 比例缩放：前序促销已对整单打折，此处按比例映射
	const double scaledNew = newTotal * (afterPrevTotal / originalTotal);
	const double discount = afterPrevTotal - scaledNew;
	return discount > 0.0 ? discount : 0.0;
}

// === 3. 免单：买 N 送 M（最便宜的 M 件免费） ===
FreeItem::FreeItem(int buyN, int freeM) : buyN_(buyN), freeM_(freeM) {}

std::string FreeItem::description() const {
	std::ostringstream ss;
	ss << "买" << buyN_ << "送" << freeM_;
	return ss.str();
}

double FreeItem::myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const {
	if (buyN_ <= 0 || freeM_ <= 0) return 0.0;
	// 收集所有"单件价格"，按从低到高排序，最便宜的 freeM 件免费
	// 注意：买 N 送 M 需达到 N 件才生效，按"每满 N 件送 M 件"规则
	int totalQty = 0;
	for (const auto& c : cart) totalQty += c.qty;
	if (totalQty < buyN_) return 0.0;

	// 每满 N 件触发一次送 M 件
	const int groups = totalQty / buyN_;
	const int freeCount = groups * freeM_;
	if (freeCount <= 0) return 0.0;

	// 收集所有件单价（按 cart 项展开），从低到高排序
	std::vector<double> unitPrices;
	unitPrices.reserve(totalQty);
	for (const auto& c : cart) {
		const double unitPrice = c.subtotal() / static_cast<double>(c.qty);
		for (int i = 0; i < c.qty; ++i) unitPrices.push_back(unitPrice);
	}
	std::sort(unitPrices.begin(), unitPrices.end());

	// 最便宜的 freeCount 件免费——但需按 afterPrevTotal/原总额比例缩放（前序促销已打折）
	const double originalTotal = [cart]() {
		double t = 0.0;
		for (const auto& c : cart) t += c.subtotal();
		return t;
	}();
	if (originalTotal <= 0.0) return 0.0;
	const double scale = afterPrevTotal / originalTotal;

	double discount = 0.0;
	for (int i = 0; i < freeCount && i < totalQty; ++i) {
		discount += unitPrices[i];
	}
	return discount * scale;
}

// === 4. 满减：满 threshold 减 reduce（不减成负数） ===
Reduction::Reduction(double threshold, double reduce) : threshold_(threshold), reduce_(reduce) {}

std::string Reduction::description() const {
	std::ostringstream ss;
	ss << "满" << threshold_ << "减" << reduce_;
	return ss.str();
}

double Reduction::myDiscount(const std::vector<CartItem>& /*cart*/, double afterPrevTotal) const {
	if (afterPrevTotal < threshold_) return 0.0;
	return (reduce_ > afterPrevTotal) ? afterPrevTotal : reduce_;
}

// === 5. 券：固定金额抵扣（不抵成负数） ===
Coupon::Coupon(double amount) : amount_(amount) {}

std::string Coupon::description() const {
	std::ostringstream ss;
	ss << "抵扣券" << amount_ << "元";
	return ss.str();
}

double Coupon::myDiscount(const std::vector<CartItem>& /*cart*/, double afterPrevTotal) const {
	return (amount_ > afterPrevTotal) ? afterPrevTotal : amount_;
}
