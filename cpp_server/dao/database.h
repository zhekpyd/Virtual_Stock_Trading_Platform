#pragma once
#include <sqlite3.h>
#include <string>

class Database {
public:
    static Database& instance() {
        static Database db;
        return db;
    }

    bool initialize(const std::string& db_path) {
        return sqlite3_open(db_path.c_str(), &db_) == SQLITE_OK;
    }

    sqlite3* get() { return db_; }

private:
    Database() = default;
    sqlite3* db_ = nullptr;
};