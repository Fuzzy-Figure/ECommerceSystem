#pragma once
// 持久层 DAO：评价表的数据访问对象
// 单条评价 = (order_id, product_id) 唯一；同一订单同一商品只能评价一次。
#include "Database.h"
#include "Review.h"
#include <optional>
#include <vector>

class ReviewDAO {
public:
	explicit ReviewDAO(Database& db);

	// 创建评价：同 order_id+product_id 已存在则返回 false
	bool createReview(std::int64_t orderId, std::int32_t productId,
	                  std::int64_t userId, int rating, const std::string& comment);

	// 检查某订单某商品是否已评价
	bool hasReviewed(std::int64_t orderId, std::int32_t productId);

	// 查询某商品的全部评价（按 id 升序）
	std::vector<Review> findByProductId(std::int32_t productId);

	// 查询某用户的全部评价（按 id 升序）
	std::vector<Review> findByUserId(std::int64_t userId);

	// 批量查某商品的平均分 + 评价数；无评价时 avg=0、cnt=0
	struct ProductRating { double avg; int count; };
	ProductRating getProductRating(std::int32_t productId);

private:
	static Review mapRow(const nlohmann::json& row);
	Database& db_;
};
