#include "service/stock_fetcher.h"
#include "service/kline_fetcher.h"
#include "utils/http_client.h"
#include "dao/database.h"
#include "dao/stock_dao.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <vector>

// ---- 解析 Sina 单行数据 ----
// 格式: var hq_str_sh600519="name,open,prev_close,price,high,low,...";
static void parse_sina_line(const std::string& line,
                             std::string& code, std::string& name,
                             double& price, double& high, double& low, double& change_pct,
                             int64_t& volume) {
    // 取股票代码
    size_t eq = line.find('=');
    if (eq == std::string::npos) return;
    std::string left = line.substr(0, eq);  // var hq_str_sh600519
    size_t last_underscore = left.rfind('_');
    if (last_underscore == std::string::npos) return;
    code = left.substr(last_underscore + 1);  // sh600519

    // 取引号内的数据
    size_t q1 = line.find('"', eq);
    size_t q2 = line.find('"', q1 + 1);
    if (q1 == std::string::npos || q2 == std::string::npos) return;
    std::string data = line.substr(q1 + 1, q2 - q1 - 1);

    // 按逗号分割
    std::vector<std::string> fields;
    std::stringstream ss(data);
    std::string f;
    while (std::getline(ss, f, ',')) fields.push_back(f);

    if (fields.size() < 6) return;
    name  = fields[0];
    price = fields[3].empty() ? 0 : std::stod(fields[3]);
    high  = fields[4].empty() ? 0 : std::stod(fields[4]);
    low   = fields[5].empty() ? 0 : std::stod(fields[5]);
    volume = fields[8].empty() ? 0 : std::stoll(fields[8]);

    // 涨跌幅: (现价-昨收)/昨收 * 100
    double prev_close = fields[2].empty() ? price : std::stod(fields[2]);
    if (prev_close > 0) change_pct = (price - prev_close) / prev_close * 100;
}

