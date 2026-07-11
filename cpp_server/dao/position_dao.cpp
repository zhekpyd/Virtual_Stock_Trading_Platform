#include "dao/position_dao.h"
#include "dao/database.h"

namespace position_dao {

std::vector<Position> get_positions(const std::string& account) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id, account, stock_name, quantity "
                      "FROM positions WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<Position> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Position pos;
        pos.id        = sqlite3_column_int64(stmt, 0);
        pos.account   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        pos.stock_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        pos.quantity  = sqlite3_column_int(stmt, 3);
        result.push_back(pos);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool buy_stock(const std::string& account, const std::string& stock_name, int quantity) {
    sqlite3* db = Database::instance().get();

    // 先看看是否已有持仓
    sqlite3_stmt* check = nullptr;
    const char* check_sql = "SELECT quantity FROM positions WHERE account = ? AND stock_name = ?;";
    sqlite3_prepare_v2(db, check_sql, -1, &check, nullptr);
    sqlite3_bind_text(check, 1, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(check, 2, stock_name.c_str(), -1, SQLITE_TRANSIENT);

    bool has_position = (sqlite3_step(check) == SQLITE_ROW);
    sqlite3_finalize(check);

    if (has_position) {
        // 已有持仓 → 加仓
        const char* sql = "UPDATE positions SET quantity = quantity + ? "
                          "WHERE account = ? AND stock_name = ?;";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, quantity);
        sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, stock_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        // 新建持仓
        const char* sql = "INSERT INTO positions (account, stock_name, quantity) "
                          "VALUES (?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, stock_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, quantity);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return true;
}

bool sell_stock(const std::string& account, const std::string& stock_name, int quantity) {
    sqlite3* db = Database::instance().get();
    const char* sql = "UPDATE positions SET quantity = quantity - ? "
                      "WHERE account = ? AND stock_name = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, stock_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return true;
}

}  // namespace position_dao