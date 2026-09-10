#include "../header/ClientView.h"
#include "../header/utils.h"
#include <sstream>
#include <string>

ClientView::ClientView(sf::RenderWindow& window, TextManager& text, ImageManager& image)
    : window_(window), text_(text), image_(image) {}

namespace {
    // === 商品列表面板布局 ===
    constexpr float cardW = 580.f, cardH = 380.f;
    constexpr float gapX  = 10.f,  gapY = 10.f;
    constexpr float startX = 120.f, startY = 80.f;
    constexpr int   perRow = 3;
    // 加购按钮：卡片右下角
    constexpr float addBtnDX = 460.f, addBtnDY = 340.f;
    constexpr float addBtnW = 100.f, addBtnH = 30.f;

    // === 购物车面板布局 ===
    constexpr float cartRowX    = 200.f, cartStartY = 150.f, cartRowH = 70.f;
    constexpr float cartNameX   = 200.f;
    constexpr float cartQtyX    = 620.f;
    constexpr float cartPriceX  = 780.f;
    constexpr float cartSubX    = 940.f;
    // 删除按钮：行内右侧
    constexpr float rmBtnDX     = 1120.f, rmBtnDY = 15.f;
    constexpr float rmBtnW      = 80.f,  rmBtnH  = 40.f;
    // 结算按钮 + 总额
    constexpr float checkoutBtnX = 1150.f, checkoutBtnY = 890.f;
    constexpr float checkoutBtnW = 200.f,  checkoutBtnH = 60.f;
    constexpr float cartTotalY    = 900.f;
}

void ClientView::render(const ClientModel& model) {
    if (panel_ == Panel::ProductList) {
        drawProductListPanel(model);
    } else {
        drawCartPanel(model);
    }
}

// ===================== 商品列表面板 =====================

