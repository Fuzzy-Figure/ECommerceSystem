#include "../header/ClientView.h"
#include "../header/utils.h"
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
}

void ClientView::render(const ClientModel& model) {
	switch (panel_) {
		case Panel::ProductList: drawProductListPanel(model); break;
		case Panel::Cart:         drawCartPanel(model);       break;
		case Panel::MyOrders:     drawMyOrdersPanel(model);   break;
	}
}

// ===================== 顶部 Tab 标签栏 =====================

void ClientView::drawTabBar() {
	const wchar_t* labels[] = { L"商品列表", L"购物车", L"我的订单" };
	const int current = static_cast<int>(panel_);
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
}

// ===================== 商品列表面板 =====================

void ClientView::drawProductListPanel(const ClientModel& model) {
	const auto& products = model.products();

	drawTabBar();
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
								startY + row * (cardH + gapY) };
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

	drawTabBar();
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

	drawTabBar();
	if (!model.status().empty()) {
		textMgr_.displayText(model.status(), { 700, statusY }, { 18, 22 }, sf::Color(150, 150, 150));
	}

	if (orders.empty()) {
		textMgr_.displayTextInCenter(L"暂无历史订单，先去结算一单试试", { 20, 30 }, sf::Color(150, 150, 150));
		return;
	}

	// 起始 y，逐订单下移
	float y = orderCardStartY;
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

	// 顶部 Tab 标签优先（所有面板共用）
	for (int i = 0; i < 3; ++i) {
		if (hit(tabBtnRect(i), mousePos)) {
			return { ClickAction::SwitchPanel, i };
		}
	}

	if (panel_ == Panel::ProductList) {
		const auto& products = model.products();
		for (std::size_t i = 0; i < products.size(); ++i) {
			const auto col = static_cast<int>(i % perRow);
			const auto row = static_cast<int>(i / perRow);
			const sf::Vector2f cardPos{ startX + col * (cardW + gapX),
										startY + row * (cardH + gapY) };
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
	else {  // Panel::MyOrders
		// 复刻 drawMyOrdersPanel 的布局：从 orderCardStartY 起逐订单逐明细下移
		float y = orderCardStartY;
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
