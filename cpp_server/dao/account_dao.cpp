#include "dao/account_dao.h"
#include "dao/database.h"

namespace account_dao {

double get_cash_balance(const std::string& account) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT cash_balance FROM accounts WHERE account = ?;";
    
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);

    double balance = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        balance = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return balance;
}

bool deposit(const std::string& account, double amount) {
    sqlite3* db = Database::instance().get();
    const char* sql = "UPDATE accounts SET cash_balance = cash_balance + ? "
                      "WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, amount);
    sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool withdraw(const std::string& account, double amount) {
    sqlite3* db = Database::instance().get();
    const char* sql = "UPDATE accounts SET cash_balance = cash_balance - ? "
                      "WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, amount);
    sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool init_account(const std::string& account) {
    sqlite3* db = Database::instance().get();
    const char* sql = "INSERT INTO accounts (account, cash_balance) VALUES (?, 0);";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

}  // namespace account_dao