void ClientView::drawProductListPanel(const ClientModel& model) {
    const auto& products = model.products();

    text_.displayText(L"微商系统 — 商品列表", {20, 12}, {20, 36}, sf::Color::Black);
    text_.displayText(L"按 Tab 切换购物车  |  按 R 刷新  |  鼠标点击 +加购",
                      {20, 56}, {18, 22}, sf::Color(120, 120, 120));
    if (!model.status().empty()) {
        text_.displayText(model.status(), {600, 12}, {18, 22}, sf::Color(150, 150, 150));
    }
    if (products.empty()) {
        text_.displayTextInCenter(L"暂无商品，按 R 刷新", {20, 30}, sf::Color(150, 150, 150));
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
            image_.displayImage(p.imagePath, imgPos, imgSize);
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
    text_.displayText(name, { pos.x + 10, pos.y + 260 }, { 20, 28 }, sf::Color::Black);

    // 描述
    const auto desc = ec::string::to_utf16(p.description);
    text_.displayText(desc, { pos.x + 10, pos.y + 300 }, { 20, 20 }, sf::Color(100, 100, 100));

    // 价格
    std::wostringstream priceStr;
    priceStr << L"¥ " << p.price;
    text_.displayText(priceStr.str(), { pos.x + 10, pos.y + 330 }, { 20, 32 }, sf::Color(200, 50, 50));

    // 库存
    std::wostringstream stockStr;
    stockStr << L"库存：" << p.stock;
    text_.displayText(stockStr.str(), { pos.x + 250, pos.y + 345 }, { 20, 20 }, sf::Color(100, 100, 100));

    // 加购按钮（绿色）
    const auto btnRect = addToCartBtnRect(pos);
    sf::RectangleShape btn({ btnRect.size.x, btnRect.size.y });
    btn.setPosition({ btnRect.position.x, btnRect.position.y });
    btn.setFillColor(sf::Color(80, 160, 80));
    btn.setOutlineColor(sf::Color(60, 130, 60));
    btn.setOutlineThickness(1.f);
    window_.draw(btn);
    text_.displayText(L"+ 加购",
                      { btnRect.position.x + 18, btnRect.position.y + 3 },
                      { 18, 22 }, sf::Color::White);
}

// ===================== 购物车面板 =====================

void ClientView::drawCartPanel(const ClientModel& model) {
    const auto& cart = model.cart();

    text_.displayText(L"微商系统 — 购物车", {20, 12}, {20, 36}, sf::Color::Black);
    text_.displayText(L"按 Tab 切回商品列表  |  鼠标点击 - 删除单项  |  点击 结算 提交订单",
                      {20, 56}, {18, 22}, sf::Color(120, 120, 120));
    if (!model.status().empty()) {
        text_.displayText(model.status(), {600, 12}, {18, 22}, sf::Color(150, 150, 150));
    }

    // 表头
    text_.displayText(L"商品名", {cartNameX, cartStartY - 50}, {18, 22}, sf::Color(80, 80, 80));
    text_.displayText(L"数量",   {cartQtyX,  cartStartY - 50}, {18, 22}, sf::Color(80, 80, 80));
    text_.displayText(L"单价",   {cartPriceX,cartStartY - 50}, {18, 22}, sf::Color(80, 80, 80));
    text_.displayText(L"小计",   {cartSubX,  cartStartY - 50}, {18, 22}, sf::Color(80, 80, 80));

    sf::RectangleShape line({ 1700, 1.f });
    line.setPosition({ cartRowX - 50, cartStartY - 20 });
    line.setFillColor(sf::Color(200, 200, 200));
    window_.draw(line);

    if (cart.empty()) {
        text_.displayTextInCenter(L"购物车为空，去加购商品吧", {20, 30}, sf::Color(150, 150, 150));
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
        text_.displayText(name, { rowPos.x, rowPos.y + 15 }, {20, 26}, sf::Color::Black);
        // 数量
        std::wostringstream qtyStr;
        qtyStr << item.qty;
        text_.displayText(qtyStr.str(), { cartQtyX, rowPos.y + 15 }, {20, 26}, sf::Color::Black);
        // 单价
        std::wostringstream priceStr;
        priceStr << L"¥" << item.price;
        text_.displayText(priceStr.str(), { cartPriceX, rowPos.y + 15 }, {20, 26}, sf::Color(80, 80, 80));
        // 小计
        std::wostringstream subStr;
        subStr << L"¥" << item.subtotal();
        text_.displayText(subStr.str(), { cartSubX, rowPos.y + 15 }, {20, 26}, sf::Color(200, 50, 50));

        // 删除按钮
        const auto rmRect = removeFromCartBtnRect(rowPos);
        sf::RectangleShape rmBtn({ rmRect.size.x, rmRect.size.y });
        rmBtn.setPosition({ rmRect.position.x, rmRect.position.y });
        rmBtn.setFillColor(sf::Color(200, 80, 80));
        rmBtn.setOutlineColor(sf::Color(160, 60, 60));
        rmBtn.setOutlineThickness(1.f);
        window_.draw(rmBtn);
        text_.displayText(L"- 删除",
                          { rmRect.position.x + 12, rmRect.position.y + 8 },
                          {16, 22}, sf::Color::White);
    }

    // 总额
    std::wostringstream totalStr;
    totalStr << L"总额：¥ " << model.cartTotal();
    text_.displayText(totalStr.str(), { cartSubX - 200, cartTotalY }, {24, 36}, sf::Color(200, 50, 50));

    // 结算按钮
    const auto ckRect = checkoutBtnRect();
    sf::RectangleShape ckBtn({ ckRect.size.x, ckRect.size.y });
    ckBtn.setPosition({ ckRect.position.x, ckRect.position.y });
    ckBtn.setFillColor(sf::Color(220, 130, 30));
    ckBtn.setOutlineColor(sf::Color(180, 100, 20));
    ckBtn.setOutlineThickness(1.f);
    window_.draw(ckBtn);
    text_.displayText(L"结算",
                      { ckRect.position.x + 70, ckRect.position.y + 14 },
                      {24, 32}, sf::Color::White);
}

// ===================== 鼠标命中测试 =====================

ClientView::ClickAction ClientView::handleClick(const sf::Vector2f& mousePos, const ClientModel& model) {
    auto hit = [](const sf::FloatRect& r, const sf::Vector2f& p) {
        return p.x >= r.position.x && p.x < r.position.x + r.size.x
            && p.y >= r.position.y && p.y < r.position.y + r.size.y;
    };

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
    } else {
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
