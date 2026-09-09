#pragma once
// 客户端 View：SFML 渲染层
// 复用 TextManager / ImageManager；从 ClientModel 读状态，不依赖网络层。
#include <SFML/Graphics.hpp>
#include "TextManager.h"
#include "ImageManager.h"
#include "ClientModel.h"
#include "Product.h"

class ClientView {
public:
    ClientView(sf::RenderWindow& window, TextManager& text, ImageManager& image);

    // 主帧渲染：标题 + 状态 + 商品网格
    void render(const ClientModel& model);

private:
    sf::RenderWindow& window_;
    TextManager&       text_;
    ImageManager&      image_;

    // 绘制单张商品卡片
    void drawCard(const Product& p, const sf::Vector2f& pos, const sf::Vector2f& size);
};
