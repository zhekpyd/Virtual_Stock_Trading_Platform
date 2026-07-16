#include "dao/stock_dao.h"
#include "dao/database.h"
#include <ctime>
#include <iostream>

namespace stock_dao {

Stock get_stock_by_name(const std::string& name) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id, name, code, price, change_pct, high_day, "
                      "low_day, high_hist, low_hist, volume, total_shares "
                      "FROM stocks WHERE name = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    Stock stock{};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stock.id           = sqlite3_column_int64(stmt, 0);
        stock.name         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        stock.code         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        stock.price        = sqlite3_column_double(stmt, 3);
        stock.change_pct   = sqlite3_column_double(stmt, 4);
        stock.high_day     = sqlite3_column_double(stmt, 5);
        stock.low_day      = sqlite3_column_double(stmt, 6);
        stock.high_hist    = sqlite3_column_double(stmt, 7);
        stock.low_hist     = sqlite3_column_double(stmt, 8);
        stock.volume       = sqlite3_column_int64(stmt, 9);
        stock.total_shares = sqlite3_column_int64(stmt, 10);
    }
    sqlite3_finalize(stmt);
    return stock;
}

std::vector<Stock> get_all_stocks() {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id, name, code, price, change_pct, high_day, "
                      "low_day, high_hist, low_hist, volume, total_shares "
                      "FROM stocks;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    std::vector<Stock> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Stock s;
        s.id           = sqlite3_column_int64(stmt, 0);
        s.name         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        s.code         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        s.price        = sqlite3_column_double(stmt, 3);
        s.change_pct   = sqlite3_column_double(stmt, 4);
        s.high_day     = sqlite3_column_double(stmt, 5);
        s.low_day      = sqlite3_column_double(stmt, 6);
        s.high_hist    = sqlite3_column_double(stmt, 7);
        s.low_hist     = sqlite3_column_double(stmt, 8);
        s.volume       = sqlite3_column_int64(stmt, 9);
        s.total_shares = sqlite3_column_int64(stmt, 10);
        result.push_back(s);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool has_history(const std::string& code) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT COUNT(*) FROM stock_history WHERE stock_name = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return exists;
}

std::vector<StockHistory> get_history(const std::string& stock_name) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id, stock_name, trade_date, open_price, "
                      "close_price, high_price, low_price, volume "
                      "FROM stock_history WHERE stock_name = ? ORDER BY trade_date;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, stock_name.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<StockHistory> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StockHistory h;
        h.id         = sqlite3_column_int64(stmt, 0);
        const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        h.stock_name = n ? n : "";
        const char* d = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        h.trade_date = d ? d : "";
        h.open_price  = sqlite3_column_double(stmt, 3);
        h.close_price = sqlite3_column_double(stmt, 4);
        h.high_price  = sqlite3_column_double(stmt, 5);
        h.low_price   = sqlite3_column_double(stmt, 6);
        h.volume      = sqlite3_column_int64(stmt, 7);
        result.push_back(h);
    }
    sqlite3_finalize(stmt);
    return result;
}

void insert_history(const std::string& stock_name,
                    const std::vector<StockHistory>& data) {
    sqlite3* db = Database::instance().get();
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    const char* sql = "INSERT INTO stock_history "
                      "(stock_name, trade_date, open_price, close_price, high_price, low_price, volume) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?) "
                      "ON CONFLICT(stock_name, trade_date) DO UPDATE SET "
                      "open_price=excluded.open_price, close_price=excluded.close_price, "
                      "high_price=excluded.high_price, low_price=excluded.low_price, "
                      "volume=excluded.volume;";

    for (const auto& h : data) {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, stock_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, h.trade_date.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 3, h.open_price);
        sqlite3_bind_double(stmt, 4, h.close_price);
        sqlite3_bind_double(stmt, 5, h.high_price);
        sqlite3_bind_double(stmt, 6, h.low_price);
        sqlite3_bind_int64(stmt, 7, h.volume);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "insert_history 失败: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

std::string get_latest_date(const std::string& stock_name) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT MAX(trade_date) FROM stock_history WHERE stock_name = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, stock_name.c_str(), -1, SQLITE_TRANSIENT);

    std::string latest;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* d = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (d) latest = d;
    }
    sqlite3_finalize(stmt);
    return latest;
}

void clear_history(const std::string& stock_name) {
    sqlite3* db = Database::instance().get();
    const char* sql = "DELETE FROM stock_history WHERE stock_name = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, stock_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

}  // namespace stock_dao