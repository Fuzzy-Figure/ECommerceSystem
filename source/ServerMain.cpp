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
	// 数据库 schema 版本号：每改一次表结构就 +1，旧库自动重建
	//   v1 = 6 个商品 + orders 表（id/total/created_at 三列）
	//   v2 = orders 加 discount/final_total 列 + promotions 表 + 5 条种子促销
	//   v3 = orders 加 status 列（退货状态）+ order_items 加 returned_qty 列
	constexpr int kSchemaVersion = 3;

	// 创建 schema 并插入种子数据；旧版本库会先 DROP 再重建
	void initDatabase(Database& db) {
		// 检查 user_version：与 kSchemaVersion 不匹配就丢弃旧表
		int userVersion = 0;
		{
			auto rows = db.query("PRAGMA user_version;");
			if (!rows.empty() && rows.front().contains("user_version")) {
				const auto& v = rows.front()["user_version"];
				if (v.is_number()) userVersion = v.get<int>();
				else if (v.is_string()) {
					try { userVersion = std::stoi(v.get<std::string>()); } catch (...) {}
				}
			}
		}
		if (userVersion != kSchemaVersion) {
			std::cout << "[Init] schema 版本 " << userVersion
				<< " ≠ 期望 " << kSchemaVersion
				<< "，丢弃旧表重建" << std::endl;
			// 外键依赖顺序：先子表后父表
			db.execute("DROP TABLE IF EXISTS order_items;");
			db.execute("DROP TABLE IF EXISTS orders;");
			db.execute("DROP TABLE IF EXISTS promotions;");
			db.execute("DROP TABLE IF EXISTS products;");
		}

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
		// 订单主表 + 明细表：结算时由 OrderDAO 在同一事务内写入
		// total=原价合计，discount=促销总折扣，final_total=实付（total-discount）
		// status: 0=正常, 1=部分退货, 2=全部退货
		db.execute(
			"CREATE TABLE IF NOT EXISTS orders ("
			"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  total      REAL NOT NULL,"
			"  discount   REAL NOT NULL DEFAULT 0,"
			"  final_total REAL NOT NULL,"
			"  status     INTEGER NOT NULL DEFAULT 0,"
			"  created_at TEXT NOT NULL"
			");"
		);
		db.execute(
			"CREATE TABLE IF NOT EXISTS order_items ("
			"  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  order_id     INTEGER NOT NULL,"
			"  product_id   INTEGER NOT NULL,"
			"  qty          INTEGER NOT NULL,"
			"  price        REAL NOT NULL,"
			"  returned_qty INTEGER NOT NULL DEFAULT 0,"
			"  FOREIGN KEY (order_id) REFERENCES orders(id)"
			");"
		);
		// 促销配置表：type 取 'discount'/'tiered'/'freeitem'/'reduction'/'coupon'
		// params 是 JSON 字符串，如 {"rate":0.8} 或 {"threshold":100,"reduce":20}
		// enabled=1 启用，0 禁用。柔性：改 DB 即可调整促销策略，无需重编译
		db.execute(
			"CREATE TABLE IF NOT EXISTS promotions ("
			"  id     INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  type   TEXT NOT NULL,"
			"  params TEXT NOT NULL,"
			"  enabled INTEGER NOT NULL DEFAULT 1"
			");"
		);
		// 写入当前 schema 版本号
		db.execute("PRAGMA user_version = " + std::to_string(kSchemaVersion) + ";");

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
		}
		else {
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

		// 促销种子数据：仅当 promotions 表为空时插入
		auto promoRows = db.query("SELECT COUNT(*) AS cnt FROM promotions;");
		int promoExisting = 0;
		if (!promoRows.empty()) {
			const auto& v = promoRows.front()["cnt"];
			if (v.is_number()) promoExisting = v.get<int>();
			else if (v.is_string()) {
				try { promoExisting = std::stoi(v.get<std::string>()); } catch (...) {}
			}
		}
		if (promoExisting > 0) {
			std::cout << "[Init] 数据库已有 " << promoExisting << " 条促销规则" << std::endl;
		}
		else {
			db.execute("DELETE FROM promotions;");
			// 示例促销：覆盖 PPT 第5页全部类型
			//   1. 满减：满 50 减 5
			//   2. 统一折：全场 9 折（rate=0.9）
			//   3. 阶梯折：第2件 9 折、第3件及以后 8 折（params.tiers=[[2,0.9],[3,0.8]]）
			//   4. 免单：买 3 送 1
			//   5. 券：10 元抵扣券
			// 注：params 列存 JSON 字符串字面量，单引号包裹，内部用双引号
			db.execute(
				"INSERT INTO promotions (type, params, enabled) VALUES "
				"('reduction', '{\"threshold\":50,\"reduce\":5}', 1),"
				"('discount',   '{\"rate\":0.9}',                  1),"
				"('tiered',     '{\"tiers\":[[2,0.9],[3,0.8]]}',   1),"
				"('freeitem',   '{\"buyN\":3,\"freeM\":1}',         1),"
				"('coupon',     '{\"amount\":10}',                1);"
			);
			std::cout << "[Init] 已插入种子促销规则" << std::endl;
		}
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

	const auto& cfg = ec::getServerConfig();
	const auto  dbPath = cfg["database"]["path"].get<std::string>();
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

	std::atomic<bool> running{ true };
	while (running.load()) {
		auto sock = std::make_shared<sf::TcpSocket>();
		if (listener.accept(*sock) != sf::Socket::Status::Done) {
			std::cerr << "[Server] accept 失败" << std::endl;
			continue;
		}
		const auto addr = sock->getRemoteAddress();
		std::cout << "[Server] 新客户端连接："
			<< (addr ? addr->toString() : std::string{ "unknown" })
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
