#include "ImageManager.h"
#include "utils.h"
#include <print>
#include <fstream>


ImageManager::ImageManager(sf::RenderWindow& _window) :window(_window) {
	EC = ecs::projectRoot().string() + '/';
	std::println("EC路径：{}", EC);
}

// 显示图片
void ImageManager::displayImage(const std::string& path, const sf::Vector2f& pos, const sf::Vector2f& size) {
	if (!window.isOpen()) {
		throw std::runtime_error("[ImageManager] 窗口无效");
	}

	const std::string absolutePath = EC + path;
	//缓存未命中
	if (auto it = textureCache.find(absolutePath); it == textureCache.end()) {
		auto texture = std::make_unique<sf::Texture>();
		if (!texture->loadFromFile(ecs::string::to_utf16(absolutePath))) {
			throw std::runtime_error("[ImageManager] 纹理加载失败，路径：" + absolutePath);
		}
		std::println("[ImageManager] 纹理加载成功，路径：{}，尺寸：{}*{}",
				absolutePath, texture->getSize().x, texture->getSize().y);
		textureCache.emplace(absolutePath, std::move(texture));
	}

	// 直接从缓存取，此时一定存在
	sf::Texture& texture = *textureCache[absolutePath];

	sf::Sprite sprite(texture);
	sprite.setPosition(pos);

	sf::Vector2u textureSize = texture.getSize();
	float scaleX = size.x / static_cast<float>(textureSize.x);
	float scaleY = size.y / static_cast<float>(textureSize.y);
	sprite.setScale({ scaleX, scaleY });

	window.draw(sprite);
}







