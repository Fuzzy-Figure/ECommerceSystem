#include "utils.h"
#include <Windows.h>
#include <fstream>

namespace ec {
	nlohmann::json& getServerConfig() {
		static nlohmann::json config = []() {
			std::ifstream configFile("../server_config.json");
			if (!configFile.is_open()) {
				throw std::runtime_error("无法打开配置文件 server_config.json");
			}
			return nlohmann::json::parse(configFile, nullptr, true, true);
		}();
		return config;
	}

	void reloadServerConfig() {
		std::ifstream configFile("../server_config.json");
		if (!configFile.is_open()) {
			throw std::runtime_error("无法打开配置文件 server_config.json");
		}
		getServerConfig() = nlohmann::json::parse(configFile, nullptr, true, true);
	}

	nlohmann::json& getClientConfig() {
		static nlohmann::json config = []() {
			std::ifstream configFile("../client_config.json");
			if (!configFile.is_open()) {
				throw std::runtime_error("无法打开配置文件 client_config.json");
			}
			return nlohmann::json::parse(configFile, nullptr, true, true);
		}();
		return config;
	}

	void reloadClientConfig() {
		std::ifstream configFile("../client_config.json");
		if (!configFile.is_open()) {
			throw std::runtime_error("无法打开配置文件 client_config.json");
		}
		getClientConfig() = nlohmann::json::parse(configFile, nullptr, true, true);
	}

	namespace string {
		std::wstring to_utf16(const std::string& utf8) {
			if (utf8.empty()) return std::wstring();

			int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
			std::wstring utf16(len, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], len);
			utf16.pop_back(); // 去掉结尾的空字符
			return utf16;
		}
		std::string to_utf8(const std::wstring& utf16) {
			if (utf16.empty()) return std::string();

			int len = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, nullptr, 0, nullptr, nullptr);
			std::string utf8(len, '\0');
			WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, &utf8[0], len, nullptr, nullptr);
			utf8.pop_back(); // 去掉结尾的空字符
			return utf8;
		}
	}



}


