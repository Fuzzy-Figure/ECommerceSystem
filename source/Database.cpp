#include "../header/Database.h"
#include "../header/sqlite3.h"
#include <stdexcept>
#include <iostream>

Database::Database(const std::string& path) {
	sqlite3* raw = nullptr;
	const int rc = sqlite3_open(path.c_str(), &raw);
	if (rc != SQLITE_OK) {
		const std::string msg = raw ? sqlite3_errmsg(raw) : "未知错误";
		if (raw) sqlite3_close(raw);
		throw std::runtime_error("[Database] 打开失败：" + path + " -> " + msg);
	}
	db_.reset(raw);
	std::cout << "[Database] 已打开：" << path << std::endl;
}

Database::~Database() = default;

Database::Database(Database&&) noexcept = default;
Database& Database::operator=(Database&&) noexcept = default;

void Database::SqliteDeleter::operator()(sqlite3* db) const noexcept {
	if (db) sqlite3_close(db);
}

int Database::execute(const std::string& sql) {
	char* errMsg = nullptr;
	const int rc = sqlite3_exec(db_.get(), sql.c_str(), nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK) {
		const std::string msg = errMsg ? errMsg : "未知错误";
		sqlite3_free(errMsg);
		throw std::runtime_error("[Database] 执行失败：" + msg + "；SQL：" + sql);
	}
	return sqlite3_changes(db_.get());
}

namespace {
	// 回调上下文：收集所有行
	struct QueryContext {
		std::vector<nlohmann::json> rows;
		std::vector<std::string>    columns;
		bool                        firstRow{ true };
	};

	int queryCallback(void* ctx, int argc, char** values, char** names) {
		const auto qc = static_cast<QueryContext*>(ctx);
		if (qc->firstRow) {
			qc->columns.reserve(argc);
			for (int i = 0; i < argc; ++i) qc->columns.emplace_back(names[i] ? names[i] : "");
			qc->firstRow = false;
		}
		nlohmann::json row = nlohmann::json::object();
		for (int i = 0; i < argc; ++i) {
			const std::string col = qc->columns[i];
			row[col] = values[i] ? values[i] : nlohmann::json{ nullptr };
		}
		qc->rows.push_back(std::move(row));
		return 0;
	}
}

std::vector<nlohmann::json> Database::query(const std::string& sql) {
	QueryContext ctx;
	char* errMsg = nullptr;
	const int rc = sqlite3_exec(db_.get(), sql.c_str(), queryCallback, &ctx, &errMsg);
	if (rc != SQLITE_OK) {
		const std::string msg = errMsg ? errMsg : "未知错误";
		sqlite3_free(errMsg);
		throw std::runtime_error("[Database] 查询失败：" + msg + "；SQL：" + sql);
	}
	return std::move(ctx.rows);
}
