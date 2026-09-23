#include "../header/ClientView.h"
#include "../header/utils.h"
#include <algorithm>
#include <sstream>
#include <string>

ClientView::ClientView(sf::RenderWindow& window, TextManager& text, ImageManager& image)
	: window_(window), textMgr_(text), imageMgr_(image) {}

namespace {
	// === 商品列表面板布局 ===
	constexpr float cardW = 580.f, cardH = 380.f;
	constexpr float gapX = 10.f, gapY = 10.f;
	constexpr float startX = 120.f, startY = 80.f;
	constexpr int   perRow = 3;
	// 加购按钮：卡片右下角
	constexpr float addBtnDX = 460.f, addBtnDY = 340.f;
	constexpr float addBtnW = 100.f, addBtnH = 30.f;

	// === 购物车面板布局 ===
	constexpr float cartRowX = 200.f, cartStartY = 150.f, cartRowH = 70.f;
	constexpr float cartNameX = 200.f;
	constexpr float cartQtyX = 620.f;
	constexpr float cartPriceX = 780.f;
	constexpr float cartSubX = 940.f;
	// 删除按钮：行内右侧
	constexpr float rmBtnDX = 1120.f, rmBtnDY = 15.f;
	constexpr float rmBtnW = 80.f, rmBtnH = 40.f;
	// 结算按钮 + 总额
	constexpr float checkoutBtnX = 1150.f, checkoutBtnY = 890.f;
	constexpr float checkoutBtnW = 200.f, checkoutBtnH = 60.f;
	constexpr float cartTotalY = 900.f;

	// === 我的订单面板布局 ===
	constexpr float orderCardX = 200.f, orderCardStartY = 150.f;
	constexpr float orderCardW = 1500.f;
	constexpr float orderItemRowH = 36.f;
	// 退货按钮：明细行右端
	constexpr float retBtnDX = 1500.f, retBtnDY = 4.f;
	constexpr float retBtnW = 100.f, retBtnH = 28.f;

	// === 顶部 Tab 标签栏（所有面板共用）===
	constexpr float tabY = 8.f, tabH = 40.f;
	constexpr float tabW = 200.f, tabGap = 10.f;
	constexpr float tabStartX = 20.f;
	constexpr float statusY = 56.f;  // Tab 下方状态信息行

	// === 登录面板布局（窗口居中，窗口默认 1700x1000）===
	constexpr float loginFieldX = 600.f;
	constexpr float loginFieldW = 500.f;
	constexpr float loginFieldH = 50.f;
	constexpr float usernameY = 300.f;
	constexpr float passwordY = 380.f;
	constexpr float authBtnY = 480.f;
	constexpr float authBtnW = 200.f;
	constexpr float authBtnH = 50.f;
	constexpr float loginBtnX = 600.f;
	constexpr float registerBtnX = 850.f;
	constexpr float loginTitleY = 200.f;
}

// 商品名超长截断显示：中文算 2 宽度单位，ASCII 算 1，累计超过 maxWidthUnits 截断加"…"
// 30 单位 ≈ 15 个中文字符或 30 个 ASCII 字符（18px 字体下约 270 像素）
static std::wstring truncateForDisplay(const std::string& utf8, int maxWidthUnits) {
	std::wstring ws = ec::string::to_utf16(utf8);
	int width = 0;
	std::wstring result;
	for (wchar_t c : ws) {
		const int cw = (c >= 0x4E00 && c <= 0x9FFF) ? 2 : 1;
		if (width + cw > maxWidthUnits) {
			result += L"…";
			break;
		}
		result += c;
		width += cw;
	}
	return result;
}

void ClientView::render(const ClientModel& model) {
	switch (panel_) {
		case Panel::Login:           drawLoginPanel(model);           break;
		case Panel::Merchant:        drawMerchantPanel(model);         break;
		case Panel::MerchantCreate:  drawMerchantCreatePanel(model);  break;
		case Panel::ProductList:     drawProductListPanel(model);     break;
		case Panel::Cart:             drawCartPanel(model);            break;
		case Panel::MyOrders:         drawMyOrdersPanel(model);        break;
	}
}

// ===================== 登录面板 =====================

void ClientView::drawLoginPanel(const ClientModel& model) {
	// 标题
	textMgr_.displayTextInUp(L"电商系统 - 用户登录", { 20, 40 }, sf::Color(40, 80, 160));

	// 用户名输入框
	const auto userRect = inputFieldRect(0);
	sf::RectangleShape userBg({ userRect.size.x, userRect.size.y });
	userBg.setPosition({ userRect.position.x, userRect.position.y });
	userBg.setFillColor(sf::Color::White);
	userBg.setOutlineColor(activeField_ == Field::Username ? sf::Color(80, 130, 200) : sf::Color(200, 200, 200));
	userBg.setOutlineThickness(activeField_ == Field::Username ? 2.f : 1.f);
	window_.draw(userBg);
	textMgr_.displayText(L"用户名：", { userRect.position.x - 100, userRect.position.y + 10 }, { 18, 24 }, sf::Color(80, 80, 80));
	textMgr_.displayText(ec::string::to_utf16(usernameInput_),
						 { userRect.position.x + 12, userRect.position.y + 12 }, { 20, 26 }, sf::Color::Black);

	// 密码输入框（显示成 '*'）
	const auto pwRect = inputFieldRect(1);
	sf::RectangleShape pwBg({ pwRect.size.x, pwRect.size.y });
	pwBg.setPosition({ pwRect.position.x, pwRect.position.y });
	pwBg.setFillColor(sf::Color::White);
	pwBg.setOutlineColor(activeField_ == Field::Password ? sf::Color(80, 130, 200) : sf::Color(200, 200, 200));
	pwBg.setOutlineThickness(activeField_ == Field::Password ? 2.f : 1.f);
	window_.draw(pwBg);
	textMgr_.displayText(L"密码：", { pwRect.position.x - 100, pwRect.position.y + 10 }, { 18, 24 }, sf::Color(80, 80, 80));
	textMgr_.displayText(std::wstring(passwordInput_.size(), L'*'),
						 { pwRect.position.x + 12, pwRect.position.y + 12 }, { 20, 26 }, sf::Color::Black);

	// 登录按钮（蓝色）
	const auto loginRect = authBtnRect(0);
	sf::RectangleShape loginBtn({ loginRect.size.x, loginRect.size.y });
	loginBtn.setPosition({ loginRect.position.x, loginRect.position.y });
	loginBtn.setFillColor(sf::Color(80, 130, 200));
	loginBtn.setOutlineColor(sf::Color(60, 100, 170));
	loginBtn.setOutlineThickness(1.f);
	window_.draw(loginBtn);
	textMgr_.displayText(L"登录", { loginRect.position.x + 70, loginRect.position.y + 12 }, { 22, 28 }, sf::Color::White);

	// 注册按钮（灰色）
	const auto regRect = authBtnRect(1);
	sf::RectangleShape regBtn({ regRect.size.x, regRect.size.y });
	regBtn.setPosition({ regRect.position.x, regRect.position.y });
	regBtn.setFillColor(sf::Color(140, 140, 140));
	regBtn.setOutlineColor(sf::Color(110, 110, 110));
	regBtn.setOutlineThickness(1.f);
	window_.draw(regBtn);
	textMgr_.displayText(L"注册", { regRect.position.x + 70, regRect.position.y + 12 }, { 22, 28 }, sf::Color::White);

	// 状态信息
	if (!model.status().empty()) {
		textMgr_.displayText(model.status(), { 600, 580 }, { 20, 26 }, sf::Color(200, 50, 50));
	}

	// 提示文字
	textMgr_.displayText(L"（提示：点击输入框切换聚焦，Enter 键等同登录）",
						 { 600, 620 }, { 16, 22 }, sf::Color(150, 150, 150));
}

