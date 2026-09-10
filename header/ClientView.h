#pragma once
// 客户端 View：SFML 渲染层
// 复用 TextManager / ImageManager；从 ClientModel 读状态，不依赖网络层。
// 两个面板：商品列表 / 购物车，按 Tab 切换；商品卡片右下角"加购"按钮可鼠标点击。
#include <SFML/Graphics.hpp>
#include <cstdint>
#include "TextManager.h"
#include "ImageManager.h"
#include "ClientModel.h"
#include "Product.h"

class ClientView {
public:
    enum class Panel { ProductList, Cart };

    // 鼠标点击命中后返回的动作；type=None 表示未命中任何按钮
    struct ClickAction {
        enum Type { None, AddToCart, RemoveFromCart, Checkout } type{None};
        std::int32_t arg{0};  // AddToCart/RemoveFromCart 用 productId
    };

    ClientView(sf::RenderWindow& window, TextManager& text, ImageManager& image);

    // 主帧渲染：根据当前面板渲染商品列表或购物车
    void render(const ClientModel& model);

    Panel panel() const noexcept { return panel_; }
    void togglePanel() {
        panel_ = (panel_ == Panel::ProductList ? Panel::Cart : Panel::ProductList);
    }
    void setPanel(Panel p) noexcept { panel_ = p; }

    // 处理鼠标点击，返回命中按钮的动作；坐标为窗口世界坐标
    ClickAction handleClick(const sf::Vector2f& mousePos, const ClientModel& model);

private:
    sf::RenderWindow& window_;
    TextManager&       text_;
    ImageManager&      image_;
    Panel              panel_{Panel::ProductList};

    // === 商品列表面板 ===
    void drawProductListPanel(const ClientModel& model);
    // 绘制单张商品卡片；index 用于换行计算
    void drawCard(const Product& p, const sf::Vector2f& pos, const sf::Vector2f& size);

    // === 购物车面板 ===
    void drawCartPanel(const ClientModel& model);

    // === 按钮矩形计算（与 drawXxxPanel 内的布局保持一致）===
    // 商品卡片右下角的"+加购"按钮
    static sf::FloatRect addToCartBtnRect(const sf::Vector2f& cardPos);
    // 购物车某行的"-"删除按钮；rowPos 是该行左上角
    static sf::FloatRect removeFromCartBtnRect(const sf::Vector2f& rowPos);
    // 购物车底部"结算"按钮
    static sf::FloatRect checkoutBtnRect();
};
