#pragma once
#include <string>
#include <filesystem>
#include <json.hpp>


namespace ecs {
	//项目根：从 exe 自身目录逐级向上搜索 server_config.json / client_config.json，找到即项目根（结果缓存）
	//所有相对资源（数据库、图片）均相对项目根解析，不依赖工作目录
	const std::filesystem::path& projectRoot();

	//服务器专用配置：读取 server_config.json 并缓存；reload 可强制重读
	nlohmann::json& getServerConfig();
	//强制重新读取 server_config.json，刷新缓存
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