// ===================== 商家管理面板 =====================

void ClientView::drawMerchantPanel(const ClientModel& model) {
	// 顶部标题栏 + 用户名 + 登出按钮（复用 drawTabBar 的登出逻辑，但不画 Tab）
	textMgr_.displayTextInUp(L"商家管理 - 商品上下架与库存调整", { 20, 12 }, sf::Color(40, 80, 160));

	// 右上角用户名 + 登出
	if (model.loggedIn()) {
		const auto lr = logoutBtnRect();
		std::wostringstream user;
		user << L"商家：" << ec::string::to_utf16(model.currentUsername());
		textMgr_.displayText(user.str(),
							 { lr.position.x - 220, lr.position.y + 8 },
							 { 18, 24 }, sf::Color(60, 60, 60));
		sf::RectangleShape btn({ lr.size.x, lr.size.y });
		btn.setPosition({ lr.position.x, lr.position.y });
		btn.setFillColor(sf::Color(200, 80, 80));
		btn.setOutlineColor(sf::Color(160, 60, 60));
		btn.setOutlineThickness(1.f);
		window_.draw(btn);
		textMgr_.displayText(L"登出",
							 { lr.position.x + 30, lr.position.y + 8 },
							 { 18, 24 }, sf::Color::White);
	}

	// 状态信息
	if (!model.status().empty()) {
		textMgr_.displayText(model.status(), { 700, statusY }, { 18, 22 }, sf::Color(150, 150, 150));
	}

	const auto& products = model.products();
	if (products.empty()) {
		textMgr_.displayTextInCenter(L"暂无商品数据", { 20, 30 }, sf::Color(150, 150, 150));
		return;
	}

	// 表头
	const float tableY = orderCardStartY;
	textMgr_.displayText(L"商品名", { orderCardX + 12,  tableY }, { 18, 24 }, sf::Color(120, 120, 120));
	textMgr_.displayText(L"价格",   { orderCardX + 450, tableY }, { 18, 24 }, sf::Color(120, 120, 120));
	textMgr_.displayText(L"库存",   { orderCardX + 600, tableY }, { 18, 24 }, sf::Color(120, 120, 120));
	textMgr_.displayText(L"状态",   { orderCardX + 750, tableY }, { 18, 24 }, sf::Color(120, 120, 120));

	float rowY = tableY + 30.f;
	constexpr float rowH = 44.f;
	for (const auto& p : products) {
		const sf::Vector2f rowPos{ orderCardX, rowY };

		// 行背景
		sf::RectangleShape rowBg({ orderCardW, rowH });
		rowBg.setPosition({ rowPos.x, rowPos.y });
		rowBg.setFillColor(p.onSale ? sf::Color(250, 250, 250) : sf::Color(240, 240, 240));
		rowBg.setOutlineColor(sf::Color(220, 220, 220));
		rowBg.setOutlineThickness(1.f);
		window_.draw(rowBg);

		// 商品名（超长截断显示，防止与价格列重叠）
		textMgr_.displayText(truncateForDisplay(p.name, 45),
							 { rowPos.x + 12, rowPos.y + 12 }, { 18, 24 }, sf::Color::Black);
		// 价格
		std::wostringstream pp; pp << L"¥" << p.price;
		textMgr_.displayText(pp.str(), { rowPos.x + 450, rowPos.y + 12 }, { 18, 24 }, sf::Color(80, 80, 80));
		// 库存
		std::wostringstream ss; ss << p.stock;
		textMgr_.displayText(ss.str(), { rowPos.x + 600, rowPos.y + 12 }, { 18, 24 }, sf::Color::Black);
		// 状态
		textMgr_.displayText(p.onSale ? L"已上架" : L"已下架",
							 { rowPos.x + 750, rowPos.y + 12 }, { 18, 24 },
							 p.onSale ? sf::Color(50, 150, 50) : sf::Color(180, 80, 80));
		// 删除按钮（状态列后面）
		const auto delRect = merchantDeleteBtnRect(rowPos);
		sf::RectangleShape delBtn({ delRect.size.x, delRect.size.y });
		delBtn.setPosition({ delRect.position.x, delRect.position.y });
		delBtn.setFillColor(sf::Color(220, 80, 80));
		delBtn.setOutlineColor(sf::Color(180, 60, 60));
		delBtn.setOutlineThickness(1.f);
		window_.draw(delBtn);
		textMgr_.displayText(L"删除",
							 { delRect.position.x + 14, delRect.position.y + 8 },
							 { 16, 22 }, sf::Color::White);

		// 上架/下架按钮
		const auto saleRect = merchantSaleBtnRect(rowPos);
		sf::RectangleShape saleBtn({ saleRect.size.x, saleRect.size.y });
		saleBtn.setPosition({ saleRect.position.x, saleRect.position.y });
		saleBtn.setFillColor(p.onSale ? sf::Color(180, 80, 80) : sf::Color(50, 150, 50));
		saleBtn.setOutlineColor(p.onSale ? sf::Color(150, 60, 60) : sf::Color(40, 120, 40));
		saleBtn.setOutlineThickness(1.f);
		window_.draw(saleBtn);
		textMgr_.displayText(p.onSale ? L"下架" : L"上架",
							 { saleRect.position.x + 18, saleRect.position.y + 10 },
							 { 16, 22 }, sf::Color::White);

		// 库存 -10 按钮
		const auto minusRect = merchantStockMinusBtnRect(rowPos);
		sf::RectangleShape minusBtn({ minusRect.size.x, minusRect.size.y });
		minusBtn.setPosition({ minusRect.position.x, minusRect.position.y });
		minusBtn.setFillColor(sf::Color(220, 130, 30));
		minusBtn.setOutlineColor(sf::Color(180, 100, 20));
		minusBtn.setOutlineThickness(1.f);
		window_.draw(minusBtn);
		textMgr_.displayText(L"库存-10",
							 { minusRect.position.x + 4, minusRect.position.y + 10 },
							 { 14, 20 }, sf::Color::White);

		// 库存 +10 按钮
		const auto plusRect = merchantStockPlusBtnRect(rowPos);
		sf::RectangleShape plusBtn({ plusRect.size.x, plusRect.size.y });
		plusBtn.setPosition({ plusRect.position.x, plusRect.position.y });
		plusBtn.setFillColor(sf::Color(80, 130, 200));
		plusBtn.setOutlineColor(sf::Color(60, 100, 170));
		plusBtn.setOutlineThickness(1.f);
		window_.draw(plusBtn);
		textMgr_.displayText(L"库存+10",
							 { plusRect.position.x + 4, plusRect.position.y + 10 },
							 { 14, 20 }, sf::Color::White);

		rowY += rowH + 4.f;
	}

	// 底部"新增商品"按钮（固定在窗口底部上方）
	const auto cb = merchantCreateBtnRect();
	sf::RectangleShape createBtn({ cb.size.x, cb.size.y });
	createBtn.setPosition({ cb.position.x, cb.position.y });
	createBtn.setFillColor(sf::Color(60, 140, 80));
	createBtn.setOutlineColor(sf::Color(40, 110, 60));
	createBtn.setOutlineThickness(1.f);
	window_.draw(createBtn);
	textMgr_.displayText(L"+ 新增商品",
						{ cb.position.x + 30, cb.position.y + 10 },
						{ 18, 24 }, sf::Color::White);
}

