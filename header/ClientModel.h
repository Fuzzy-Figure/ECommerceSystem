#pragma once
// 客户端 Model：本地数据状态
// 仅持有来自服务端的商品列表快照；UI 不直接依赖网络层。
#include <vector>
#include <string>
#include "Product.h"

class ClientModel {
public:
	// 服务端推送商品列表后调用
	void setProducts(std::vector<Product> products);

	const std::vector<Product>& products() const noexcept { return products_; }

	// 连接/错误提示信息（由 Controller 写入，View 渲染）
	void setStatus(std::wstring status) { status_ = std::move(status); }
	const std::wstring& status() const noexcept { return status_; }

private:
	std::vector<Product> products_;
	std::wstring         status_;  // 当前状态/提示信息
};
