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

void insert_trade(const std::string& account, const std::string& stock_name,
                  const std::string& type, int quantity, double price,
                  double total_amount) {
    sqlite3* db = Database::instance().get();
    const char* sql = "INSERT INTO trades (account, stock_name, type, quantity, price, total_amount) "
                      "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, stock_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, quantity);
    sqlite3_bind_double(stmt, 5, price);
    sqlite3_bind_double(stmt, 6, total_amount);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<Trade> get_trades(const std::string& account) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id, account, stock_name, type, quantity, price, "
                      "total_amount, status, created_at "
                      "FROM trades WHERE account = ? ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<Trade> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Trade t;
        t.id       = sqlite3_column_int64(stmt, 0);
        t.account  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.stock_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.type     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.quantity = sqlite3_column_int(stmt, 4);
        t.price    = sqlite3_column_double(stmt, 5);
        t.total_amount = sqlite3_column_double(stmt, 6);
        t.status   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        t.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result.push_back(t);
    }
    sqlite3_finalize(stmt);
    return result;
}


}  // namespace position_dao