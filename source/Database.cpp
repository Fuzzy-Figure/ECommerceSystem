#include "Database.h"
#include <sqlite3.h>
#include <stdexcept>
#include <print>
#include <format>

Database::Database(const std::string& path) {
	sqlite3* raw = nullptr;
	const int rc = sqlite3_open(path.c_str(), &raw);
	if (rc != SQLITE_OK) {
		const std::string msg = raw ? sqlite3_errmsg(raw) : "未知错误";
		if (raw) sqlite3_close(raw);
		throw std::runtime_error(std::format("[Database] 打开失败：{} -> {}", path, msg));
	}
	db_.reset(raw);
	std::println("[Database] 已打开：{}", path);
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

// ===================== Prepared Statement =====================

namespace {
	// RAII 封装 sqlite3_stmt，离开作用域自动 finalize
	struct StmtDeleter {
		void operator()(sqlite3_stmt* stmt) const noexcept {
			if (stmt) sqlite3_finalize(stmt);
		}
	};
	using StmtPtr = std::unique_ptr<sqlite3_stmt, StmtDeleter>;

	StmtPtr prepareStmt(sqlite3* db, const std::string& sql) {
		sqlite3_stmt* raw = nullptr;
		const int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &raw, nullptr);
		if (rc != SQLITE_OK) {
			throw std::runtime_error("[Database] 预编译失败：" + std::string(sqlite3_errmsg(db)) + "；SQL：" + sql);
		}
		return StmtPtr(raw);
	}

	// 按位置（1 起）绑定参数到 ? 占位符
	void bindParams(sqlite3_stmt* stmt, const std::vector<DbValue>& params) {
		for (std::size_t i = 0; i < params.size(); ++i) {
			const int idx = static_cast<int>(i + 1);
			int rc = SQLITE_OK;
			std::visit([&](const auto& v) {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, std::int64_t>) {
					rc = sqlite3_bind_int64(stmt, idx, v);
				}
				else if constexpr (std::is_same_v<T, double>) {
					rc = sqlite3_bind_double(stmt, idx, v);
				}
				else {
					// std::string：SQLITE_TRANSIENT 让 SQLite 内部拷贝，绑定后源字符串可释放
					rc = sqlite3_bind_text(stmt, idx, v.c_str(), static_cast<int>(v.size()), SQLITE_TRANSIENT);
				}
			}, params[i]);
			if (rc != SQLITE_OK) {
				throw std::runtime_error("[Database] 绑定参数失败，位置 " + std::to_string(idx));
			}
		}
	}
}

int Database::executeStmt(const std::string& sql, const std::vector<DbValue>& params) {
	auto stmt = prepareStmt(db_.get(), sql);
	bindParams(stmt.get(), params);
	const int rc = sqlite3_step(stmt.get());
	if (rc != SQLITE_DONE) {
		throw std::runtime_error("[Database] 执行失败：" + std::string(sqlite3_errmsg(db_.get())) + "；SQL：" + sql);
	}
	return sqlite3_changes(db_.get());
}

std::vector<nlohmann::json> Database::queryStmt(const std::string& sql, const std::vector<DbValue>& params) {
	auto stmt = prepareStmt(db_.get(), sql);
	bindParams(stmt.get(), params);
	std::vector<nlohmann::json> rows;
	const int colCount = sqlite3_column_count(stmt.get());
	while (true) {
		const int rc = sqlite3_step(stmt.get());
		if (rc == SQLITE_DONE) break;
		if (rc != SQLITE_ROW) {
			throw std::runtime_error("[Database] 查询失败：" + std::string(sqlite3_errmsg(db_.get())) + "；SQL：" + sql);
		}
		nlohmann::json row = nlohmann::json::object();
		for (int i = 0; i < colCount; ++i) {
			const char* name = sqlite3_column_name(stmt.get(), i);
			const std::string col = name ? name : "";
			switch (sqlite3_column_type(stmt.get(), i)) {
				case SQLITE_INTEGER:
					row[col] = sqlite3_column_int64(stmt.get(), i);
					break;
				case SQLITE_FLOAT:
					row[col] = sqlite3_column_double(stmt.get(), i);
					break;
				case SQLITE_TEXT: {
					const auto* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), i));
					row[col] = txt ? std::string(txt) : std::string{};
					break;
				}
				default:
					row[col] = nullptr;
					break;
			}
		}
		rows.push_back(std::move(row));
	}
	return rows;
}
