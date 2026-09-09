#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include "../header/utils.h"
#include "../header/TextManager.h"
#include "../header/ImageManager.h"
#include "../header/ClientModel.h"
#include "../header/ClientView.h"
#include "../header/ClientController.h"
#include <Windows.h>

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    try {
        ec::reloadClientConfig();
    } catch (const std::exception& e) {
        std::cerr << "加载配置失败：" << e.what() << std::endl;
        return 1;
    }

    const auto& cfg  = ec::getClientConfig();
    const auto  winW = cfg["size"]["window"]["width"].get<unsigned>();
    const auto  winH = cfg["size"]["window"]["height"].get<unsigned>();
    const auto  ip   = cfg["server"]["ip"].get<std::string>();
    const auto  port = cfg["server"]["port"].get<unsigned short>();

    const std::string titleUtf8 = "微商系统";
    sf::RenderWindow window(sf::VideoMode({ winW, winH }),
                             sf::String::fromUtf8(titleUtf8.begin(), titleUtf8.end()));

    TextManager  text(window);
    ImageManager image(window);

    ClientModel      model;
    ClientView       view(window, text, image);
    ClientController controller(window, model, view);

    model.setStatus(L"正在连接服务器...");
    if (controller.connect(ip, port)) {
        controller.requestProductList();  // 连接后立即拉取商品列表
    }

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            controller.handleEvent(*event);
        }
        controller.update();  // 消费接收线程投递的消息、更新 Model

        window.clear(sf::Color(245, 245, 245));
        view.render(model);
        window.display();
    }

    controller.disconnect();
    return 0;
}
