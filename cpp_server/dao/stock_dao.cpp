#include "dao/stock_dao.h"
#include "dao/database.h"

namespace stock_dao {

Stock get_stock_by_name(const std::string& name) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id, name, price, change_pct, high_day, "
                      "low_day, high_hist, low_hist, volume, total_shares "
                      "FROM stocks WHERE name = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    Stock stock{};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stock.id           = sqlite3_column_int64(stmt, 0);
        stock.name         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        stock.price        = sqlite3_column_double(stmt, 2);
        stock.change_pct   = sqlite3_column_double(stmt, 3);
        stock.high_day     = sqlite3_column_double(stmt, 4);
        stock.low_day      = sqlite3_column_double(stmt, 5);
        stock.high_hist    = sqlite3_column_double(stmt, 6);
        stock.low_hist     = sqlite3_column_double(stmt, 7);
        stock.volume       = sqlite3_column_int64(stmt, 8);
        stock.total_shares = sqlite3_column_int64(stmt, 9);
    }
    sqlite3_finalize(stmt);
    return stock;
}

std::vector<Stock> get_all_stocks() {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id, name, price, change_pct, high_day, "
                      "low_day, high_hist, low_hist, volume, total_shares "
                      "FROM stocks;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    std::vector<Stock> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Stock s;
        s.id           = sqlite3_column_int64(stmt, 0);
        s.name         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        s.price        = sqlite3_column_double(stmt, 2);
        s.change_pct   = sqlite3_column_double(stmt, 3);
        s.high_day     = sqlite3_column_double(stmt, 4);
        s.low_day      = sqlite3_column_double(stmt, 5);
        s.high_hist    = sqlite3_column_double(stmt, 6);
        s.low_hist     = sqlite3_column_double(stmt, 7);
        s.volume       = sqlite3_column_int64(stmt, 8);
        s.total_shares = sqlite3_column_int64(stmt, 9);
        result.push_back(s);
    }
    sqlite3_finalize(stmt);
    return result;
}

}  // namespace stock_dao