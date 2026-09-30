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
	// 注意顺序：Login=0；商家面板 Merchant/MerchantCreate/MerchantEdit/Promotion/PromotionForm 独立（不参与 Tab 切换）；
	// ProductList/Cart/MyOrders 三面板按顶部 Tab 切换，Tab 索引仍按 0/1/2；商家 Tab 为 商品列表/我的订单/促销管理。
	enum class Panel { Login, Merchant, MerchantCreate, MerchantEdit, ProductList, Cart, MyOrders, Promotion, PromotionForm };

	// 当前聚焦的输入框
	enum class Field { Username, Password, ProductName, ProductPrice, ProductStock, ProductDesc, ProductImage, ProductSearch, PromoSlot0, PromoSlot1, PromoSlot2, PromoSlot3 };

	// 鼠标点击命中后返回的动作；type=None 表示未命中任何按钮
	struct ClickAction {
		enum Type {
			None, AddToCart, RemoveFromCart, Checkout,
			ReturnItem,    // 售后退货：orderId + productId + qty
			SwitchPanel,   // 切换面板：arg 是目标 Panel 的 int 值
			Login,         // 登录按钮：用当前输入框的用户名/密码
			Register,      // 注册按钮：同上
			FocusField,    // 聚焦输入框：arg 是 Field 的 int 值
			Logout,        // 登出按钮：清用户身份 + 切回登录面板
			MerchantSetOnSale,  // 商家上架/下架：productId + arg(0=下架,1=上架)
			MerchantStockPlus,  // 商家库存 +10：productId
			MerchantStockMinus,// 商家库存 -10：productId（不低于0）
			MerchantCreateProduct,  // 商家新增商品：用 MerchantCreate 表单输入
			MerchantCreateBack,     // 商家新增商品表单的"返回"按钮：切回 Merchant 面板
			MerchantDeleteProduct,  // 商家删除商品：productId
			MerchantEditProduct,    // 商家编辑商品：productId → 预填表单切到 MerchantEdit 面板
			MerchantEditSubmit,     // 商家编辑表单的"保存修改"按钮
			MerchantEditBack,       // 商家编辑表单的"返回"按钮
			MerchantShipOrder,      // 商家发货：orderId（待发货→已发货）
			UserConfirmReceive,     // 用户确认收货：orderId（已发货→已完成）
			MerchantPromoToggle,    // 商家启用/禁用促销：promotionId + arg(0=禁用,1=启用)
			MerchantPromoEdit,      // 商家编辑促销：promotionId → 切到 PromotionForm 面板
			MerchantPromoDelete,    // 商家删除促销：promotionId
			MerchantPromoCreate,    // 商家新增促销入口：切到 PromotionForm 面板（创建模式）
			MerchantPromoFormSubmit,// 促销表单提交（创建或编辑）
			MerchantPromoFormBack,  // 促销表单返回：切回 Promotion 面板
			MerchantPromoSelectType // 促销表单选择类型：arg 是类型索引 0..4
		} type{ None };
		std::int32_t arg{ 0 };     // 通用参数
		std::int64_t orderId{};  // ReturnItem 用 orderId
		std::int32_t productId{};// ReturnItem/Merchant* 用 productId
		std::int32_t promotionId{}; // 促销操作的 promotionId
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
	// 在当前聚焦输入框末尾追加一个字符（接收 UTF-32，ASCII 32..126 或中文 CJK 0x4E00..0x9FFF）
	// 内部转 UTF-8 存储；其他字符忽略
	void  appendInputChar(std::uint32_t ch);
	// 删除当前聚焦输入框末尾一个字符
	void  backspaceInput();
	// 清空输入框（登录/注册成功、切面板、提交新增商品后调用）
	void  clearInputs() noexcept;
	const std::string& usernameInput() const noexcept { return usernameInput_; }
	const std::string& passwordInput() const noexcept { return passwordInput_; }

	// === 商家新增商品表单输入框状态（仅 UI 状态，提交/返回后丢弃）===
	const std::string& productNameInput()  const noexcept { return productNameInput_; }
	const std::string& productPriceInput() const noexcept { return productPriceInput_; }
	const std::string& productStockInput() const noexcept { return productStockInput_; }
	const std::string& productDescInput()  const noexcept { return productDescInput_; }
	const std::string& productImageInput() const noexcept { return productImageInput_; }

	// === "我的订单"面板滚动 ===
	// 滚动 deltaPx 像素（正值内容向上=向下滚动；负值相反），自动按内容/可见区夹取边界
	void  scrollMyOrders(float deltaPx, const ClientModel& model);
	// 重置到顶部（切回 MyOrders 面板时调用）
	void  resetMyOrdersScroll() noexcept { myOrdersScrollY_ = 0.f; }

	// === 商品列表面板滚动 ===
	// 滚动 deltaPx 像素（正值内容向上=向下滚动；负值相反），自动按内容/可见区夹取边界
	void  scrollProductList(float deltaPx, const ClientModel& model);
	// 重置到顶部（切回 ProductList 面板时调用）
	void  resetProductListScroll() noexcept { productListScrollY_ = 0.f; }

	// === 商家编辑商品表单 ===
	// 预填商品信息到编辑表单输入框 + 记录编辑的 productId
	void setEditProduct(std::int32_t id, const std::string& name, double price,
						std::int32_t stock, const std::string& desc, const std::string& image);
	// 当前正在编辑的 productId
	std::int32_t editingProductId() const noexcept { return editingProductId_; }

	// === 商家促销表单（创建/编辑共用）===
	// 进入创建模式：清空表单，promotionId=0
	void resetPromotionForm();
	// 进入编辑模式：预填 type + params JSON（拆入对应槽位），记录 promotionId
	void setEditPromotion(std::int32_t id, const std::string& type, const nlohmann::json& params);
	// 当前编辑的促销 id（0=创建模式）
	std::int32_t editingPromotionId() const noexcept { return editingPromotionId_; }
	// 当前选中的促销类型（创建模式下由用户选，编辑模式下固定）
	const std::string& promoType() const noexcept { return promoType_; }
	// 设置促销类型（创建模式下选择类型按钮调用）
	void setPromoType(const std::string& type);
	// 根据当前类型 + 各槽位输入拼装 params JSON；校验失败返回空对象
	nlohmann::json buildPromotionParams() const;

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
	// === 商家新增商品表单输入框状态 ===
	std::string productNameInput_;
	std::string productPriceInput_;
	std::string productStockInput_;
	std::string productDescInput_;
	std::string productImageInput_;
	// === 商品列表搜索框 ===
	std::string searchInput_;
	Field       activeField_{ Field::Username };
	// === 商家编辑商品的 productId ===
	std::int32_t editingProductId_{ 0 };
	// === 商家促销表单 ===
	// 4 个参数槽位，根据类型填充不同含义（如满减：[满额, 减额]；折扣：[折数]）
	std::string promoSlotValue_[4];
	std::int32_t editingPromotionId_{ 0 }; // 0=创建模式，>0=编辑模式
	std::string promoType_;               // 当前促销类型

	// === "我的订单"面板垂直滚动偏移（>=0，绘制时 y - offset）===
	float myOrdersScrollY_{ 0.f };
	// 计算 MyOrders 内容总高度（所有订单卡片 + 间距累计）
	float computeMyOrdersContentHeight(const ClientModel& model) const noexcept;
	// 计算 MyOrders 可见区高度（从 orderCardStartY 到窗口底部留 20 像素）
	float computeMyOrdersVisibleHeight() const noexcept;

	// === 商品列表面板垂直滚动偏移（>=0，绘制时 y - offset）===
	float productListScrollY_{ 0.f };
	// 计算 ProductList 内容总高度（所有商品卡片按 3 列网格布局累计）
	float computeProductListContentHeight(const ClientModel& model) const noexcept;
	// 计算 ProductList 可见区高度（从 startY 到窗口底部留 20 像素）
	float computeProductListVisibleHeight() const noexcept;

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

	// === 商家管理面板 ===
	void drawMerchantPanel(const ClientModel& model);
	// 商家商品行：上架/下架按钮矩形
	sf::FloatRect merchantSaleBtnRect(const sf::Vector2f& rowPos) const;
	// 商家商品行：库存 +10 按钮矩形
	sf::FloatRect merchantStockPlusBtnRect(const sf::Vector2f& rowPos) const;
	// 商家商品行：库存 -10 按钮矩形
	sf::FloatRect merchantStockMinusBtnRect(const sf::Vector2f& rowPos) const;
	// 商家商品行：删除按钮矩形（状态列后面）
	sf::FloatRect merchantDeleteBtnRect(const sf::Vector2f& rowPos) const;
	// 商家商品行：编辑按钮矩形（删除按钮左边）
	sf::FloatRect merchantEditBtnRect(const sf::Vector2f& rowPos) const;
	// 商家面板底部"新增商品"按钮矩形
	sf::FloatRect merchantCreateBtnRect() const;

	// === 商家新增商品表单面板 ===
	void drawMerchantCreatePanel(const ClientModel& model);
	// 表单输入框矩形；field 取 2..6 对应 ProductName/Price/Stock/Desc/Image
	static sf::FloatRect merchantCreateFieldRect(int field);
	// 表单"提交新增"按钮矩形
	static sf::FloatRect merchantCreateSubmitBtnRect();
	// 表单"返回"按钮矩形
	static sf::FloatRect merchantCreateBackBtnRect();

	// === 商家编辑商品表单面板 ===
	void drawMerchantEditPanel(const ClientModel& model);

	// === 商家促销列表面板 ===
	void drawPromotionPanel(const ClientModel& model);
	// 促销行：启用/禁用按钮矩形
	sf::FloatRect promoToggleBtnRect(const sf::Vector2f& rowPos) const;
	// 促销行：编辑按钮矩形
	sf::FloatRect promoEditBtnRect(const sf::Vector2f& rowPos) const;
	// 促销行：删除按钮矩形
	sf::FloatRect promoDeleteBtnRect(const sf::Vector2f& rowPos) const;
	// 促销面板底部"新增促销"按钮矩形
	sf::FloatRect promoCreateBtnRect() const;

	// === 商家促销表单面板（创建/编辑）===
	void drawPromotionFormPanel(const ClientModel& model);
	// 促销类型选择按钮矩形；index 0..4 对应 5 种类型
	sf::FloatRect promoTypeBtnRect(int index) const;
	// 促销参数槽位输入框矩形；index 0..3 对应 4 个槽位
	sf::FloatRect promoSlotRect(int index) const;
	// 促销表单提交按钮矩形
	sf::FloatRect promoSubmitBtnRect() const;
	// 促销表单返回按钮矩形
	sf::FloatRect promoBackBtnRect() const;

	// === 按钮矩形计算（与 drawXxxPanel 内的布局保持一致）===
	// 商品卡片右下角的"+加购"按钮
	static sf::FloatRect addToCartBtnRect(const sf::Vector2f& cardPos);
	// 购物车某行的"-"删除按钮；rowPos 是该行左上角
	static sf::FloatRect removeFromCartBtnRect(const sf::Vector2f& rowPos);
	// 购物车底部"结算"按钮
	static sf::FloatRect checkoutBtnRect();
	// 订单明细行右端的"退货"按钮；rowPos 是该明细行左上角
	static sf::FloatRect returnBtnRect(const sf::Vector2f& rowPos);
	// 订单卡片右上角操作按钮（发货/确认收货）；cardTop 是卡片左上角
	sf::FloatRect orderActionBtnRect(const sf::Vector2f& cardTop) const;
	// 顶部 Tab 标签矩形；index 取 0/1/2 对应 ProductList/Cart/MyOrders
	static sf::FloatRect tabBtnRect(int index);
	// 登录面板输入框矩形；field=0 用户名，1 密码
	static sf::FloatRect inputFieldRect(int field);
	// 登录/注册按钮矩形；btn=0 登录，1 注册
	static sf::FloatRect authBtnRect(int btn);
	// 右上角登出按钮矩形（窗口右边界附近，y=tabY）
	sf::FloatRect logoutBtnRect() const;
	// 绘制顶部 Tab 标签栏 + 右上角用户名/登出按钮（仅 ProductList/Cart/MyOrders 三面板用）
	void drawTabBar(const ClientModel& model);
};