// ===================== 商家新增商品表单面板 =====================

void ClientView::drawMerchantCreatePanel(const ClientModel& model) {
	textMgr_.displayTextInUp(L"商家管理 - 新增商品", { 20, 12 }, sf::Color(40, 80, 160));

	// 右上角用户名 + 登出（与商家面板一致）
	if (model.loggedIn()) {
		const auto lr = logoutBtnRect();
		std::wostringstream user;
		user << L"商家：" << ec::string::to_utf16(model.currentUsername());
		textMgr_.displayText(user.str(),
							 { lr.position.x - 220, lr.position.y + 8 },
							 { 18, 24 }, sf::Color(60, 60, 60));
		sf::RectangleShape btn({ lr.size.x, lr.size.y });
		btn.setPosition({ lr.position.x, lr.position.y });
		btn.setFillColor(sf::Color(200, 80, 80));
		btn.setOutlineColor(sf::Color(160, 60, 60));
		btn.setOutlineThickness(1.f);
		window_.draw(btn);
		textMgr_.displayText(L"登出",
							 { lr.position.x + 30, lr.position.y + 8 },
							 { 18, 24 }, sf::Color::White);
	}

	// 表单标题
	textMgr_.displayText(L"填写商品信息（带 * 为必填）", { 100, 100 }, { 20, 26 }, sf::Color(60, 60, 60));

	// 5 个输入框：名称/价格/库存/描述/图片路径
	const wchar_t* labels[] = { L"商品名称 *", L"价格 (元) *", L"库存 *", L"描述", L"图片路径" };
	const std::string* values[] = {
		&productNameInput_, &productPriceInput_, &productStockInput_,
		&productDescInput_, &productImageInput_
	};
	const Field fields[] = {
		Field::ProductName, Field::ProductPrice, Field::ProductStock,
		Field::ProductDesc, Field::ProductImage
	};
	for (int i = 0; i < 5; ++i) {
		const auto r = merchantCreateFieldRect(i);
		// 标签
		textMgr_.displayText(labels[i],
							{ r.position.x - 130, r.position.y + 8 },
							{ 18, 24 }, sf::Color(80, 80, 80));
		// 输入框背景
		sf::RectangleShape bg({ r.size.x, r.size.y });
		bg.setPosition({ r.position.x, r.position.y });
		bg.setFillColor(sf::Color::White);
		bg.setOutlineColor(activeField_ == fields[i] ? sf::Color(80, 130, 200) : sf::Color(200, 200, 200));
		bg.setOutlineThickness(activeField_ == fields[i] ? 2.f : 1.f);
		window_.draw(bg);
		// 输入框文本
		std::wstring shown = ec::string::to_utf16(*values[i]);
		if (activeField_ == fields[i]) shown += L"_";  // 光标占位
		textMgr_.displayText(shown,
							{ r.position.x + 8, r.position.y + 8 },
							{ 18, 24 }, sf::Color::Black);
	}

	// 提交 + 返回按钮
	const auto sb = merchantCreateSubmitBtnRect();
	sf::RectangleShape submitBtn({ sb.size.x, sb.size.y });
	submitBtn.setPosition({ sb.position.x, sb.position.y });
	submitBtn.setFillColor(sf::Color(60, 140, 80));
	submitBtn.setOutlineColor(sf::Color(40, 110, 60));
	submitBtn.setOutlineThickness(1.f);
	window_.draw(submitBtn);
	textMgr_.displayText(L"提交新增",
						{ sb.position.x + 30, sb.position.y + 10 },
						{ 18, 24 }, sf::Color::White);

	const auto bb = merchantCreateBackBtnRect();
	sf::RectangleShape backBtn({ bb.size.x, bb.size.y });
	backBtn.setPosition({ bb.position.x, bb.position.y });
	backBtn.setFillColor(sf::Color(150, 150, 150));
	backBtn.setOutlineColor(sf::Color(120, 120, 120));
	backBtn.setOutlineThickness(1.f);
	window_.draw(backBtn);
	textMgr_.displayText(L"返回",
						{ bb.position.x + 30, bb.position.y + 10 },
						{ 18, 24 }, sf::Color::White);

	// 状态信息
	if (!model.status().empty()) {
		textMgr_.displayText(model.status(), { 100, 580 }, { 18, 22 }, sf::Color(200, 50, 50));
	}

	// 提示
	textMgr_.displayText(L"（Tab 键切换输入框，Enter 键等同提交）",
						{ 100, 620 }, { 16, 22 }, sf::Color(150, 150, 150));
}

