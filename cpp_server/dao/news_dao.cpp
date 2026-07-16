#include "dao/news_dao.h"
#include "dao/database.h"

namespace news_dao {

bool exists(int source_id) {
    auto* db = Database::instance().get();
    auto* stmt = (sqlite3_stmt*)nullptr;
    sqlite3_prepare_v2(db, "SELECT 1 FROM news_analysis WHERE source_id=?;", -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, source_id);
    bool ok = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return ok;
}

void insert(const NewsAnalysis& n) {
    auto* db = Database::instance().get();
    auto* stmt = (sqlite3_stmt*)nullptr;
    sqlite3_prepare_v2(db,
        "INSERT INTO news_analysis(source_id,title,content,tags,summary,stocks_impact) VALUES(?,?,?,?,?,?);",
        -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, n.source_id);
    sqlite3_bind_text(stmt, 2, n.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, n.content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, n.tags.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, n.summary.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, n.stocks_impact.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<NewsAnalysis> get_all() {
    auto* db = Database::instance().get();
    auto* stmt = (sqlite3_stmt*)nullptr;
    sqlite3_prepare_v2(db, "SELECT id,source_id,title,content,tags,summary,created_at FROM news_analysis ORDER BY id DESC LIMIT 100;", -1, &stmt, 0);
    std::vector<NewsAnalysis> v;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        NewsAnalysis n;
        n.id = sqlite3_column_int(stmt, 0);
        n.source_id = sqlite3_column_int(stmt, 1);
        auto* t = (const char*)sqlite3_column_text(stmt, 2); if (t) n.title = t;
        t = (const char*)sqlite3_column_text(stmt, 3); if (t) n.content = t;
        t = (const char*)sqlite3_column_text(stmt, 4); if (t) n.tags = t;
        t = (const char*)sqlite3_column_text(stmt, 5); if (t) n.summary = t;
        t = (const char*)sqlite3_column_text(stmt, 6); if (t) n.created_at = t;
        v.push_back(n);
    }
    sqlite3_finalize(stmt);
    return v;
}

std::vector<NewsAnalysis> get_recent(int hours) {
    sqlite3* db = Database::instance().get();
    std::string sql_str = "SELECT id,source_id,title,content,tags,summary,stocks_impact,created_at "
        "FROM news_analysis WHERE created_at >= datetime('now','localtime','-" +
        std::to_string(hours) + " hours') ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql_str.c_str(), -1, &stmt, nullptr);
    std::vector<NewsAnalysis> v;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        NewsAnalysis n;
        n.id = sqlite3_column_int(stmt, 0);
        n.source_id = sqlite3_column_int(stmt, 1);
        const char* t = (const char*)sqlite3_column_text(stmt, 2); if (t) n.title = t;
        t = (const char*)sqlite3_column_text(stmt, 3); if (t) n.content = t;
        t = (const char*)sqlite3_column_text(stmt, 4); if (t) n.tags = t;
        t = (const char*)sqlite3_column_text(stmt, 5); if (t) n.summary = t;
        t = (const char*)sqlite3_column_text(stmt, 6); if (t) n.created_at = t;
        t = (const char*)sqlite3_column_text(stmt, 7); if (t) n.stocks_impact = t;
        t = (const char*)sqlite3_column_text(stmt, 8); if (t) n.created_at = t;
        v.push_back(n);
    }
    sqlite3_finalize(stmt);
    return v;
}

}