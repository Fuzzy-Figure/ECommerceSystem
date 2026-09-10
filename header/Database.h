#pragma once
// 持久层入口：SQLite RAII 封装
// 头文件仅前向声明 sqlite3，避免在头文件中暴露 sqlite3.h。
#include <memory>
#include <string>
#include <vector>
#include <json.hpp>

struct sqlite3;  // 前向声明；实现见 Database.cpp

class Database {
public:
	explicit Database(const std::string& path);
	~Database();  // 必须在 .cpp 中实现（unique_ptr 析构需 sqlite3 完整定义）

	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;
	Database(Database&&) noexcept;
	Database& operator=(Database&&) noexcept;

	// 执行无返回值 SQL（DDL/DML），返回受影响行数；失败抛 std::runtime_error
	int execute(const std::string& sql);

	// 查询，每行以 JSON 对象返回；列名作为键，列值按类型映射。
	std::vector<nlohmann::json> query(const std::string& sql);

private:
	struct SqliteDeleter {
		void operator()(sqlite3* db) const noexcept;  // 在 .cpp 实现
	};
	std::unique_ptr<sqlite3, SqliteDeleter> db_;
};