void ClientView::appendInputChar(std::uint32_t ch) {
	// 只接收可打印字符：ASCII 32..126 或中文 CJK 0x4E00..0x9FFF；其他忽略
	const bool isAscii = (ch >= 32 && ch <= 126);
	const bool isCjk = (ch >= 0x4E00 && ch <= 0x9FFF);
	if (!isAscii && !isCjk) return;
	// UTF-32 → UTF-8
	std::string utf8;
	if (ch < 0x80) {
		utf8.push_back(static_cast<char>(ch));
	} else if (ch < 0x800) {
		utf8.push_back(static_cast<char>(0xC0 | (ch >> 6)));
		utf8.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
	} else if (ch < 0x10000) {
		utf8.push_back(static_cast<char>(0xE0 | (ch >> 12)));
		utf8.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
		utf8.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
	} else {
		utf8.push_back(static_cast<char>(0xF0 | (ch >> 18)));
		utf8.push_back(static_cast<char>(0x80 | ((ch >> 12) & 0x3F)));
		utf8.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
		utf8.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
	}
	// 用字节限制 + UTF-8 字节数共同约束
	const auto append = [this, &utf8](std::string& dst, std::size_t maxBytes) {
		if (dst.size() + utf8.size() <= maxBytes) dst += utf8;
	};
	switch (activeField_) {
		case Field::Username:       append(usernameInput_, 64); break;
		case Field::Password:       append(passwordInput_, 64); break;
		case Field::ProductName:    append(productNameInput_, 128); break;
		case Field::ProductPrice:   append(productPriceInput_, 12); break;
		case Field::ProductStock:   append(productStockInput_, 10); break;
		case Field::ProductDesc:    append(productDescInput_, 256); break;
		case Field::ProductImage:   append(productImageInput_, 256); break;
	}
}

// 从 UTF-8 末尾删掉一个完整字符（1..4 字节），避免把中文删半个
static void popUtf8Char(std::string& s) {
	if (s.empty()) return;
	// 找最后一个字符的起始字节：连续的 0x80..0xBF 是后续字节
	auto it = s.end() - 1;
	while (it != s.begin() && (static_cast<unsigned char>(*it) & 0xC0) == 0x80) {
		--it;
	}
	s.erase(it, s.end());
}

void ClientView::backspaceInput() {
	switch (activeField_) {
		case Field::Username:       popUtf8Char(usernameInput_); break;
		case Field::Password:       popUtf8Char(passwordInput_); break;
		case Field::ProductName:    popUtf8Char(productNameInput_); break;
		case Field::ProductPrice:   popUtf8Char(productPriceInput_); break;
		case Field::ProductStock:   popUtf8Char(productStockInput_); break;
		case Field::ProductDesc:    popUtf8Char(productDescInput_); break;
		case Field::ProductImage:   popUtf8Char(productImageInput_); break;
	}
}

void ClientView::clearInputs() noexcept {
	usernameInput_.clear();
	passwordInput_.clear();
	productNameInput_.clear();
	productPriceInput_.clear();
	productStockInput_.clear();
	productDescInput_.clear();
	productImageInput_.clear();
	activeField_ = Field::Username;
}

// ===================== 顶部 Tab 标签栏 =====================

void ClientView::drawTabBar(const ClientModel& model) {
	const wchar_t* labels[] = { L"商品列表", L"购物车", L"我的订单" };
	// Panel 枚举包含 Login=0，Tab 索引 0/1/2 对应 ProductList/Cart/MyOrders，故减去 Login 偏移
	const int current = static_cast<int>(panel_) - static_cast<int>(Panel::ProductList);
	for (int i = 0; i < 3; ++i) {
		const auto r = tabBtnRect(i);
		sf::RectangleShape bg({ r.size.x, r.size.y });
		bg.setPosition({ r.position.x, r.position.y });
		if (i == current) {
			bg.setFillColor(sf::Color(80, 130, 200));
			bg.setOutlineColor(sf::Color(60, 100, 170));
		}
		else {
			bg.setFillColor(sf::Color(230, 230, 230));
			bg.setOutlineColor(sf::Color(200, 200, 200));
		}
		bg.setOutlineThickness(1.f);
		window_.draw(bg);
		const auto textColor = (i == current) ? sf::Color::White : sf::Color::Black;
		textMgr_.displayText(labels[i],
							 { r.position.x + 50, r.position.y + 8 },
							 { 18, 24 }, textColor);
	}

	// 右上角：当前用户名 + 登出按钮（仅登录后显示）
	if (model.loggedIn()) {
		const auto lr = logoutBtnRect();
		// 用户名文本（登出按钮左侧）
		std::wostringstream user;
		user << L"用户：" << ec::string::to_utf16(model.currentUsername());
		textMgr_.displayText(user.str(),
							 { lr.position.x - 200, lr.position.y + 8 },
							 { 18, 24 }, sf::Color(60, 60, 60));
		// 登出按钮（红色调，区别于蓝色 Tab）
		sf::RectangleShape btn({ lr.size.x, lr.size.y });
		btn.setPosition({ lr.position.x, lr.position.y });
		btn.setFillColor(sf::Color(200, 80, 80));
		btn.setOutlineColor(sf::Color(160, 60, 60));
		btn.setOutlineThickness(1.f);
		window_.draw(btn);
		textMgr_.displayText(L"登出",
							 { lr.position.x + 30, lr.position.y + 8 },
							 { 18, 24 }, sf::Color::White);
	}
}

// ===================== 商品列表面板 =====================

void ClientView::drawProductListPanel(const ClientModel& model) {
	const auto& products = model.products();

	drawTabBar(model);
	if (!model.status().empty()) {
		textMgr_.displayText(model.status(), { 700, statusY }, { 18, 22 }, sf::Color(150, 150, 150));
	}
	if (products.empty()) {
		textMgr_.displayTextInCenter(L"暂无商品，点击顶部 商品列表 标签刷新", { 20, 30 }, sf::Color(150, 150, 150));
		return;
	}

	for (std::size_t i = 0; i < products.size(); ++i) {
		const auto col = static_cast<int>(i % perRow);
		const auto row = static_cast<int>(i / perRow);
		const sf::Vector2f pos{ startX + col * (cardW + gapX),
								startY - productListScrollY_ + row * (cardH + gapY) };
		drawCard(products[i], pos, { cardW, cardH });
	}
}

