#include "service/stock_fetcher.h"
#include "utils/http_client.h"
#include "dao/database.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <windows.h>

static double safe_double(const nlohmann::json& item, const std::string& key) {
    if (item.contains(key) && item[key].is_number()) {
        return item[key].get<double>();
    }
    return 0.0;
}

static int64_t safe_int64(const nlohmann::json& item, const std::string& key) {
    if (item.contains(key) && item[key].is_number()) {
        return item[key].get<int64_t>();
    }
    return 0;
}

void fetch_and_update_stocks() {
    std::cout << "开始抓取全部股票数据..." << std::endl;

    sqlite3* db = Database::instance().get();
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    int page = 1;
    int total_fetched = 0;

    while (true) {
        std::string url = "http://push2.eastmoney.com/api/qt/clist/get?"
                          "pn=" + std::to_string(page) + "&pz=200&po=1&np=1&fltt=2&invt=2&fid=f3&fs=m:0+t:6&"
                          "fields=f2,f3,f5,f14,f15,f16";

        std::string data = http_get(url);
        if (data.empty()) break;

        auto json = nlohmann::json::parse(data);
        auto list = json["data"]["diff"];
        if (list.empty()) break;  // 没更多数据了

        for (const auto& item : list) {
            std::string name  = item.value("f14", "");
            double price      = safe_double(item, "f2");
            double change_pct = safe_double(item, "f3");
            double high_day   = safe_double(item, "f15");
            double low_day    = safe_double(item, "f16");
            int64_t volume    = safe_int64(item, "f5");


            const char* sql = "INSERT INTO stocks (name, price, change_pct, high_day, low_day, volume) "
                              "VALUES (?, ?, ?, ?, ?, ?) "
                              "ON CONFLICT(name) DO UPDATE SET "
                              "price=excluded.price, change_pct=excluded.change_pct, "
                              "high_day=excluded.high_day, low_day=excluded.low_day, "
                              "volume=excluded.volume;";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) continue;
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 2, price);
            sqlite3_bind_double(stmt, 3, change_pct);
            sqlite3_bind_double(stmt, 4, high_day);
            sqlite3_bind_double(stmt, 5, low_day);
            sqlite3_bind_int64(stmt, 6, volume);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            total_fetched++;
        }

        std::cout << "第 " << page << " 页完成" << std::endl;
        page++;
        Sleep(500);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    std::cout << "全部完成！共获取 " << total_fetched << " 只股票" << std::endl;
}