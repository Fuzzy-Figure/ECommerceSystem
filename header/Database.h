#pragma once
// 持久层入口：SQLite RAII 封装
// 头文件仅前向声明 sqlite3，避免在头文件中暴露 sqlite3.h。
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <json.hpp>

struct sqlite3;  // 前向声明；实现见 Database.cpp

// SQL 绑定参数值：支持 64 位整数 / 浮点 / UTF-8 字符串
// 传给 executeStmt/queryStmt 后按位置绑定到 SQL 中的 ? 占位符
using DbValue = std::variant<std::int64_t, double, std::string>;

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

	// 执行带绑定参数的 SQL（DML），参数按位置绑定到 ?；返回受影响行数
	// 用户输入必须走此接口，禁止拼接到 SQL 字符串（防注入）
	int executeStmt(const std::string& sql, const std::vector<DbValue>& params);

	// 查询带绑定参数的 SQL，返回 JSON 行（列值按 SQLite 实际类型映射：INTEGER/FLOAT→number，TEXT→string，NULL→null）
	std::vector<nlohmann::json> queryStmt(const std::string& sql, const std::vector<DbValue>& params);

private:
	struct SqliteDeleter {
		void operator()(sqlite3* db) const noexcept;  // 在 .cpp 实现
	};
	std::unique_ptr<sqlite3, SqliteDeleter> db_;
};