void ClientView::drawCard(const Product& p, const sf::Vector2f& pos, const sf::Vector2f& size) {
	// 卡片背景
	sf::RectangleShape bg({ size.x, size.y });
	bg.setPosition(pos);
	bg.setFillColor(sf::Color::White);
	bg.setOutlineColor(sf::Color(200, 200, 200));
	bg.setOutlineThickness(1.f);
	window_.draw(bg);

	// 图片：上半部分（高 250）
	const sf::Vector2f imgPos{ pos.x, pos.y };
	const sf::Vector2f imgSize{ size.x, 250.f };
	bool imgOk = false;
	if (!p.imagePath.empty()) {
		try {
			imageMgr_.displayImage(p.imagePath, imgPos, imgSize);
			imgOk = true;
		} catch (const std::exception&) {
			imgOk = false;
		}
	}
	if (!imgOk) {
		sf::RectangleShape placeholder(imgSize);
		placeholder.setPosition(imgPos);
		placeholder.setFillColor(sf::Color(230, 230, 230));
		window_.draw(placeholder);
	}

	// 分隔线
	sf::RectangleShape line({ size.x, 1.f });
	line.setPosition({ pos.x, pos.y + 250.f });
	line.setFillColor(sf::Color(220, 220, 220));
	window_.draw(line);

	// 名称
	const auto name = ec::string::to_utf16(p.name);
	textMgr_.displayText(name, { pos.x + 10, pos.y + 260 }, { 20, 28 }, sf::Color::Black);

	// 描述
	const auto desc = ec::string::to_utf16(p.description);
	textMgr_.displayText(desc, { pos.x + 10, pos.y + 300 }, { 20, 20 }, sf::Color(100, 100, 100));

	// 价格
	std::wostringstream priceStr;
	priceStr << L"¥ " << p.price;
	textMgr_.displayText(priceStr.str(), { pos.x + 10, pos.y + 330 }, { 20, 32 }, sf::Color(200, 50, 50));

	// 库存
	std::wostringstream stockStr;
	stockStr << L"库存：" << p.stock;
	textMgr_.displayText(stockStr.str(), { pos.x + 250, pos.y + 345 }, { 20, 20 }, sf::Color(100, 100, 100));

	// 加购按钮（绿色）
	const auto btnRect = addToCartBtnRect(pos);
	sf::RectangleShape btn({ btnRect.size.x, btnRect.size.y });
	btn.setPosition({ btnRect.position.x, btnRect.position.y });
	btn.setFillColor(sf::Color(80, 160, 80));
	btn.setOutlineColor(sf::Color(60, 130, 60));
	btn.setOutlineThickness(1.f);
	window_.draw(btn);
	textMgr_.displayText(L"+ 加购",
						 { btnRect.position.x + 18, btnRect.position.y + 3 },
						 { 18, 22 }, sf::Color::White);
}

// ===================== 购物车面板 =====================

void ClientView::drawCartPanel(const ClientModel& model) {
	const auto& cart = model.cart();

	drawTabBar(model);
	if (!model.status().empty()) {
		textMgr_.displayText(model.status(), { 700, statusY }, { 18, 22 }, sf::Color(150, 150, 150));
	}

	// 表头
	textMgr_.displayText(L"商品名", { cartNameX, cartStartY - 50 }, { 18, 22 }, sf::Color(80, 80, 80));
	textMgr_.displayText(L"数量", { cartQtyX,  cartStartY - 50 }, { 18, 22 }, sf::Color(80, 80, 80));
	textMgr_.displayText(L"单价", { cartPriceX,cartStartY - 50 }, { 18, 22 }, sf::Color(80, 80, 80));
	textMgr_.displayText(L"小计", { cartSubX,  cartStartY - 50 }, { 18, 22 }, sf::Color(80, 80, 80));

	sf::RectangleShape line({ 1700, 1.f });
	line.setPosition({ cartRowX - 50, cartStartY - 20 });
	line.setFillColor(sf::Color(200, 200, 200));
	window_.draw(line);

	if (cart.empty()) {
		textMgr_.displayTextInCenter(L"购物车为空，去加购商品吧", { 20, 30 }, sf::Color(150, 150, 150));
		return;
	}

	// 购物车项
	for (std::size_t i = 0; i < cart.size(); ++i) {
		const sf::Vector2f rowPos{ cartRowX, cartStartY + i * cartRowH };

		// 隔行变色
		if (i % 2 == 0) {
			sf::RectangleShape rowBg({ 1700, cartRowH });
			rowBg.setPosition(rowPos);
			rowBg.setFillColor(sf::Color(250, 250, 250));
			window_.draw(rowBg);
		}

		const auto& item = cart[i];
		// 商品名
		const auto name = ec::string::to_utf16(item.name);
		textMgr_.displayText(name, { rowPos.x, rowPos.y + 15 }, { 20, 26 }, sf::Color::Black);
		// 数量
		std::wostringstream qtyStr;
		qtyStr << item.qty;
		textMgr_.displayText(qtyStr.str(), { cartQtyX, rowPos.y + 15 }, { 20, 26 }, sf::Color::Black);
		// 单价
		std::wostringstream priceStr;
		priceStr << L"¥" << item.price;
		textMgr_.displayText(priceStr.str(), { cartPriceX, rowPos.y + 15 }, { 20, 26 }, sf::Color(80, 80, 80));
		// 小计
		std::wostringstream subStr;
		subStr << L"¥" << item.subtotal();
		textMgr_.displayText(subStr.str(), { cartSubX, rowPos.y + 15 }, { 20, 26 }, sf::Color(200, 50, 50));

		// 删除按钮
		const auto rmRect = removeFromCartBtnRect(rowPos);
		sf::RectangleShape rmBtn({ rmRect.size.x, rmRect.size.y });
		rmBtn.setPosition({ rmRect.position.x, rmRect.position.y });
		rmBtn.setFillColor(sf::Color(200, 80, 80));
		rmBtn.setOutlineColor(sf::Color(160, 60, 60));
		rmBtn.setOutlineThickness(1.f);
		window_.draw(rmBtn);
		textMgr_.displayText(L"- 删除",
							 { rmRect.position.x + 12, rmRect.position.y + 8 },
							 { 16, 22 }, sf::Color::White);
	}

	// 总额
	std::wostringstream totalStr;
	totalStr << L"总额：¥ " << model.cartTotal();
	textMgr_.displayText(totalStr.str(), { cartSubX - 200, cartTotalY }, { 24, 36 }, sf::Color(200, 50, 50));

	// 结算按钮
	const auto ckRect = checkoutBtnRect();
	sf::RectangleShape ckBtn({ ckRect.size.x, ckRect.size.y });
	ckBtn.setPosition({ ckRect.position.x, ckRect.position.y });
	ckBtn.setFillColor(sf::Color(220, 130, 30));
	ckBtn.setOutlineColor(sf::Color(180, 100, 20));
	ckBtn.setOutlineThickness(1.f);
	window_.draw(ckBtn);
	textMgr_.displayText(L"结算",
						 { ckRect.position.x + 70, ckRect.position.y + 14 },
						 { 24, 32 }, sf::Color::White);
}

