#include "../header/ClientController.h"
#include "../header/utils.h"
#include "../header/Product.h"
#include "../header/Order.h"
#include <iostream>

ClientController::ClientController(sf::RenderWindow& window, ClientModel& model, ClientView& view)
	: window_(window), model_(model), view_(view) {}

ClientController::~ClientController() {
	disconnect();
}

bool ClientController::connect(const std::string& ip, unsigned short port) {
	socket_ = std::make_shared<sf::TcpSocket>();
	// IpAddress 在 SFML 3 通过 fromString 工厂构造
	auto addrOpt = sf::IpAddress::fromString(ip);
	if (!addrOpt) {
		model_.setStatus(L"无效的服务器地址：" + ec::string::to_utf16(ip));
		return false;
	}
	const auto status = socket_->connect(*addrOpt, port, sf::seconds(5));
	if (status != sf::Socket::Status::Done) {
		model_.setStatus(L"连接服务器失败，请检查服务端是否已启动");
		socket_.reset();
		return false;
	}

	running_.store(true);
	recvThread_ = std::thread(&ClientController::recvLoop, this);
	model_.setStatus(L"已连接服务器 " + ec::string::to_utf16(ip));
	return true;
}

void ClientController::disconnect() {
	running_.store(false);
	if (socket_) socket_->disconnect();
	if (recvThread_.joinable()) recvThread_.join();
	socket_.reset();
}

void ClientController::requestProductList() {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法发送请求");
		return;
	}
	const nlohmann::json req = {
		{"code", static_cast<int>(proto::RequestCode::ListProducts)}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(L"已请求商品列表，等待服务器响应...");
}

void ClientController::requestCheckout() {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法结算");
		return;
	}
	const auto& cart = model_.cart();
	if (cart.empty()) {
		model_.setStatus(L"购物车为空，无法结算");
		return;
	}
	nlohmann::json items = nlohmann::json::array();
	for (const auto& c : cart) {
		items.push_back({
			{"productId", c.productId},
			{"qty",       c.qty}
						});
	}
	const nlohmann::json req = {
		{"code",   static_cast<int>(proto::RequestCode::Checkout)},
		{"items",  items},
		{"userId", model_.currentUserId()}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送结算请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(L"已提交结算请求，等待服务器响应...");
}

void ClientController::requestListOrders() {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法拉取订单");
		return;
	}
	const nlohmann::json req = {
		{"code",   static_cast<int>(proto::RequestCode::ListOrders)},
		{"userId", model_.currentUserId()}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送订单列表请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(L"已请求历史订单，等待服务器响应...");
}

void ClientController::requestAfterSale(std::int64_t orderId, std::int32_t productId, std::int32_t qty) {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法发起售后");
		return;
	}
	const nlohmann::json req = {
		{"code",      static_cast<int>(proto::RequestCode::AfterSale)},
		{"orderId",   orderId},
		{"productId", productId},
		{"qty",       qty}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送退货请求失败，连接可能已断开");
		return;
	}
	std::wostringstream ss;
	ss << L"已提交退货请求：订单 #" << orderId << L"，请等待处理...";
	model_.setStatus(ss.str());
}

void ClientController::requestMerchantListProducts() {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法拉取商家商品");
		return;
	}
	const nlohmann::json req = {
		{"code",   static_cast<int>(proto::RequestCode::MerchantListProducts)},
		{"userId", model_.currentUserId()}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送商家商品请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(L"正在拉取商家商品列表...");
}

void ClientController::requestMerchantSetOnSale(std::int32_t productId, bool onSale) {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法操作");
		return;
	}
	const nlohmann::json req = {
		{"code",      static_cast<int>(proto::RequestCode::MerchantSetOnSale)},
		{"userId",    model_.currentUserId()},
		{"productId", productId},
		{"onSale",    onSale}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送上下架请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(onSale ? L"正在上架..." : L"正在下架...");
}

