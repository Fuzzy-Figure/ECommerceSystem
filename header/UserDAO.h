#pragma once
// 用户持久层：按用户名查找、创建新用户。
// 密码不存明文，用 std::hash 拼接 username 做 salt 简单哈希（课设够用）。
#include "Database.h"
#include "User.h"
#include <cstdint>
#include <string>
#include <optional>

class UserDAO {
public:
    explicit UserDAO(Database& db);

    // 按用户名查找：返回 id + username；不存在返回 nullopt
    std::optional<User> findByUsername(const std::string& username);

    // 校验密码：username + password 拼接做 hash，与库中 password_hash 比对
    // 成功返回 User（不含密码），失败返回 nullopt
    std::optional<User> authenticate(const std::string& username, const std::string& password);

    // 创建新用户；用户名已存在返回 false
    bool createUser(const std::string& username, const std::string& password, std::int64_t& newIdOut);

    // 单独生成密码哈希（供 createUser 内部用）
    static std::string hashPassword(const std::string& username, const std::string& password);

private:
    Database& db_;
};