// ===================== 我的订单面板 =====================

void ClientView::drawMyOrdersPanel(const ClientModel& model) {
	const auto& orders = model.orders();

	drawTabBar(model);
	if (!model.status().empty()) {
		textMgr_.displayText(model.status(), { 700, statusY }, { 18, 22 }, sf::Color(150, 150, 150));
	}

	if (orders.empty()) {
		textMgr_.displayTextInCenter(L"暂无历史订单，先去结算一单试试", { 20, 30 }, sf::Color(150, 150, 150));
		return;
	}

	// 起始 y，逐订单下移；滚动偏移让内容向上移动
	float y = orderCardStartY - myOrdersScrollY_;
	for (const auto& order : orders) {
		// 订单卡片背景
		const float cardH = 90.f + static_cast<float>(order.items.size()) * orderItemRowH;
		sf::RectangleShape bg({ orderCardW, cardH });
		bg.setPosition({ orderCardX, y });
		bg.setFillColor(sf::Color(250, 250, 250));
		bg.setOutlineColor(sf::Color(200, 200, 200));
		bg.setOutlineThickness(1.f);
		window_.draw(bg);

		// 订单头：ID + 时间 + 状态
		std::wostringstream head;
		head << L"订单 #" << order.id << L"   " << ec::string::to_utf16(order.createdAt);
		switch (order.status) {
			case 1:  head << L"   [部分退货]"; break;
			case 2:  head << L"   [全部退货]"; break;
			default: head << L"   [正常]";    break;
		}
		textMgr_.displayText(head.str(), { orderCardX + 12, y + 8 }, { 18, 24 }, sf::Color::Black);

		// 原价/折扣/实付
		std::wostringstream money;
		money << L"原价 ¥" << order.originalTotal
			<< L"  折扣 -¥" << order.discount
			<< L"  实付 ¥" << order.finalTotal;
		textMgr_.displayText(money.str(), { orderCardX + 12, y + 38 }, { 18, 22 }, sf::Color(80, 80, 80));

		// 明细表头
		float itemY = y + 70.f;
		textMgr_.displayText(L"商品", { orderCardX + 12,  itemY }, { 16, 18 }, sf::Color(120, 120, 120));
		textMgr_.displayText(L"单价", { orderCardX + 400, itemY }, { 16, 18 }, sf::Color(120, 120, 120));
		textMgr_.displayText(L"购买", { orderCardX + 600, itemY }, { 16, 18 }, sf::Color(120, 120, 120));
		textMgr_.displayText(L"已退", { orderCardX + 750, itemY }, { 16, 18 }, sf::Color(120, 120, 120));

		itemY += 20.f;
		for (const auto& it : order.items) {
			const sf::Vector2f rowPos{ orderCardX, itemY };
			// 仅当该明细还有可退数量（status≠2 且 qty>returnedQty）时绘制退货按钮
			const std::int32_t returnable = it.qty - it.returnedQty;
			if (order.status != 2 && returnable > 0) {
				const auto r = returnBtnRect(rowPos);
				sf::RectangleShape btn({ r.size.x, r.size.y });
				btn.setPosition({ r.position.x, r.position.y });
				btn.setFillColor(sf::Color(220, 130, 30));
				btn.setOutlineColor(sf::Color(180, 100, 20));
				btn.setOutlineThickness(1.f);
				window_.draw(btn);
				textMgr_.displayText(L"退货 1 件",
									 { r.position.x + 6, r.position.y + 4 },
									 { 14, 18 }, sf::Color::White);
			}
			// 文本
			textMgr_.displayText(ec::string::to_utf16(it.name),
								 { orderCardX + 12,  itemY + 4 }, { 16, 22 }, sf::Color::Black);
			std::wostringstream pp; pp << L"¥" << it.price;
			textMgr_.displayText(pp.str(), { orderCardX + 400, itemY + 4 }, { 16, 22 }, sf::Color(80, 80, 80));
			std::wostringstream qq; qq << it.qty;
			textMgr_.displayText(qq.str(), { orderCardX + 600, itemY + 4 }, { 16, 22 }, sf::Color::Black);
			std::wostringstream rq; rq << it.returnedQty;
			textMgr_.displayText(rq.str(), { orderCardX + 750, itemY + 4 }, { 16, 22 }, sf::Color(200, 100, 100));
			itemY += orderItemRowH;
		}
		// 下一张订单间距
		y = itemY + 12.f;
	}
}

// ===================== 鼠标命中测试 =====================