void ClientController::requestMerchantUpdateStock(std::int32_t productId, std::int32_t delta) {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法调整库存");
		return;
	}
	// 先从本地模型查到当前库存，计算目标值（服务端只接受绝对值，不接受 delta）
	std::int32_t current = 0;
	if (const auto* p = model_.findProduct(productId)) current = p->stock;
	const std::int32_t target = std::max(0, current + delta);
	const nlohmann::json req = {
		{"code",      static_cast<int>(proto::RequestCode::MerchantUpdateStock)},
		{"userId",    model_.currentUserId()},
		{"productId", productId},
		{"stock",     target}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送库存调整请求失败，连接可能已断开");
		return;
	}
	std::wostringstream ss;
	ss << L"正在将库存调整为 " << target << L"...";
	model_.setStatus(ss.str());
}

void ClientController::requestMerchantCreateProduct() {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法新增商品");
		return;
	}
	const auto& name  = view_.productNameInput();
	const auto& price = view_.productPriceInput();
	const auto& stock = view_.productStockInput();
	const auto& desc  = view_.productDescInput();
	const auto& image = view_.productImageInput();
	if (name.empty()) {
		model_.setStatus(L"商品名称不能为空");
		return;
	}
	// 价格/库存必须是合法数字
	double priceVal = 0.0;
	std::int32_t stockVal = 0;
	try {
		priceVal = std::stod(price);
	} catch (...) {
		model_.setStatus(L"价格必须是数字（如 19.9）");
		return;
	}
	try {
		stockVal = std::stoi(stock);
	} catch (...) {
		model_.setStatus(L"库存必须是整数（如 100）");
		return;
	}
	if (priceVal < 0 || stockVal < 0) {
		model_.setStatus(L"价格和库存不能为负数");
		return;
	}
	const nlohmann::json req = {
		{"code",        static_cast<int>(proto::RequestCode::MerchantCreateProduct)},
		{"userId",      model_.currentUserId()},
		{"name",        name},
		{"description", desc},
		{"price",       priceVal},
		{"stock",       stockVal},
		{"imagePath",   image}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送新增商品请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(L"正在提交新增商品...");
}

void ClientController::requestLogin() {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法登录");
		return;
	}
	const auto& username = view_.usernameInput();
	const auto& password = view_.passwordInput();
	if (username.empty() || password.empty()) {
		model_.setStatus(L"用户名或密码不能为空");
		return;
	}
	const nlohmann::json req = {
		{"code",     static_cast<int>(proto::RequestCode::Login)},
		{"username", username},
		{"password", password}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送登录请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(L"正在登录，请稍候...");
}

void ClientController::requestRegister() {
	if (!socket_) {
		model_.setStatus(L"未连接服务器，无法注册");
		return;
	}
	const auto& username = view_.usernameInput();
	const auto& password = view_.passwordInput();
	if (username.empty() || password.empty()) {
		model_.setStatus(L"用户名或密码不能为空");
		return;
	}
	const nlohmann::json req = {
		{"code",     static_cast<int>(proto::RequestCode::Register)},
		{"username", username},
		{"password", password}
	};
	if (!proto::sendJson(*socket_, req)) {
		model_.setStatus(L"发送注册请求失败，连接可能已断开");
		return;
	}
	model_.setStatus(L"正在注册，请稍候...");
}

