#pragma once
// 促销策略族（PPT 第5页 + 第8页双重点名）：
//   - 策略模式：Promotion 抽象基类，派生类实现具体算法
//   - 装饰器模式：基类内置 inner_，叠加多促销时链式包装
//   - 工厂模式：PromotionFactory 按固定顺序组装装饰器链（见 PromotionFactory.h）
//
// 设计要点：子类只实现 myDiscount；apply 在基类统一处理"先算 inner 再叠加自己"。
// 应用顺序（A 方案固定）：Reduction(满减) → Discount(统一折) → TieredDiscount(阶梯)
//                          → FreeItem(免单) → Coupon(券)
#include "CartItem.h"
#include <memory>
#include <string>
#include <vector>
#include <utility>

class Promotion {
public:
	virtual ~Promotion() = default;

	// 主入口：基于 cart + 当前累计应付金额 currentTotal，返回本次促销链累计折扣金额。
	// currentTotal 是"前面所有促销应用后"的金额（装饰器链式语义）。
	double apply(const std::vector<CartItem>& cart, double currentTotal) const;

	// 装饰器接口：包装内层促销（nullptr 表示叶子策略，apply 时跳过 inner）
	void setInner(std::unique_ptr<Promotion> inner) { inner_ = std::move(inner); }

	// 描述本次促销（用于订单明细、UI 展示）
	virtual std::string description() const = 0;

protected:
	// 子类实现：基于"前序促销应用后"的金额 afterPrevTotal，计算自己的折扣金额（必须 >=0）。
	virtual double myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const = 0;

private:
	std::unique_ptr<Promotion> inner_;  // 装饰器链：被装饰的内层促销
};

// 1. 统一折扣：整单 × rate（如 0.8 = 全场8折）
class Discount : public Promotion {
public:
	explicit Discount(double rate);
	std::string description() const override;
protected:
	double myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const override;
private:
	double rate_;
};

// 2. 阶梯折扣：同一商品按件数递增折扣，如第2件9折、第3件及以后8折
//    tiers: [(2, 0.9), (3, 0.8)] 表示第2件该件9折、第3件及以后该件8折
class TieredDiscount : public Promotion {
public:
	explicit TieredDiscount(std::vector<std::pair<int, double>> tiers);
	std::string description() const override;
protected:
	double myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const override;
private:
	std::vector<std::pair<int, double>> tiers_;
};

// 3. 免单：买 N 送 M（最便宜的 M 件免费）
class FreeItem : public Promotion {
public:
	FreeItem(int buyN, int freeM);
	std::string description() const override;
protected:
	double myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const override;
private:
	int buyN_;
	int freeM_;
};

// 4. 满减：满 threshold 减 reduce（不减成负数）
class Reduction : public Promotion {
public:
	Reduction(double threshold, double reduce);
	std::string description() const override;
protected:
	double myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const override;
private:
	double threshold_;
	double reduce_;
};

// 5. 券：固定金额抵扣（不抵成负数）
class Coupon : public Promotion {
public:
	explicit Coupon(double amount);
	std::string description() const override;
protected:
	double myDiscount(const std::vector<CartItem>& cart, double afterPrevTotal) const override;
private:
	double amount_;
};
