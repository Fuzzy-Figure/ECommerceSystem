#include "utils.h"
#include <Windows.h>
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace ecs {
	namespace {
		//从 exe 自身目录逐级向上搜索指定配置文件，返回其绝对路径；到磁盘根仍未找到则抛异常
		std::filesystem::path locateConfigFile(const std::string& filename) {
			wchar_t exePath[MAX_PATH]{};
			GetModuleFileNameW(nullptr, exePath, MAX_PATH);
			std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
			while (true) {
				const auto candidate = dir / filename;
				if (std::filesystem::exists(candidate)) {
					return candidate;
				}
				const auto parent = dir.parent_path();
				if (parent == dir) {
					throw std::runtime_error("无法定位配置文件 " + filename + "（已从 exe 目录向上搜索到磁盘根）");
				}
				dir = parent;
			}
		}

		nlohmann::json parseConfigFile(const std::string& filename) {
			std::ifstream configFile(locateConfigFile(filename));
			if (!configFile.is_open()) {
				throw std::runtime_error("无法打开配置文件 " + filename);
			}
			return nlohmann::json::parse(configFile, nullptr, true, true);
		}
	}

	const std::filesystem::path& projectRoot() {
		static const std::filesystem::path root = []() {
			try {
				return locateConfigFile("server_config.json").parent_path();
			}
			catch (const std::exception&) {
				return locateConfigFile("client_config.json").parent_path();
			}
		}();
		return root;
	}

	nlohmann::json& getServerConfig() {
		static nlohmann::json config = parseConfigFile("server_config.json");
		return config;
	}

	void reloadServerConfig() {
		getServerConfig() = parseConfigFile("server_config.json");
	}

	nlohmann::json& getClientConfig() {
		static nlohmann::json config = parseConfigFile("client_config.json");
		return config;
	}

	void reloadClientConfig() {
		getClientConfig() = parseConfigFile("client_config.json");
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


