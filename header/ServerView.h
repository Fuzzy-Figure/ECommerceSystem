#pragma once
// 服务端 View：纯控制台日志输出（无 GUI）
// PPTX 第7页要求服务端"业务层+持久层"，View 仅做交易/请求日志展示。
#include <string>

class ServerView {
public:
    // 显示来自某客户端的请求（已序列化）
    void showRequest(const std::string& clientAddr, const std::string& action);
    // 显示回送响应
    void showResponse(const std::string& clientAddr, const std::string& result);
    // 显示错误
    void showError(const std::string& msg);
};