ClientView::ClickAction ClientView::handleClick(const sf::Vector2f& mousePos, const ClientModel& model) {
	auto hit = [](const sf::FloatRect& r, const sf::Vector2f& p) {
		return p.x >= r.position.x && p.x < r.position.x + r.size.x
			&& p.y >= r.position.y && p.y < r.position.y + r.size.y;
	};

	// 登录面板独立处理，不参与顶部 Tab 切换
	if (panel_ == Panel::Login) {
		if (hit(inputFieldRect(0), mousePos)) {
			return { ClickAction::FocusField, static_cast<std::int32_t>(Field::Username) };
		}
		if (hit(inputFieldRect(1), mousePos)) {
			return { ClickAction::FocusField, static_cast<std::int32_t>(Field::Password) };
		}
		if (hit(authBtnRect(0), mousePos)) {
			return { ClickAction::Login, 0 };
		}
		if (hit(authBtnRect(1), mousePos)) {
			return { ClickAction::Register, 0 };
		}
		return { ClickAction::None, 0 };
	}

	// 顶部 Tab 标签优先（ProductList/Cart/MyOrders 三面板共用）
	// i=0 对应 ProductList；arg 用 Panel 枚举值便于 controller 直接 static_cast
	for (int i = 0; i < 3; ++i) {
		if (hit(tabBtnRect(i), mousePos)) {
			const int panelIdx = static_cast<int>(Panel::ProductList) + i;
			return { ClickAction::SwitchPanel, panelIdx };
		}
	}
	// 右上角登出按钮（登录后才显示）
	if (model.loggedIn() && hit(logoutBtnRect(), mousePos)) {
		return { ClickAction::Logout, 0 };
	}

	if (panel_ == Panel::ProductList) {
		const auto& products = model.products();
		for (std::size_t i = 0; i < products.size(); ++i) {
			const auto col = static_cast<int>(i % perRow);
			const auto row = static_cast<int>(i / perRow);
			const sf::Vector2f cardPos{ startX + col * (cardW + gapX),
										startY - productListScrollY_ + row * (cardH + gapY) };
			if (hit(addToCartBtnRect(cardPos), mousePos)) {
				return { ClickAction::AddToCart, products[i].id };
			}
		}
	}
	else if (panel_ == Panel::Cart) {
		const auto& cart = model.cart();
		for (std::size_t i = 0; i < cart.size(); ++i) {
			const sf::Vector2f rowPos{ cartRowX, cartStartY + i * cartRowH };
			if (hit(removeFromCartBtnRect(rowPos), mousePos)) {
				return { ClickAction::RemoveFromCart, cart[i].productId };
			}
		}
		if (hit(checkoutBtnRect(), mousePos)) {
			return { ClickAction::Checkout, 0 };
		}
	}
	else if (panel_ == Panel::MyOrders) {
		// 复刻 drawMyOrdersPanel 的布局：从 orderCardStartY - 滚动偏移 起逐订单逐明细下移
		float y = orderCardStartY - myOrdersScrollY_;
		for (const auto& order : model.orders()) {
			const float itemY0 = y + 70.f + 20.f;  // 头 + 表头
			float itemY = itemY0;
			for (const auto& it : order.items) {
				const sf::Vector2f rowPos{ orderCardX, itemY };
				const std::int32_t returnable = it.qty - it.returnedQty;
				if (order.status != 2 && returnable > 0) {
					if (hit(returnBtnRect(rowPos), mousePos)) {
						ClickAction act{ ClickAction::ReturnItem, 0 };
						act.orderId = order.id;
						act.productId = it.productId;
						act.qty = 1;  // 暂固定 1 件，UI 可扩展数量输入框
						return act;
					}
				}
				itemY += orderItemRowH;
			}
			// 下一张订单起点：itemY + 间距（与 drawMyOrdersPanel 同步）
			y = itemY + 12.f;
		}
	}
	else if (panel_ == Panel::Merchant) {
		// 复刻 drawMerchantPanel：tableY + 30 起，每行 rowH+4
		constexpr float rowH = 44.f;
		float rowY = orderCardStartY + 30.f;
		for (const auto& p : model.products()) {
			const sf::Vector2f rowPos{ orderCardX, rowY };
			if (hit(merchantSaleBtnRect(rowPos), mousePos)) {
				ClickAction act{ ClickAction::MerchantSetOnSale, p.onSale ? 0 : 1 };
				act.productId = p.id;
				return act;
			}
			if (hit(merchantStockMinusBtnRect(rowPos), mousePos)) {
				ClickAction act{ ClickAction::MerchantStockMinus, 0 };
				act.productId = p.id;
				return act;
			}
			if (hit(merchantStockPlusBtnRect(rowPos), mousePos)) {
				ClickAction act{ ClickAction::MerchantStockPlus, 0 };
				act.productId = p.id;
				return act;
			}
			if (hit(merchantDeleteBtnRect(rowPos), mousePos)) {
				ClickAction act{ ClickAction::MerchantDeleteProduct, 0 };
				act.productId = p.id;
				return act;
			}
			rowY += rowH + 4.f;
		}
		// 底部"新增商品"按钮
		if (hit(merchantCreateBtnRect(), mousePos)) {
			return { ClickAction::SwitchPanel, static_cast<int>(Panel::MerchantCreate) };
		}
	}
	else if (panel_ == Panel::MerchantCreate) {
		// 5 个输入框命中 → 聚焦切换
		for (int i = 0; i < 5; ++i) {
			if (hit(merchantCreateFieldRect(i), mousePos)) {
				ClickAction act{ ClickAction::FocusField, static_cast<int>(Field::ProductName) + i };
				return act;
			}
		}
		if (hit(merchantCreateSubmitBtnRect(), mousePos)) {
			return { ClickAction::MerchantCreateProduct, 0 };
		}
		if (hit(merchantCreateBackBtnRect(), mousePos)) {
			return { ClickAction::MerchantCreateBack, 0 };
		}
	}
	return { ClickAction::None, 0 };
}

sf::FloatRect ClientView::addToCartBtnRect(const sf::Vector2f& cardPos) {
	return sf::FloatRect(sf::Vector2f{ cardPos.x + addBtnDX, cardPos.y + addBtnDY },
						 sf::Vector2f{ addBtnW, addBtnH });
}

sf::FloatRect ClientView::removeFromCartBtnRect(const sf::Vector2f& rowPos) {
	return sf::FloatRect(sf::Vector2f{ rmBtnDX, rowPos.y + rmBtnDY },
						 sf::Vector2f{ rmBtnW, rmBtnH });
}

sf::FloatRect ClientView::checkoutBtnRect() {
	return sf::FloatRect(sf::Vector2f{ checkoutBtnX, checkoutBtnY },
						 sf::Vector2f{ checkoutBtnW, checkoutBtnH });
}

sf::FloatRect ClientView::returnBtnRect(const sf::Vector2f& rowPos) {
	return sf::FloatRect(sf::Vector2f{ rowPos.x + retBtnDX, rowPos.y + retBtnDY },
						 sf::Vector2f{ retBtnW, retBtnH });
}

sf::FloatRect ClientView::tabBtnRect(int index) {
	return sf::FloatRect(sf::Vector2f{ tabStartX + index * (tabW + tabGap), tabY },
						 sf::Vector2f{ tabW, tabH });
}

sf::FloatRect ClientView::inputFieldRect(int field) {
	const float y = (field == 0) ? usernameY : passwordY;
	return sf::FloatRect(sf::Vector2f{ loginFieldX, y },
						 sf::Vector2f{ loginFieldW, loginFieldH });
}

sf::FloatRect ClientView::authBtnRect(int btn) {
	const float x = (btn == 0) ? loginBtnX : registerBtnX;
	return sf::FloatRect(sf::Vector2f{ x, authBtnY },
						 sf::Vector2f{ authBtnW, authBtnH });
}

