#include "dao/prediction_dao.h"
#include "dao/database.h"
#include <iostream>
#include <cstdlib>

namespace prediction_dao {

void insert(const Prediction& p) {
    sqlite3* db = Database::instance().get();
    const char* sql = "INSERT INTO predictions(stock_code,stock_name,day_offset,predicted_rate,confidence,price_now) "
                      "VALUES(?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, p.stock_code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, p.stock_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, p.day_offset);
    sqlite3_bind_double(stmt, 4, p.predicted_rate);
    sqlite3_bind_double(stmt, 5, p.confidence);
    sqlite3_bind_double(stmt, 6, p.price_now);
    if (sqlite3_step(stmt) != SQLITE_DONE)
        std::cerr << "INSERT预测失败: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(stmt);
}

std::vector<Prediction> get_for_code(const std::string& code) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id,stock_code,stock_name,day_offset,predicted_rate,confidence,"
                      "price_now,actual_price,actual_rate,status,created_at "
                      "FROM predictions WHERE stock_code=? ORDER BY id DESC LIMIT 5;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<Prediction> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Prediction p;
        p.id = sqlite3_column_int64(stmt, 0);
        const char* s = (const char*)sqlite3_column_text(stmt, 1); if (s) p.stock_code = s;
        s = (const char*)sqlite3_column_text(stmt, 2); if (s) p.stock_name = s;
        p.day_offset = sqlite3_column_int(stmt, 3);
        p.predicted_rate = sqlite3_column_double(stmt, 4);
        p.confidence = sqlite3_column_double(stmt, 5);
        p.price_now = sqlite3_column_double(stmt, 6);
        p.actual_price = sqlite3_column_double(stmt, 7);
        p.actual_rate = sqlite3_column_double(stmt, 8);
        s = (const char*)sqlite3_column_text(stmt, 9); if (s) p.status = s;
        s = (const char*)sqlite3_column_text(stmt, 10); if (s) p.created_at = s;
        result.push_back(p);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Prediction> get_pending(int day_offset) {
    sqlite3* db = Database::instance().get();
    const char* sql = "SELECT id,stock_code,stock_name,day_offset,predicted_rate,confidence,"
                      "price_now,actual_price,actual_rate,status,created_at "
                      "FROM predictions WHERE status='pending' AND day_offset=?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, day_offset);
    std::vector<Prediction> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Prediction p;
        p.id = sqlite3_column_int64(stmt, 0);
        const char* s = (const char*)sqlite3_column_text(stmt, 1); if (s) p.stock_code = s;
        s = (const char*)sqlite3_column_text(stmt, 2); if (s) p.stock_name = s;
        p.day_offset = sqlite3_column_int(stmt, 3);
        p.predicted_rate = sqlite3_column_double(stmt, 4);
        p.confidence = sqlite3_column_double(stmt, 5);
        p.price_now = sqlite3_column_double(stmt, 6);
        p.actual_price = sqlite3_column_double(stmt, 7);
        p.actual_rate = sqlite3_column_double(stmt, 8);
        s = (const char*)sqlite3_column_text(stmt, 9); if (s) p.status = s;
        s = (const char*)sqlite3_column_text(stmt, 10); if (s) p.created_at = s;
        result.push_back(p);
    }
    sqlite3_finalize(stmt);
    return result;
}

void verify(int id, double actual_price) {
    sqlite3* db = Database::instance().get();
    auto preds = get_for_code("");
    double price_now = 0, predicted_rate = 0;
    for (const auto& pp : preds) if (pp.id == id) { price_now = pp.price_now; predicted_rate = pp.predicted_rate; break; }
    double actual_rate = price_now > 0 ? (actual_price - price_now) / price_now * 100.0 : 0;

    const char* sql = "UPDATE predictions SET actual_price=?,actual_rate=?,status='verified' WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, actual_price);
    sqlite3_bind_double(stmt, 2, actual_rate);
    sqlite3_bind_int64(stmt, 3, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

}  // namespace prediction_dao