void fetch_and_update_stocks() {
    std::cout << "=== 股票数据更新 ===" << std::endl;

    sqlite3* db = Database::instance().get();

    // ---- Phase 1: 从本地库读取所有已有股票代码 ----
    std::vector<std::string> codes;
    {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, "SELECT code FROM stocks WHERE code != '';", -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* c = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (c && c[0]) codes.push_back(c);
        }
        sqlite3_finalize(stmt);
    }

    // 如果本地没有股票代码，先从东方财富拉取代码列表（只取 f12,f14）
    std::cout << "本地已有 " << codes.size() << " 个股票代码" << std::endl;

    if (codes.empty()) {
        std::cout << "本地无股票数据，从东方财富拉取代码列表..." << std::endl;
        int page = 1;
        while (true) {
            std::string url = "http://push2.eastmoney.com/api/qt/clist/get?"
                              "pn=" + std::to_string(page) + "&pz=200&po=1&np=1&fltt=2&invt=2&fid=f3&fs=m:0+t:6&"
                              "fields=f12,f14,f20";
            std::string data = http_get(url);
            if (data.empty()) {
                std::cout << "第" << page << "页失败，停止拉取代码" << std::endl;
                break;
            }
            auto json = nlohmann::json::parse(data);
            auto list = json["data"]["diff"];
            if (list.empty()) break;

            sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
            for (const auto& item : list) {
                std::string code = item.value("f12", "");
                std::string name = item.value("f14", "");
                if (code.empty() || name.empty()) continue;
                codes.push_back(code);

                // 先插入代码和名字，价格后面 Sina 补
                const char* sql = "INSERT INTO stocks (name, code) VALUES (?, ?) "
                                  "ON CONFLICT(name) DO NOTHING;";
                sqlite3_stmt* stmt = nullptr;
                sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
                sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, code.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
            std::cout << "代码列表第 " << page << " 页完成 (" << codes.size() << " 只)" << std::endl;
            page++;
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        }
    }

    if (codes.empty()) {
        std::cout << "没有任何股票代码，退出" << std::endl;
        return;
    }

    // ---- Phase 2: 用 Sina 批量拉价格 (每批 100 只) ----
    std::cout << "共 " << codes.size() << " 只股票，开始用 Sina 拉取价格..." << std::endl;
    int updated = 0;

    for (size_t i = 0; i < codes.size(); i += 100) {
        // 构建 Sina 请求 URL: sh600519,sz000001,...
        std::string batch;
        size_t end = std::min(i + 100, codes.size());
        for (size_t j = i; j < end; j++) {
            if (j > i) batch += ",";
            // 6 开头 = 上海，其他 = 深圳
            if (codes[j][0] == '6') batch += "sh" + codes[j];
            else batch += "sz" + codes[j];
        }

        std::string url = "http://hq.sinajs.cn/list=" + batch;
        std::string data = http_get(url);
        std::cout << "Sina 第 " << (i/100 + 1) << " 批: " << data.length() << " 字节" << std::endl;

        if (data.empty()) {
            std::cout << "Sina 第 " << (i/100 + 1) << " 批失败，跳过" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // 解析每行数据并更新数据库
        sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

        std::stringstream ss(data);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty() || line.find("hq_str_") == std::string::npos) continue;

            std::string code, name;
            double price = 0, high = 0, low = 0, change = 0;
            int64_t volume = 0;
            parse_sina_line(line, code, name, price, high, low, change, volume);

            if (code.empty()) continue;

            // 用 ON CONFLICT(code) 更新价格——需要在 code 列加 UNIQUE
            const char* sql = "UPDATE stocks SET "
                              "name=?, price=?, change_pct=?, high_day=?, low_day=?, volume=?, "
                              "high_hist=COALESCE(high_hist, ?), "
                              "low_hist=COALESCE(low_hist, ?) "
                              "WHERE code=?;";
            sqlite3_stmt* stmt = nullptr;
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 2, price);
            sqlite3_bind_double(stmt, 3, change);
            sqlite3_bind_double(stmt, 4, high);
            sqlite3_bind_double(stmt, 5, low);
            sqlite3_bind_int64(stmt, 6, volume);
            sqlite3_bind_double(stmt, 7, high);   // COALESCE(high_hist, ?)
            sqlite3_bind_double(stmt, 8, low);    // COALESCE(low_hist, ?)
            sqlite3_bind_text(stmt, 9, code.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            updated++;
        }

        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
        std::cout << "Sina 第 " << (i/100 + 1) << " 批完成 (" << updated << " 只已更新)" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));  // Sina 比较宽容，0.3 秒够了
    }

    // ---- Phase 3: 从已有 K 线数据批量更新历史最高最低 ----
    std::cout << "更新历史最高最低..." << std::endl;
    sqlite3_exec(db,
        "UPDATE stocks SET "
        "  high_hist = (SELECT MAX(high_price) FROM stock_history WHERE stock_name = stocks.name),"
        "  low_hist  = (SELECT MIN(low_price)  FROM stock_history WHERE stock_name = stocks.name) "
        "WHERE EXISTS (SELECT 1 FROM stock_history WHERE stock_name = stocks.name);",
        nullptr, nullptr, nullptr);
    std::cout << "历史最高最低更新完成" << std::endl;

    // ---- Phase 4: 给没有历史数据的股票拉取 K 线（每次最多 20 只） ----
    {
        std::vector<std::string> need_kline;
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db,
            "SELECT s.code FROM stocks s "
            "WHERE s.code != '' "
            "AND NOT EXISTS (SELECT 1 FROM stock_history h WHERE h.stock_name = s.name) "
            "LIMIT 100;",
            -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* c = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (c) need_kline.push_back(c);
        }
        sqlite3_finalize(stmt);

        if (!need_kline.empty()) {
            std::cout << "为 " << need_kline.size() << " 只股票拉取历史K线..." << std::endl;
            for (const auto& cd : need_kline) {
                std::string stock_name = cd;
                // 找股票名
                auto all = stock_dao::get_all_stocks();
                for (const auto& s : all) {
                    if (s.code == cd) { stock_name = s.name; break; }
                }
                auto klines = fetch_kline(cd, stock_name, "daily", "20250101");
                if (!klines.empty()) {
                    stock_dao::insert_history(stock_name, klines);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));  // 温柔对待 API
            }
            // 重新计算
            sqlite3_exec(db,
                "UPDATE stocks SET "
                "  high_hist = (SELECT MAX(high_price) FROM stock_history WHERE stock_name = stocks.name),"
                "  low_hist  = (SELECT MIN(low_price)  FROM stock_history WHERE stock_name = stocks.name) "
                "WHERE EXISTS (SELECT 1 FROM stock_history WHERE stock_name = stocks.name);",
                nullptr, nullptr, nullptr);
            std::cout << "K 线拉取完成，历史高低已更新" << std::endl;
        }
    }

    std::cout << "股票数据更新完成！共更新 " << updated << " 只股票" << std::endl;
}
