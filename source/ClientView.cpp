#include "../header/ClientView.h"
#include "../header/utils.h"
#include <sstream>
#include <string>

ClientView::ClientView(sf::RenderWindow& window, TextManager& text, ImageManager& image)
    : window_(window), text_(text), image_(image) {}

void ClientView::render(const ClientModel& model) {
    const auto& products = model.products();
    constexpr float cardW = 580.f, cardH = 380.f;
    constexpr float gapX  = 10.f,  gapY  = 10.f;
    constexpr float startX = 120.f, startY = 80.f;
    constexpr int   perRow = 3;

    // 标题
    text_.displayText(L"微商系统 — 商品列表", {20, 12}, {20, 36}, sf::Color::Black);
    // 状态/提示
    if (!model.status().empty()) {
        text_.displayText(model.status(), {20, 56}, {20, 22}, sf::Color(150, 150, 150));
    }
    // 列表为空时给出提示
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
    text_.displayText(stockStr.str(), { pos.x + 400, pos.y + 345 }, { 20, 20 }, sf::Color(100, 100, 100));
}
