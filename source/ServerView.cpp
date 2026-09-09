#include "../header/ServerView.h"
#include <iostream>
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
	std::cout << "[" << nowStr() << "][REQ ] " << clientAddr << " -> " << action << std::endl;
}

void ServerView::showResponse(const std::string& clientAddr, const std::string& result) {
	std::cout << "[" << nowStr() << "][RSP ] " << clientAddr << " <- " << result << std::endl;
}

void ServerView::showError(const std::string& msg) {
	std::cout << "[" << nowStr() << "][ERR ] " << msg << std::endl;
}
