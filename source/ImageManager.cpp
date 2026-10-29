#include "ImageManager.h"
#include "utils.h"
#include <print>
#include <fstream>

namespace {
	//图片加载失败时的占位图（相对项目根）
	const std::string kPlaceholderPath = "images/placeholder.png";

	//加载纹理；失败返回 nullptr（不抛异常）
	std::unique_ptr<sf::Texture> tryLoadTexture(const std::string& absolutePath) {
		auto texture = std::make_unique<sf::Texture>();
		if (!texture->loadFromFile(ecs::string::to_utf16(absolutePath))) {
			return nullptr;
		}
		return texture;
	}
}

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
	auto it = textureCache.find(absolutePath);
	if (it == textureCache.end()) {
		auto texture = tryLoadTexture(absolutePath);
		if (!texture) {
			// 主图失败：降级为占位图，并缓存到该路径键下（之后每帧直接命中，不再重试）
			std::println("[ImageManager] 图片加载失败，改用占位图：{}", absolutePath);
			texture = tryLoadTexture(EC + kPlaceholderPath);
			if (!texture) {
				// 占位图也失败：缓存空纹理标记"放弃"，之后静默跳过
				std::println("[ImageManager] 占位图也加载失败，放弃显示：{}", absolutePath);
				textureCache.emplace(absolutePath, nullptr);
				return;
			}
		}
		else {
			std::println("[ImageManager] 纹理加载成功，路径：{}，尺寸：{}*{}",
					absolutePath, texture->getSize().x, texture->getSize().y);
		}
		it = textureCache.emplace(absolutePath, std::move(texture)).first;
	}

	if (!it->second) {
		return;  // 此前已标记放弃显示
	}

	// 直接取缓存绘制
	sf::Texture& texture = *it->second;

	sf::Sprite sprite(texture);
	sprite.setPosition(pos);

	sf::Vector2u textureSize = texture.getSize();
	float scaleX = size.x / static_cast<float>(textureSize.x);
	float scaleY = size.y / static_cast<float>(textureSize.y);
	sprite.setScale({ scaleX, scaleY });

	window.draw(sprite);
}







