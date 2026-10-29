#include "UserDAO.h"
#include <functional>
#include <format>

UserDAO::UserDAO(Database& db) : db_(db) {}

std::string UserDAO::hashPassword(const std::string& username, const std::string& password) {
    // 简单 hash：username 做 salt，拼接后取 std::hash
    // 课设够用，非密码学安全；同编译器同平台结果一致
    const std::string salted = username + ":" + password;
    const auto h = std::hash<std::string>{}(salted);
    return std::format("{}", h);
}

namespace {
    std::int64_t toInt64(const nlohmann::json& v) {
        if (v.is_number()) return v.get<std::int64_t>();
        if (v.is_string()) { try { return std::stoll(v.get<std::string>()); } catch (...) {} }
        return 0;
    }
    std::string toStr(const nlohmann::json& v) {
        if (v.is_string()) return v.get<std::string>();
        if (v.is_number()) return std::to_string(v.get<double>());
        return {};
    }

    // 把一行 JSON 映射为 User（findByUsername/authenticate 共用）
    User mapUserRow(const nlohmann::json& row) {
        User u;
        u.id       = toInt64(row["id"]);
        u.username = toStr(row["username"]);
        u.role     = static_cast<User::Role>(toInt64(row["role"]));
        return u;
    }
}

std::optional<User> UserDAO::findByUsername(const std::string& username) {
    auto rows = db_.queryStmt(
        "SELECT id, username, role FROM users WHERE username=?;",
        { username });
    if (rows.empty()) return std::nullopt;
    return mapUserRow(rows.front());
}

std::optional<User> UserDAO::authenticate(const std::string& username, const std::string& password) {
    const std::string hash = hashPassword(username, password);
    auto rows = db_.queryStmt(
        "SELECT id, username, role FROM users WHERE username=? AND password_hash=?;",
        { username, hash });
    if (rows.empty()) return std::nullopt;
    return mapUserRow(rows.front());
}

std::vector<User> UserDAO::findAll() {
    std::vector<User> users;
    auto rows = db_.query("SELECT id, username, role FROM users ORDER BY id;");
    users.reserve(rows.size());
    for (const auto& r : rows) users.push_back(mapUserRow(r));
    return users;
}

bool UserDAO::createUser(const std::string& username, const std::string& password, std::int64_t& newIdOut) {
    if (findByUsername(username).has_value()) return false;  // 用户名已存在

    // 新注册用户默认 role=consumer（普通用户），商家角色由种子数据或后台指定
    db_.executeStmt(
        "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?);",
        { username, hashPassword(username, password), static_cast<std::int64_t>(User::Role::consumer) });

    auto idRows = db_.query("SELECT last_insert_rowid() AS id;");
	if (!idRows.empty()) {
		newIdOut = toInt64(idRows.front()["id"]);
		return newIdOut > 0;
	}
	return false;
}

// 按 id 查 username（UserDAO 内部用，不暴露到 .h）
namespace {
    std::string usernameById(Database& db, std::int64_t userId) {
        auto rows = db.queryStmt(
            "SELECT username FROM users WHERE id=?;",
            { userId });
        if (rows.empty()) return {};
        return toStr(rows.front()["username"]);
    }
}

bool UserDAO::updateUsername(std::int64_t userId, const std::string& newName,
                             const std::string& currentPassword) {
    // 1. 取当前用户名
    const std::string oldName = usernameById(db_, userId);
    if (oldName.empty()) return false;  // 用户不存在
    if (newName == oldName) return false; // 新旧同名无意义

    // 2. 校验旧密码（旧名做 salt）
    if (!authenticate(oldName, currentPassword).has_value()) return false;

    // 3. 新名查重
    if (findByUsername(newName).has_value()) return false;

    // 4. 新哈希（salt = newName，密码沿用 currentPassword）
    const std::string newHash = hashPassword(newName, currentPassword);

    db_.executeStmt(
        "UPDATE users SET username=?, password_hash=? WHERE id=?;",
        { newName, newHash, userId });
    return true;
}

bool UserDAO::updatePassword(std::int64_t userId, const std::string& oldPwd,
                             const std::string& newPwd) {
    // 1. 取当前用户名（保持 salt 不变）
    const std::string name = usernameById(db_, userId);
    if (name.empty()) return false;

    // 2. 校验旧密码
    if (!authenticate(name, oldPwd).has_value()) return false;

    // 3. 新哈希（salt = 旧名）
    db_.executeStmt(
        "UPDATE users SET password_hash=? WHERE id=?;",
        { hashPassword(name, newPwd), userId });
    return true;
}