sf::FloatRect ClientView::logoutBtnRect() const {
	// 右上角：距离窗口右边 20px，与 Tab 同高
	constexpr float w = 100.f, h = tabH;
	const float x = static_cast<float>(window_.getSize().x) - w - 20.f;
	return sf::FloatRect(sf::Vector2f{ x, tabY }, sf::Vector2f{ w, h });
}

// 商家面板行内按钮：从右往左依次是 上架/下架、库存-10、库存+10
// 按钮从右往左排列：删除(60) → 上架/下架(70) → 库存-10(80) → 库存+10(80)
sf::FloatRect ClientView::merchantDeleteBtnRect(const sf::Vector2f& rowPos) const {
	constexpr float w = 60.f, h = 30.f;
	const float x = rowPos.x + orderCardW - w - 10.f;
	return sf::FloatRect(sf::Vector2f{ x, rowPos.y + 7.f }, sf::Vector2f{ w, h });
}

sf::FloatRect ClientView::merchantSaleBtnRect(const sf::Vector2f& rowPos) const {
	constexpr float w = 70.f, h = 30.f;
	const float deleteX = rowPos.x + orderCardW - 60.f - 10.f;  // 删除按钮左边
	const float x = deleteX - w - 8.f;
	return sf::FloatRect(sf::Vector2f{ x, rowPos.y + 7.f }, sf::Vector2f{ w, h });
}

sf::FloatRect ClientView::merchantStockMinusBtnRect(const sf::Vector2f& rowPos) const {
	constexpr float w = 80.f, h = 30.f;
	const float saleX = rowPos.x + orderCardW - 60.f - 10.f - 70.f - 8.f;  // 上架/下架按钮左边
	const float x = saleX - w - 8.f;
	return sf::FloatRect(sf::Vector2f{ x, rowPos.y + 7.f }, sf::Vector2f{ w, h });
}

sf::FloatRect ClientView::merchantStockPlusBtnRect(const sf::Vector2f& rowPos) const {
	constexpr float w = 80.f, h = 30.f;
	const float minusX = rowPos.x + orderCardW - 60.f - 10.f - 70.f - 8.f - 80.f - 8.f;
	const float x = minusX - w - 8.f;
	return sf::FloatRect(sf::Vector2f{ x, rowPos.y + 7.f }, sf::Vector2f{ w, h });
}

// 商家面板底部"新增商品"按钮（左下角）
sf::FloatRect ClientView::merchantCreateBtnRect() const {
	constexpr float w = 160.f, h = 40.f;
	const float x = orderCardX;
	const float y = static_cast<float>(window_.getSize().y) - h - 20.f;
	return sf::FloatRect(sf::Vector2f{ x, y }, sf::Vector2f{ w, h });
}

// 商家新增商品表单输入框矩形；field 取 0..4 对应 Name/Price/Stock/Desc/Image
sf::FloatRect ClientView::merchantCreateFieldRect(int field) {
	constexpr float w = 500.f, h = 40.f;
	constexpr float startX = 250.f;
	constexpr float startY = 150.f;
	constexpr float gap = 50.f;
	const float y = startY + field * gap;
	return sf::FloatRect(sf::Vector2f{ startX, y }, sf::Vector2f{ w, h });
}

// 提交按钮（表单下方左侧）
sf::FloatRect ClientView::merchantCreateSubmitBtnRect() {
	constexpr float w = 160.f, h = 44.f;
	return sf::FloatRect(sf::Vector2f{ 250, 440 }, sf::Vector2f{ w, h });
}

// 返回按钮（提交按钮右侧）
sf::FloatRect ClientView::merchantCreateBackBtnRect() {
	constexpr float w = 120.f, h = 44.f;
	return sf::FloatRect(sf::Vector2f{ 430, 440 }, sf::Vector2f{ w, h });
}

// ===================== "我的订单"面板滚动 =====================

float ClientView::computeMyOrdersContentHeight(const ClientModel& model) const noexcept {
	const auto& orders = model.orders();
	if (orders.empty()) return 0.f;
	// 与 drawMyOrdersPanel 的布局同步：每张卡片 90 + items * rowH，卡片间距 12
	float h = 0.f;
	for (const auto& order : orders) {
		h += 90.f + static_cast<float>(order.items.size()) * orderItemRowH + 12.f;
	}
	// 末尾的 12 是 drawMyOrdersPanel 里 `y = itemY + 12.f` 累加的，对可见性无影响
	return h;
}

float ClientView::computeMyOrdersVisibleHeight() const noexcept {
	// 从 orderCardStartY 到窗口底部，底部留 20 像素边距
	return std::max(100.f, static_cast<float>(window_.getSize().y) - orderCardStartY - 20.f);
}

void ClientView::scrollMyOrders(float deltaPx, const ClientModel& model) {
	// 仅 MyOrders 面板应用滚动；其他面板忽略（防止误滚后切回时位置错乱）
	if (panel_ != Panel::MyOrders) return;
	const float contentH = computeMyOrdersContentHeight(model);
	const float visibleH = computeMyOrdersVisibleHeight();
	const float maxOffset = (contentH > visibleH) ? (contentH - visibleH) : 0.f;
	float newY = myOrdersScrollY_ + deltaPx;
	if (newY < 0.f) newY = 0.f;
	if (newY > maxOffset) newY = maxOffset;
	myOrdersScrollY_ = newY;
}

// ===================== 商品列表面板滚动 =====================

float ClientView::computeProductListContentHeight(const ClientModel& model) const noexcept {
	const auto& products = model.products();
	if (products.empty()) return 0.f;
	// 与 drawProductListPanel 布局同步：每行 perRow 张卡片，每张 cardH + gapY
	const int rows = static_cast<int>((products.size() + perRow - 1) / perRow);
	return static_cast<float>(rows) * (cardH + gapY);
}

float ClientView::computeProductListVisibleHeight() const noexcept {
	// 从 startY 到窗口底部，底部留 20 像素边距
	return std::max(100.f, static_cast<float>(window_.getSize().y) - startY - 20.f);
}

void ClientView::scrollProductList(float deltaPx, const ClientModel& model) {
	// 仅 ProductList 面板应用滚动；其他面板忽略（防止误滚后切回时位置错乱）
	if (panel_ != Panel::ProductList) return;
	const float contentH = computeProductListContentHeight(model);
	const float visibleH = computeProductListVisibleHeight();
	const float maxOffset = (contentH > visibleH) ? (contentH - visibleH) : 0.f;
	float newY = productListScrollY_ + deltaPx;
	if (newY < 0.f) newY = 0.f;
	if (newY > maxOffset) newY = maxOffset;
	productListScrollY_ = newY;
}
