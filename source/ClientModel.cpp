#include "ClientModel.h"
#include <cstdint>

void ClientModel::setProducts(std::vector<Product> products) {
	products_ = std::move(products);
}

const Product* ClientModel::findProduct(std::int32_t productId) const {
	for (const auto& p : products_) {
		if (p.id == productId) return &p;
	}
	return nullptr;
}

std::int32_t ClientModel::cartQtyOf(std::int32_t productId) const {
	for (const auto& item : cart_) {
		if (item.productId == productId) {
			return item.qty;
		}
	}
	return 0;
}

void ClientModel::addToCart(std::int32_t productId, std::int32_t qty) {
	if (qty <= 0) return;
	// 已存在则累加
	for (auto& item : cart_) {
		if (item.productId == productId) {
			item.qty += qty;
			return;
		}
	}
	// 新增：从商品列表取名称/价格
	const Product* p = findProduct(productId);
	if (!p) return;
	cart_.push_back(CartItem{ productId, p->name, p->price, qty });
}

bool ClientModel::decreaseFromCart(std::int32_t productId) {
	for (auto it = cart_.begin(); it != cart_.end(); ++it) {
		if (it->productId == productId) {
			if (--it->qty <= 0) {
				cart_.erase(it);
			}
			return true;
		}
	}
	return false;
}

void ClientModel::removeFromCart(std::int32_t productId) {
	for (auto it = cart_.begin(); it != cart_.end(); ) {
		if (it->productId == productId) it = cart_.erase(it);
		else ++it;
	}
}

double ClientModel::cartTotal() const {
	double total = 0.0;
	for (const auto& c : cart_) total += c.subtotal();
	return total;
}

const Review* ClientModel::findReview(std::int64_t orderId, std::int32_t productId) const {
	for (const auto& r : reviews_) {
		if (r.orderId == orderId && r.productId == productId) return &r;
	}
	return nullptr;
}
