#include "dao/user_dao.h"
#include "dao/database.h"
#include <iostream>

namespace user_dao {

// ========== 插入用户 ==========
int64_t insert_user(const std::string& account,
                    const std::string& username,
                    const std::string& password,
                    const std::string& phone) {
    sqlite3* db = Database::instance().get();

    const char* sql = "INSERT INTO users (account, username, password, phone) "
                      "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, account.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, phone.c_str(),    -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cout << "SQL INSERT 错误: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db);
}

// ========== 检查账号是否已存在 ==========
bool account_exists(const std::string& account) {
    sqlite3* db = Database::instance().get();

    const char* sql = "SELECT COUNT(*) FROM users WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        exists = (count > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
}

std::string get_password(const std::string& account) {
    sqlite3* db = Database::instance().get();

    const char* sql = "SELECT password FROM users WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);

    std::string password;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // 查到了！取第 0 列（password）的文字值
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text) {
            password = text;
        }
    }
    // 没查到 → password 就是空字符串

    sqlite3_finalize(stmt);
    return password;
}

User get_user_by_account(const std::string& account) {
    sqlite3* db = Database::instance().get();

    const char* sql = "SELECT id, account, username, password, phone FROM users WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);

    User user{};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id       = sqlite3_column_int64(stmt, 0);
        user.account  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.phone    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }
    sqlite3_finalize(stmt);
    return user;
}

bool update_username(const std::string& account, const std::string& new_username) {
    sqlite3* db = Database::instance().get();

    const char* sql = "UPDATE users SET username = ? WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, new_username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool update_password(const std::string& account, const std::string& new_password) {
    sqlite3* db = Database::instance().get();

    const char* sql = "UPDATE users SET password = ? WHERE account = ?;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, new_password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

}  // namespace user_dao