#pragma once
#include <string>
#include <json.hpp>


namespace ec {
	//服务器专用配置：读取 server_config.json 并缓存；reload 可强制重读
	nlohmann::json& getServerConfig();
	//强制重新读取 server_config.json，刷新缓存（bo 阶段每局前调用）
	void reloadServerConfig();

	//客户端专用配置：读取 client_config.json 并缓存；reload 可强制重读
	nlohmann::json& getClientConfig();
	//强制重新读取 client_config.json，刷新缓存
	void reloadClientConfig();

	namespace string {
		std::wstring to_utf16(const std::string& utf8);
		std::string to_utf8(const std::wstring& wstr);
	}
}


