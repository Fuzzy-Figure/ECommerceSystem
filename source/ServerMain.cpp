#include <SFML/Graphics.hpp>
#include <string>

std::string cppVersionName(long v) {
    if (v == 199711L) return "C++98/Pre-C++11";
    if (v == 201103L) return "C++11";
    if (v == 201402L) return "C++14";
    if (v == 201703L) return "C++17";
    if (v == 202002L) return "C++20";
    if (v == 202302L) return "C++23";
    if (v == 202603L) return "C++26";
    return "Unknown C++ version";
}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 200 }), "SFML3 C++ Version");

    // 换成你自己的字体文件路径
    sf::Font font("C:/Windows/Fonts/msyh.ttc");

    sf::Text text(font);
    text.setString(
        "C++ version: " + cppVersionName(__cplusplus) +
        "\n__cplusplus = " + std::to_string(__cplusplus)
    );
    text.setCharacterSize(28);
    text.setFillColor(sf::Color::Green);
    text.setPosition({ 20.f, 20.f });

    while (window.isOpen()) {
        while (const std::optional e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>())
                window.close();
        }

        window.clear(sf::Color::Black);
        window.draw(text);
        window.display();
    }
}