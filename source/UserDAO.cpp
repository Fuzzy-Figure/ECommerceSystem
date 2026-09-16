#include "../header/UserDAO.h"
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
    q << "SELECT id, username FROM users WHERE username='" << escaped << "';";
    auto rows = db_.query(q.str());
    if (rows.empty()) return std::nullopt;
    User u;
    u.id       = toInt64(rows.front()["id"]);
    u.username = toStr(rows.front()["username"]);
    return u;
}

std::optional<User> UserDAO::authenticate(const std::string& username, const std::string& password) {
    std::string escaped;
    escaped.reserve(username.size());
    for (char c : username) { escaped += (c == '\'' ? "''" : std::string(1, c)); }
    const std::string hash = hashPassword(username, password);
    std::ostringstream q;
    q << "SELECT id, username FROM users WHERE username='" << escaped
      << "' AND password_hash='" << hash << "';";
    auto rows = db_.query(q.str());
    if (rows.empty()) return std::nullopt;
    User u;
    u.id       = toInt64(rows.front()["id"]);
    u.username = toStr(rows.front()["username"]);
    return u;
}

bool UserDAO::createUser(const std::string& username, const std::string& password, std::int64_t& newIdOut) {
    if (findByUsername(username).has_value()) return false;  // 用户名已存在

    std::string escapedUser, escapedHash;
    escapedUser.reserve(username.size());
    escapedHash.reserve(hashPassword(username, password).size());
    for (char c : username) { escapedUser += (c == '\'' ? "''" : std::string(1, c)); }
    const std::string hash = hashPassword(username, password);
    for (char c : hash) { escapedHash += (c == '\'' ? "''" : std::string(1, c)); }

    std::ostringstream ins;
    ins << "INSERT INTO users (username, password_hash) VALUES ('"
        << escapedUser << "', '" << escapedHash << "');";
    db_.execute(ins.str());

    auto idRows = db_.query("SELECT last_insert_rowid() AS id;");
    if (!idRows.empty()) {
        newIdOut = toInt64(idRows.front()["id"]);
        return newIdOut > 0;
    }
    return false;
}
