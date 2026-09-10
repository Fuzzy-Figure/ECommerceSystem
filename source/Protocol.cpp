#include "../header/Protocol.h"
#include <string>
#include <iostream>

namespace proto {
	namespace {
		// 阻塞循环写满 size 字节，处理 SFML Partial 状态
		bool sendAll(sf::TcpSocket& socket, const void* data, std::size_t size) {
			if (size == 0) return true;
			const auto* p = static_cast<const std::uint8_t*>(data);
			std::size_t sent = 0;
			while (sent < size) {
				std::size_t thisSent = 0;
				const auto status = socket.send(p + sent, size - sent, thisSent);
				sent += thisSent;
				if (status == sf::Socket::Status::Disconnected ||
					status == sf::Socket::Status::Error ||
					status == sf::Socket::Status::NotReady) {
					return false;
				}
				// Done / Partial：继续直到全部发出
			}
			return true;
		}

		// 阻塞循环读满 size 字节
		bool recvAll(sf::TcpSocket& socket, void* data, std::size_t size) {
			if (size == 0) return true;
			auto* p = static_cast<std::uint8_t*>(data);
			std::size_t total = 0;
			while (total < size) {
				std::size_t got = 0;
				const auto status = socket.receive(p + total, size - total, got);
				if (status == sf::Socket::Status::Disconnected ||
					status == sf::Socket::Status::Error ||
					status == sf::Socket::Status::NotReady ||
					status == sf::Socket::Status::Partial) {
					return false;
				}
				if (got == 0) return false;
				total += got;
			}
			return true;
		}
	}

	bool sendJson(sf::TcpSocket& socket, const nlohmann::json& payload) {
		const std::string body = payload.dump();
		const std::uint32_t len = static_cast<std::uint32_t>(body.size());
		if (!sendAll(socket, &len, sizeof(len))) return false;
		if (!sendAll(socket, body.data(), body.size())) return false;
		return true;
	}

	std::optional<nlohmann::json> recvJson(sf::TcpSocket& socket) {
		std::uint32_t len = 0;
		if (!recvAll(socket, &len, sizeof(len))) return std::nullopt;
		// 上限防御：单条消息不超过 16MB
		if (len == 0 || len > 16u * 1024u * 1024u) return std::nullopt;
		std::string body(len, '\0');
		if (!recvAll(socket, body.data(), body.size())) return std::nullopt;
		try {
			return nlohmann::json::parse(body, nullptr, true, true);
		} catch (const std::exception& e) {
			std::cout << "[Protocol] JSON 解析失败：" << e.what() << std::endl;
			return std::nullopt;
		}
	}
}
