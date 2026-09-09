#include <SFML/Network.hpp>
#include "../header/utils.h"
#include "../header/Database.h"
#include "../header/ServerController.h"
#include "../header/Protocol.h"

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <Windows.h>

namespace {
    // 创建 schema 并插入种子数据（仅当表为空）
    void initDatabase(Database& db) {
        db.execute(
            "CREATE TABLE IF NOT EXISTS products ("
            "  id          INTEGER PRIMARY KEY,"
            "  name        TEXT NOT NULL,"
            "  description TEXT,"
            "  price       REAL NOT NULL,"
            "  stock       INTEGER NOT NULL DEFAULT 0,"
            "  imagePath   TEXT"
            ");"
        );

        auto rows = db.query("SELECT COUNT(*) AS cnt FROM products;");
        int existing = 0;
        if (!rows.empty()) {
            const auto& v = rows.front()["cnt"];
            if (v.is_number()) existing = v.get<int>();
            else if (v.is_string()) {
                try { existing = std::stoi(v.get<std::string>()); } catch (...) {}
            }
        }
        if (existing > 0) {
            std::cout << "[Init] 数据库已有 " << existing << " 件商品" << std::endl;
            return;
        }

        db.execute("DELETE FROM products;");
        db.execute(
            "INSERT INTO products (id, name, description, price, stock, imagePath) VALUES "
            "(1, '绿茶',     '清香型绿茶 250g 礼盒', 9.9,  100, 'images/绿茶.png'),"
            "(2, '红茶',     '红茶礼盒装 200g',      19.9,  80, 'images/红茶.png'),"
            "(3, '茉莉花茶', '茉莉花茶 100g 罐装',   14.5,  60, 'images/茉莉花茶.png'),"
            "(4, '乌龙茶',   '高山乌龙茶 150g',      29.9,  50, 'images/乌龙茶.png'),"
            "(5, '普洱茶',   '云南普洱茶饼 357g',    59.0,  30, 'images/普洱茶.png'),"
            "(6, '白茶',     '福鼎白毫银针 100g',    78.0,  20, 'images/白茶.png');"
        );
        std::cout << "[Init] 已插入种子商品数据" << std::endl;
    }
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    // 重定向到文件时强制立即刷新，便于诊断
    std::cout.setf(std::ios_base::unitbuf);
    std::cerr.setf(std::ios_base::unitbuf);
    std::cout << "[Server] 进程启动" << std::endl;

    try {
        ec::reloadServerConfig();
    } catch (const std::exception& e) {
        std::cerr << "加载配置失败：" << e.what() << std::endl;
        return 1;
    }

    const auto& cfg      = ec::getServerConfig();
    const auto  dbPath    = cfg["database"]["path"].get<std::string>();
    const auto  listenPort = cfg["server"]["port"].get<unsigned short>();

    std::unique_ptr<Database> db;
    try {
        db = std::make_unique<Database>(dbPath);
        initDatabase(*db);
    } catch (const std::exception& e) {
        std::cerr << "数据库初始化失败：" << e.what() << std::endl;
        return 1;
    }

    ServerController controller(*db);

    sf::TcpListener listener;
    if (listener.listen(listenPort) != sf::Socket::Status::Done) {
        std::cerr << "[Server] TcpListener 监听端口 " << listenPort << " 失败" << std::endl;
        return 1;
    }
    std::cout << "[Server] 已监听端口 " << listenPort << "，等待客户端连接..." << std::endl;

    std::atomic<bool> running{true};
    while (running.load()) {
        auto sock = std::make_shared<sf::TcpSocket>();
        if (listener.accept(*sock) != sf::Socket::Status::Done) {
            std::cerr << "[Server] accept 失败" << std::endl;
            continue;
        }
        const auto addr = sock->getRemoteAddress();
        std::cout << "[Server] 新客户端连接："
                  << (addr ? addr->toString() : std::string{"unknown"})
                  << ":" << sock->getRemotePort() << std::endl;

        // 每客户端一线程，循环收消息并投递到工作队列
        std::thread([sock, &controller, &running]() {
            while (running.load()) {
                auto req = proto::recvJson(*sock);
                if (!req) {
                    std::cout << "[Server] 客户端断开连接" << std::endl;
                    break;
                }
                controller.submit(sock, std::move(*req));
            }
        }).detach();
    }

    controller.shutdown();
    listener.close();
    return 0;
}