void ClientController::handleEvent(const sf::Event& event) {
	if (event.is<sf::Event::Closed>()) {
		window_.close();
		return;
	}
	// 鼠标左键释放：交由 View 命中测试决定动作
	if (event.is<sf::Event::MouseButtonReleased>()) {
		const auto* mb = event.getIf<sf::Event::MouseButtonReleased>();
		if (mb && mb->button == sf::Mouse::Button::Left) {
			const sf::Vector2f worldPos{ static_cast<float>(mb->position.x),
										 static_cast<float>(mb->position.y) };
			const auto action = view_.handleClick(worldPos, model_);
			switch (action.type) {
				case ClientView::ClickAction::AddToCart:
					model_.addToCart(action.arg, 1);
					model_.setStatus(L"已加入购物车");
					break;
				case ClientView::ClickAction::RemoveFromCart:
					model_.removeFromCart(action.arg);
					model_.setStatus(L"已从购物车移除");
					break;
				case ClientView::ClickAction::Checkout:
					requestCheckout();
					break;
				case ClientView::ClickAction::ReturnItem:
					requestAfterSale(action.orderId, action.productId, action.qty);
					break;
				case ClientView::ClickAction::SwitchPanel: {
						const int idx = action.arg;
						const auto target = static_cast<ClientView::Panel>(idx);
						// 允许切到 MerchantCreate（新增商品表单）或 [ProductList, MyOrders] 三面板
						if (target == ClientView::Panel::MerchantCreate) {
							view_.clearInputs();
							view_.setActiveField(ClientView::Field::ProductName);
							view_.setPanel(target);
							model_.setStatus(L"新增商品：填写表单后点提交");
						}
						else if (idx >= static_cast<int>(ClientView::Panel::ProductList)
							&& idx <= static_cast<int>(ClientView::Panel::MyOrders)) {
							view_.setPanel(target);
							// 进入商品列表/订单列表时自动拉取最新数据
							if (target == ClientView::Panel::ProductList) {
								requestProductList();
							}
							else if (target == ClientView::Panel::MyOrders) {
								view_.resetMyOrdersScroll();  // 切回时重置滚动到顶
								requestListOrders();
							}
						}
						break;
					}
				case ClientView::ClickAction::FocusField:
					view_.setActiveField(static_cast<ClientView::Field>(action.arg));
					break;
				case ClientView::ClickAction::Login:
					requestLogin();
					break;
				case ClientView::ClickAction::Register:
					requestRegister();
					break;
				case ClientView::ClickAction::Logout:
					// 清用户身份 + 购物车 + 订单缓存，切回登录面板
					model_.clearUser();
					model_.clearCart();
					model_.clearOrders();
					view_.clearInputs();
					view_.setPanel(ClientView::Panel::Login);
					model_.setStatus(L"已登出，请重新登录");
					break;
				case ClientView::ClickAction::MerchantSetOnSale:
					requestMerchantSetOnSale(action.productId, action.arg != 0);
					break;
				case ClientView::ClickAction::MerchantStockPlus:
					requestMerchantUpdateStock(action.productId, +10);
					break;
				case ClientView::ClickAction::MerchantStockMinus:
					requestMerchantUpdateStock(action.productId, -10);
					break;
				case ClientView::ClickAction::MerchantCreateProduct:
					requestMerchantCreateProduct();
					break;
				case ClientView::ClickAction::MerchantCreateBack:
					// 返回商家管理面板，清空表单输入
					view_.clearInputs();
					view_.setPanel(ClientView::Panel::Merchant);
					model_.setStatus(L"已返回商家管理面板");
					break;
				case ClientView::ClickAction::None:
				default: break;
			}
		}
		return;
	}
	// SFML 3 中按键事件类型为 KeyPressed
	if (event.is<sf::Event::KeyPressed>()) {
		const auto* kp = event.getIf<sf::Event::KeyPressed>();
		if (kp == nullptr) return;
		const auto key = kp->code;
		if (key == sf::Keyboard::Key::Escape) {
			window_.close();
		}
		// 登录面板：Enter 等同点击登录按钮，Backspace 删除末尾字符
		if (view_.panel() == ClientView::Panel::Login) {
			if (key == sf::Keyboard::Key::Enter) {
				requestLogin();
			}
			else if (key == sf::Keyboard::Key::Backspace) {
				view_.backspaceInput();
			}
			else if (key == sf::Keyboard::Key::Tab) {
				// Tab 在用户名/密码输入框之间切换
				view_.setActiveField(view_.activeField() == ClientView::Field::Username
					? ClientView::Field::Password
					: ClientView::Field::Username);
			}
		}
		else if (view_.panel() == ClientView::Panel::MerchantCreate) {
			if (key == sf::Keyboard::Key::Enter) {
				requestMerchantCreateProduct();
			}
			else if (key == sf::Keyboard::Key::Backspace) {
				view_.backspaceInput();
			}
			else if (key == sf::Keyboard::Key::Tab) {
				// Tab 在 5 个输入框之间循环切换
				const auto f = view_.activeField();
				const int next = (static_cast<int>(f) + 1 - static_cast<int>(ClientView::Field::ProductName)) % 5
					+ static_cast<int>(ClientView::Field::ProductName);
				view_.setActiveField(static_cast<ClientView::Field>(next));
			}
		}
		return;
	}
	// 文本输入事件：Login/MerchantCreate 面板接收字符（ASCII 或中文 CJK）到当前聚焦输入框
	if (event.is<sf::Event::TextEntered>()) {
		const auto* te = event.getIf<sf::Event::TextEntered>();
		if (te == nullptr) return;
		// 仅 Login 和 MerchantCreate 两个面板接收文本输入
		if (view_.panel() != ClientView::Panel::Login
			&& view_.panel() != ClientView::Panel::MerchantCreate) return;
		// 传 UTF-32 码点；appendInputChar 内部按 ASCII/CJK 过滤并转 UTF-8
		view_.appendInputChar(te->unicode);
		return;
	}
	// 鼠标滚轮：仅在"我的订单"面板内滚动订单列表
	if (event.is<sf::Event::MouseWheelScrolled>()) {
			const auto* mw = event.getIf<sf::Event::MouseWheelScrolled>();
			if (mw == nullptr) return;
			// SFML 3：delta>0 表示向上滚（向前），内容应向下滚动 → 偏移减小
			// 每滚一格约 80 像素，便于快速浏览
			constexpr float kScrollStep = 80.f;
			if (view_.panel() == ClientView::Panel::MyOrders) {
				view_.scrollMyOrders(-mw->delta * kScrollStep, model_);
			}
			else if (view_.panel() == ClientView::Panel::ProductList) {
				view_.scrollProductList(-mw->delta * kScrollStep, model_);
			}
		}
	}

