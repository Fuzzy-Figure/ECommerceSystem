#pragma once
// 客户端 View：SFML 渲染层
// 复用 TextManager / ImageManager；从 ClientModel 读状态，不依赖网络层。
// 面板：登录 / 商品列表 / 购物车 / 我的订单；
// 商品列表/购物车/我的订单三面板按顶部 Tab 切换；登录面板独立（不画 Tab）。
// 商品卡片右下角"加购"按钮可鼠标点击；登录面板含用户名/密码输入框 + 登录/注册按钮。
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include "TextManager.h"
#include "ImageManager.h"
#include "ClientModel.h"
#include "Product.h"

class ClientView {
public:
	// 注意顺序：Login 在前作为启动面板；后续三面板的 Tab 索引仍按 ProductList/Cart/MyOrders
	enum class Panel { Login, ProductList, Cart, MyOrders };

	// 当前聚焦的输入框（仅 Login 面板用）
	enum class Field { Username, Password };

	// 鼠标点击命中后返回的动作；type=None 表示未命中任何按钮
	struct ClickAction {
		enum Type {
			None, AddToCart, RemoveFromCart, Checkout,
			ReturnItem,    // 售后退货：orderId + productId + qty
			SwitchPanel,   // 切换面板：arg 是目标 Panel 的 int 值
			Login,         // 登录按钮：用当前输入框的用户名/密码
			Register,      // 注册按钮：同上
			FocusField     // 聚焦输入框：arg 是 Field 的 int 值
		} type{ None };
		std::int32_t arg{ 0 };     // AddToCart/RemoveFromCart 用 productId；SwitchPanel 用 panel 索引；FocusField 用 Field 索引
		std::int64_t orderId{};  // ReturnItem 用 orderId
		std::int32_t productId{};// ReturnItem 用 productId
		std::int32_t qty{ 1 };      // ReturnItem 的退货数量（暂固定 1，可扩展）
	};

	ClientView(sf::RenderWindow& window, TextManager& text, ImageManager& image);

	// 主帧渲染：根据当前面板渲染登录/商品列表/购物车/我的订单
	void render(const ClientModel& model);

	Panel panel() const noexcept { return panel_; }
	void setPanel(Panel p) noexcept { panel_ = p; }

	// === 登录面板输入框状态（仅 UI 状态，登录后丢弃）===
	Field activeField() const noexcept { return activeField_; }
	void  setActiveField(Field f) noexcept { activeField_ = f; }
	// 在当前聚焦输入框末尾追加一个 ASCII 字符（32..126）
	void  appendInputChar(char c);
	// 删除当前聚焦输入框末尾一个字符
	void  backspaceInput();
	// 清空两个输入框（登录/注册成功后调用）
	void  clearInputs() noexcept;
	const std::string& usernameInput() const noexcept { return usernameInput_; }
	const std::string& passwordInput() const noexcept { return passwordInput_; }

	// === "我的订单"面板滚动 ===
	// 滚动 deltaPx 像素（正值内容向上=向下滚动；负值相反），自动按内容/可见区夹取边界
	void  scrollMyOrders(float deltaPx, const ClientModel& model);
	// 重置到顶部（切回 MyOrders 面板时调用）
	void  resetMyOrdersScroll() noexcept { myOrdersScrollY_ = 0.f; }

	// 处理鼠标点击，返回命中按钮的动作；坐标为窗口世界坐标
	ClickAction handleClick(const sf::Vector2f& mousePos, const ClientModel& model);

private:
	sf::RenderWindow& window_;
	TextManager& textMgr_;
	ImageManager& imageMgr_;
	Panel              panel_{ Panel::Login };

	// === 登录面板输入框状态 ===
	std::string usernameInput_;
	std::string passwordInput_;
	Field       activeField_{ Field::Username };

	// === "我的订单"面板垂直滚动偏移（>=0，绘制时 y - offset）===
	float myOrdersScrollY_{ 0.f };
	// 计算 MyOrders 内容总高度（所有订单卡片 + 间距累计）
	float computeMyOrdersContentHeight(const ClientModel& model) const noexcept;
	// 计算 MyOrders 可见区高度（从 orderCardStartY 到窗口底部留 20 像素）
	float computeMyOrdersVisibleHeight() const noexcept;

	// === 商品列表面板 ===
	void drawProductListPanel(const ClientModel& model);
	// 绘制单张商品卡片；index 用于换行计算
	void drawCard(const Product& p, const sf::Vector2f& pos, const sf::Vector2f& size);

	// === 购物车面板 ===
	void drawCartPanel(const ClientModel& model);

	// === 我的订单面板 ===
	void drawMyOrdersPanel(const ClientModel& model);

	// === 登录面板 ===
	void drawLoginPanel(const ClientModel& model);

	// === 按钮矩形计算（与 drawXxxPanel 内的布局保持一致）===
	// 商品卡片右下角的"+加购"按钮
	static sf::FloatRect addToCartBtnRect(const sf::Vector2f& cardPos);
	// 购物车某行的"-"删除按钮；rowPos 是该行左上角
	static sf::FloatRect removeFromCartBtnRect(const sf::Vector2f& rowPos);
	// 购物车底部"结算"按钮
	static sf::FloatRect checkoutBtnRect();
	// 订单明细行右端的"退货"按钮；rowPos 是该明细行左上角
	static sf::FloatRect returnBtnRect(const sf::Vector2f& rowPos);
	// 顶部 Tab 标签矩形；index 取 0/1/2 对应 ProductList/Cart/MyOrders
	static sf::FloatRect tabBtnRect(int index);
	// 登录面板输入框矩形；field=0 用户名，1 密码
	static sf::FloatRect inputFieldRect(int field);
	// 登录/注册按钮矩形；btn=0 登录，1 注册
	static sf::FloatRect authBtnRect(int btn);
	// 绘制顶部 Tab 标签栏（仅 ProductList/Cart/MyOrders 三面板用，登录面板不画）
	void drawTabBar();
};
