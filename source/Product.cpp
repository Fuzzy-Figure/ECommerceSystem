#include "Product.h"

nlohmann::json Product::toJson() const {
	return {
		{"id",          id},
		{"name",        name},
		{"description", description},
		{"price",       price},
		{"stock",       stock},
		{"imagePath",   imagePath},
		{"onSale",      onSale},
		{"avgRating",   avgRating},
		{"reviewCount", reviewCount},
	};
}

Product Product::fromJson(const nlohmann::json& j) {
	Product p;
	p.id = j.value("id", std::int32_t{});
	p.name = j.value("name", std::string{});
	p.description = j.value("description", std::string{});
	p.price = j.value("price", 0.0);
	p.stock = j.value("stock", std::int32_t{});
	p.imagePath = j.value("imagePath", std::string{});
	p.onSale = j.value("onSale", true);
	p.avgRating = j.value("avgRating", 0.0);
	p.reviewCount = j.value("reviewCount", 0);
	return p;
}