void ClientController::update() {
	std::queue<nlohmann::json> local;
	{
		std::lock_guard<std::mutex> lk(msgMtx_);
		local.swap(pendingMsgs_);
	}
	while (!local.empty()) {
		auto msg = std::move(local.front());
		local.pop();
		processMessage(msg);
	}
}

void ClientController::recvLoop() {
	while (running_.load() && socket_) {
		auto msg = proto::recvJson(*socket_);
		if (!msg) {
			// 连接断开
			std::lock_guard<std::mutex> lk(msgMtx_);
			pendingMsgs_.push(nlohmann::json{
				{"code", static_cast<int>(proto::ResponseCode::Error)},
				{"message", "连接已断开"}
							  });
			break;
		}
		{
			std::lock_guard<std::mutex> lk(msgMtx_);
			pendingMsgs_.push(std::move(*msg));
		}
	}
}

void ClientController::processMessage(nlohmann::json& msg) {
	const auto code = msg.value("code", 0);
	switch (code) {
		case static_cast<int>(proto::ResponseCode::ProductList): {
			std::vector<Product> products;
			if (msg.contains("products") && msg["products"].is_array()) {
				for (const auto& pj : msg["products"]) {
					products.push_back(Product::fromJson(pj));
				}
			}
			model_.setProducts(std::move(products));
			std::wostringstream ss;
			ss << L"已加载 " << model_.products().size() << L" 件商品（点击「商品列表」Tab 可重新拉取）";
			model_.setStatus(ss.str());
			break;
		}
		case static_cast<int>(proto::ResponseCode::CheckoutResult): {
			const auto success = msg.value("success", false);
			if (success) {
				const auto orderId = msg.value("orderId", std::int64_t{});
				const auto originalTotal = msg.value("originalTotal", 0.0);
				const auto discount = msg.value("discount", 0.0);
				const auto finalTotal = msg.value("total", 0.0);  // 兼容字段名
				std::wostringstream ss;
				ss << L"结算成功！订单 #" << orderId
					<< L"  原价 ¥" << originalTotal
					<< L"  促销折扣 -¥" << discount
					<< L"  实付 ¥" << finalTotal;
				model_.setStatus(ss.str());
				model_.clearCart();
				// 结算成功后切回商品列表，便于看到库存变化
				view_.setPanel(ClientView::Panel::ProductList);
				requestProductList();
			}
			else {
				const auto m = msg.value("message", std::string{ "未知错误" });
				model_.setStatus(L"结算失败：" + ec::string::to_utf16(m));
			}
			break;
		}
		case static_cast<int>(proto::ResponseCode::OrderList): {
			std::vector<Order> orders;
			if (msg.contains("orders") && msg["orders"].is_array()) {
				for (const auto& oj : msg["orders"]) {
					orders.push_back(Order::fromJson(oj));
				}
			}
			model_.setOrders(std::move(orders));
			std::wostringstream ss;
			ss << L"已加载 " << model_.orders().size() << L" 条历史订单";
			model_.setStatus(ss.str());
			break;
		}
		case static_cast<int>(proto::ResponseCode::AfterSaleResult): {
			const auto success = msg.value("success", false);
			if (success) {
				const auto refund = msg.value("refund", 0.0);
				std::wostringstream ss;
				ss << L"退货成功！退款 ¥" << refund << L"，已回库存";
				model_.setStatus(ss.str());
				// 刷新订单列表，看到 returnedQty + status 更新
				requestListOrders();
			}
			else {
				const auto m = msg.value("message", std::string{ "未知错误" });
				model_.setStatus(L"退货失败：" + ec::string::to_utf16(m));
			}
			break;
		}
		case static_cast<int>(proto::ResponseCode::Error): {
			const auto m = msg.value("message", std::string{ "未知错误" });
			model_.setStatus(L"错误：" + ec::string::to_utf16(m));
			break;
		}
		case static_cast<int>(proto::ResponseCode::LoginResult): {
			const auto success = msg.value("success", false);
			if (success) {
				const auto userId   = msg["user"].value("id",       std::int64_t{});
				const auto username = msg["user"].value("username", std::string{});
				const auto role     = msg["user"].value("role",     std::int32_t{});
				model_.setUser(userId, username, role);
				view_.clearInputs();
				// 按角色分流：商家进商家面板，普通用户进商品列表
				if (model_.isMerchant()) {
					view_.setPanel(ClientView::Panel::Merchant);
					requestMerchantListProducts();
				}
				else {
					view_.setPanel(ClientView::Panel::ProductList);
					requestProductList();
				}
				std::wostringstream ss;
				ss << L"欢迎 " << ec::string::to_utf16(username)
					<< (model_.isMerchant() ? L"（商家）" : L"") << L"，已登录";
				model_.setStatus(ss.str());
			}
			else {
				const auto m = msg.value("message", std::string{ "登录失败" });
				model_.setStatus(ec::string::to_utf16(m));
			}
			break;
		}
		case static_cast<int>(proto::ResponseCode::RegisterResult): {
			const auto success = msg.value("success", false);
			if (success) {
				const auto userId   = msg["user"].value("id",       std::int64_t{});
				const auto username = msg["user"].value("username", std::string{});
				const auto role     = msg["user"].value("role",     std::int32_t{});
				model_.setUser(userId, username, role);
				view_.clearInputs();
				// 新注册用户默认普通用户，进商品列表
				view_.setPanel(ClientView::Panel::ProductList);
				requestProductList();
				std::wostringstream ss;
				ss << L"注册成功，已自动登录为 " << ec::string::to_utf16(username);
				model_.setStatus(ss.str());
			}
			else {
				const auto m = msg.value("message", std::string{ "注册失败" });
				model_.setStatus(ec::string::to_utf16(m));
			}
			break;
		}
		case static_cast<int>(proto::ResponseCode::MerchantProductList): {
			std::vector<Product> products;
			if (msg.contains("products") && msg["products"].is_array()) {
				for (const auto& pj : msg["products"]) {
					products.push_back(Product::fromJson(pj));
				}
			}
			model_.setProducts(std::move(products));
			std::wostringstream ss;
			ss << L"商家商品列表已加载 " << model_.products().size() << L" 件";
			model_.setStatus(ss.str());
			break;
		}
		case static_cast<int>(proto::ResponseCode::MerchantActionResult): {
			const auto success = msg.value("success", false);
			const auto m = msg.value("message", std::string{});
			model_.setStatus(ec::string::to_utf16(m));
			if (success) {
				// 如果当前在 MerchantCreate 面板，说明是新增商品成功 → 清表单 + 切回 Merchant
				if (view_.panel() == ClientView::Panel::MerchantCreate) {
					view_.clearInputs();
					view_.setPanel(ClientView::Panel::Merchant);
				}
				// 刷新商家商品列表，看到最新上下架状态/库存/新增的商品
				requestMerchantListProducts();
			}
			break;
		}

		default:
			model_.setStatus(L"收到未知响应码：" + std::to_wstring(code));
	}
}
