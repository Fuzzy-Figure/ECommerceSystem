#include "ServerView.h"
#include <print>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace {
	// 当前时间字符串，便于日志阅读
	std::string nowStr() {
		const auto now = std::chrono::system_clock::now();
		const auto t = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
		localtime_s(&tm, &t);
		std::ostringstream ss;
		ss << std::put_time(&tm, "%H:%M:%S");
		return ss.str();
	}
}

void ServerView::showRequest(const std::string& clientAddr, const std::string& action) {
	std::println("[{}][REQ] {} -> {}", nowStr(), clientAddr, action);
}

void ServerView::showResponse(const std::string& clientAddr, const std::string& result) {
	std::println("[{}][RSP] {} <- {}", nowStr(), clientAddr, result);
}

void ServerView::showError(const std::string& msg) {
	std::println("[{}][ERR] {}", nowStr(), msg);
}
