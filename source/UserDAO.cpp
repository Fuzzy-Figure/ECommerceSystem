#include "UserDAO.h"
#include <functional>
#include <sstream>

UserDAO::UserDAO(Database& db) : db_(db) {}

std::string UserDAO::hashPassword(const std::string& username, const std::string& password) {
    // 简单 hash：username 做 salt，拼接后取 std::hash
    // 课设够用，非密码学安全；同编译器同平台结果一致
    const std::string salted = username + ":" + password;
    const auto h = std::hash<std::string>{}(salted);
    std::ostringstream ss;
    ss << h;
    return ss.str();
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
}

std::optional<User> UserDAO::findByUsername(const std::string& username) {
    // 用单引号转义用户名中的单引号
    std::string escaped;
    escaped.reserve(username.size());
    for (char c : username) { escaped += (c == '\'' ? "''" : std::string(1, c)); }
    std::ostringstream q;
    q << "SELECT id, username, role FROM users WHERE username='" << escaped << "';";
    auto rows = db_.query(q.str());
    if (rows.empty()) return std::nullopt;
    User u;
    u.id       = toInt64(rows.front()["id"]);
    u.username = toStr(rows.front()["username"]);
    u.role     = static_cast<std::int32_t>(toInt64(rows.front()["role"]));
    return u;
}

std::optional<User> UserDAO::authenticate(const std::string& username, const std::string& password) {
    std::string escaped;
    escaped.reserve(username.size());
    for (char c : username) { escaped += (c == '\'' ? "''" : std::string(1, c)); }
    const std::string hash = hashPassword(username, password);
    std::ostringstream q;
    q << "SELECT id, username, role FROM users WHERE username='" << escaped
      << "' AND password_hash='" << hash << "';";
    auto rows = db_.query(q.str());
    if (rows.empty()) return std::nullopt;
    User u;
    u.id       = toInt64(rows.front()["id"]);
    u.username = toStr(rows.front()["username"]);
    u.role     = static_cast<std::int32_t>(toInt64(rows.front()["role"]));
    return u;
}

std::vector<User> UserDAO::findAll() {
    std::vector<User> users;
    auto rows = db_.query("SELECT id, username, role FROM users ORDER BY id;");
    users.reserve(rows.size());
    for (const auto& r : rows) {
        User u;
        u.id       = toInt64(r["id"]);
        u.username = toStr(r["username"]);
        u.role     = static_cast<std::int32_t>(toInt64(r["role"]));
        users.push_back(std::move(u));
    }
    return users;
}

bool UserDAO::createUser(const std::string& username, const std::string& password, std::int64_t& newIdOut) {
    if (findByUsername(username).has_value()) return false;  // 用户名已存在

    std::string escapedUser, escapedHash;
    escapedUser.reserve(username.size());
    escapedHash.reserve(hashPassword(username, password).size());
    for (char c : username) { escapedUser += (c == '\'' ? "''" : std::string(1, c)); }
    const std::string hash = hashPassword(username, password);
    for (char c : hash) { escapedHash += (c == '\'' ? "''" : std::string(1, c)); }

    // 新注册用户默认 role=0（普通用户），商家角色由种子数据或后台指定
    std::ostringstream ins;
    ins << "INSERT INTO users (username, password_hash, role) VALUES ('"
        << escapedUser << "', '" << escapedHash << "', 0);";
    db_.execute(ins.str());

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
        std::ostringstream q;
        q << "SELECT username FROM users WHERE id=" << userId << ";";
        auto rows = db.query(q.str());
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

    // 转义单引号
    auto escape = [](const std::string& s) {
        std::string out; out.reserve(s.size());
        for (char c : s) out += (c == '\'' ? "''" : std::string(1, c));
        return out;
    };
    const std::string escapedName = escape(newName);
    const std::string escapedHash = escape(newHash);

    std::ostringstream upd;
    upd << "UPDATE users SET username='" << escapedName
        << "', password_hash='" << escapedHash
        << "' WHERE id=" << userId << ";";
    db_.execute(upd.str());
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
    const std::string newHash = hashPassword(name, newPwd);
    auto escape = [](const std::string& s) {
        std::string out; out.reserve(s.size());
        for (char c : s) out += (c == '\'' ? "''" : std::string(1, c));
        return out;
    };
    const std::string escapedHash = escape(newHash);

    std::ostringstream upd;
    upd << "UPDATE users SET password_hash='" << escapedHash
        << "' WHERE id=" << userId << ";";
    db_.execute(upd.str());
    return true;
}
