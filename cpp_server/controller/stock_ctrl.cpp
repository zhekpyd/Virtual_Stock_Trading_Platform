#include "controller/stock_ctrl.h"
#include "dao/database.h"
#include "dao/stock_dao.h"
#include "dao/account_dao.h"
#include "dao/position_dao.h"
#include "dao/user_dao.h"
#include "service/kline_fetcher.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <ctime>
#include <cstdio>

namespace stock_ctrl {

// ===== 股票行情 =====
crow::response get_market(const crow::request& req) {
    auto stocks = stock_dao::get_all_stocks();

    nlohmann::json stocks_json = nlohmann::json::array();
    for (const auto& s : stocks) {
        nlohmann::json item;
        item["code"]         = s.code;
        item["name"]         = s.name;
        item["price"]        = s.price;
        item["change"]       = s.change_pct;
        item["high_day"]     = s.high_day;
        item["low_day"]      = s.low_day;
        item["high_hist"]    = s.high_hist;
        item["low_hist"]     = s.low_hist;
        item["volume"]       = s.volume;
        item["total_shares"] = s.total_shares;
        stocks_json.push_back(item);
    }

    nlohmann::json resp;
    resp["success"] = true;
    resp["stocks"]  = stocks_json;
    return crow::response(200, resp.dump());
}

// ===== 买入 =====
crow::response buy_stock(const crow::request& req) {
    auto body = nlohmann::json::parse(req.body);
    std::string account   = body.value("account", "");
    std::string stock_name = body.value("stock_name", "");
    int quantity           = body.value("quantity", 0);
    double price           = body.value("price", 0.0);
    std::string password   = body.value("password", "");

    // 验证密码
    std::string db_pwd = user_dao::get_password(account);
    if (db_pwd.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "账号不存在";
        return crow::response(400, resp.dump());
    }
    if (password != db_pwd) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "密码错误";
        return crow::response(400, resp.dump());
    }

    // 检查余额
    double total = price * quantity;
    double balance = account_dao::get_cash_balance(account);
    if (balance < total) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "余额不足";
        return crow::response(400, resp.dump());
    }

    // 扣款
    account_dao::withdraw(account, total);

    // 增加持仓
    position_dao::buy_stock(account, stock_name, quantity);
    position_dao::insert_trade(account, stock_name, "买入", quantity, price, total);

    nlohmann::json resp;
    resp["success"] = true;
    resp["message"] = "成功买入 " + stock_name + " " + std::to_string(quantity) + " 股，花费 " + std::to_string(total) + " 元";
    return crow::response(200, resp.dump());
}

// ===== 卖出 =====
crow::response sell_stock(const crow::request& req) {
    auto body = nlohmann::json::parse(req.body);
    std::string account   = body.value("account", "");
    std::string stock_name = body.value("stock_name", "");
    int quantity           = body.value("quantity", 0);
    double price           = body.value("price", 0.0);
    std::string password   = body.value("password", "");

    // 验证密码
    std::string db_pwd = user_dao::get_password(account);
    if (db_pwd.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "账号不存在";
        return crow::response(400, resp.dump());
    }
    if (password != db_pwd) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "密码错误";
        return crow::response(400, resp.dump());
    }

    // 检查持仓
    auto positions = position_dao::get_positions(account);
    int held = 0;
    for (const auto& p : positions) {
        if (p.stock_name == stock_name) {
            held = p.quantity;
            break;
        }
    }
    if (held < quantity) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "持仓不足";
        return crow::response(400, resp.dump());
    }

    // 到账
    double total = price * quantity;
    account_dao::deposit(account, total);

    // 减少持仓
    position_dao::sell_stock(account, stock_name, quantity);
    position_dao::insert_trade(account, stock_name, "卖出", quantity, price, total);

    nlohmann::json resp;
    resp["success"] = true;
    resp["message"] = "成功卖出 " + stock_name + " " + std::to_string(quantity) + " 股，获得 " + std::to_string(total) + " 元";
    return crow::response(200, resp.dump());
}

crow::response get_kline(const crow::request& req) {
    const char* c = req.url_params.get("code");
    std::string code = c ? c : "";
    const char* t = req.url_params.get("type");
    std::string type = t ? t : "daily";  // 默认日K

    if (code.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "缺少 code 参数";
        return crow::response(400, resp.dump());
    }

    // 找股票名
    auto stocks = stock_dao::get_all_stocks();
    std::string stock_name = code;
    for (const auto& s : stocks) {
        if (s.code == code) { stock_name = s.name; break; }
    }

    // 获取最新日期，判断要不要拉数据
    std::string cache_name = (type == "daily") ? stock_name : stock_name + "_" + type;
    std::string latest = stock_dao::get_latest_date(cache_name);
    
    // 日K: beg=一年前, 分钟K: beg=今天
    std::string beg_date;
    if (type == "1min") {
        // 分钟K只保留今天，清掉旧数据
        time_t now = time(nullptr);
        char today[16];
        strftime(today, sizeof(today), "%Y%m%d", localtime(&now));
        beg_date = today;
        if (latest < today) {
            // 今天的新数据来了，旧分钟线全清
            stock_dao::clear_history(stock_name);
            latest = "";
        }
    }
    
    if (beg_date.empty() || type == "daily") {
        // 日K: 从一年前开始
        time_t now = time(nullptr);
        char one_year_ago[16];
        time_t year_ago = now - 365 * 24 * 3600;
        strftime(one_year_ago, sizeof(one_year_ago), "%Y%m%d", localtime(&year_ago));
        beg_date = one_year_ago;
    }

    // 如果最新日期 < 今天，去拉新数据
    time_t now = time(nullptr);
    char today[16];
    strftime(today, sizeof(today), "%Y%m%d", localtime(&now));
    if (latest.empty() || latest < today) {
        auto klines = fetch_kline(code, stock_name, type, beg_date);
        std::cout << "K线抓取结果: " << klines.size() << " 条" << std::endl;
        if (!klines.empty()) {
            stock_dao::insert_history(cache_name, klines);
            auto test = stock_dao::get_history(cache_name);
            std::cout << "刚插入后查询: " << test.size() << " 条, cache_name=" << cache_name << std::endl;
        }
    }

    auto history = stock_dao::get_history(cache_name);
    nlohmann::json klines = nlohmann::json::array();
    for (const auto& h : history) {
        // 把 "20260711" 转成时间戳（毫秒）
        std::string date_str = h.trade_date;
        int y, m, d;
        sscanf(date_str.c_str(), "%d-%d-%d", &y, &m, &d);
        struct tm tm = {};
        tm.tm_year = y - 1900;
        tm.tm_mon  = m - 1;
        tm.tm_mday = d;
        time_t ts = mktime(&tm);
        int64_t millis = static_cast<int64_t>(ts) * 1000;

        nlohmann::json item;
        item["timestamp"] = millis;
        item["open"]      = h.open_price;
        item["high"]      = h.high_price;
        item["low"]       = h.low_price;
        item["close"]     = h.close_price;
        item["volume"]    = h.volume;
        klines.push_back(item);
    }

    if (!history.empty()) {
        double hist_high = 0, hist_low = 1e9;
        for (const auto& h : history) {
            if (h.high_price > hist_high) hist_high = h.high_price;
            if (h.low_price < hist_low)   hist_low  = h.low_price;
        }
        // 更新 stocks 表
        sqlite3* db = Database::instance().get();
        const char* sql = "UPDATE stocks SET high_hist=?, low_hist=? WHERE code=?;";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_double(stmt, 1, hist_high);
        sqlite3_bind_double(stmt, 2, hist_low);
        sqlite3_bind_text(stmt, 3, code.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    nlohmann::json resp;
    resp["success"] = true;
    resp["klines"]  = klines;
    return crow::response(200, resp.dump());
}

crow::response get_trades(const crow::request& req) {
    const char* acc = req.url_params.get("account");
    std::string account = acc ? acc : "";
    if (account.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "缺少 account 参数";
        return crow::response(400, resp.dump());
    }

    auto trades = position_dao::get_trades(account);

    nlohmann::json records = nlohmann::json::array();
    for (const auto& t : trades) {
        nlohmann::json item;
        char id_buf[16];
        snprintf(id_buf, sizeof(id_buf), "TX%06d", static_cast<int>(t.id));
        item["id"]           = std::string(id_buf);
        item["stock_name"]   = t.stock_name;
        item["type"]         = t.type;
        item["quantity"]     = static_cast<double>(t.quantity);
        item["price"]        = t.price;
        item["total_amount"] = t.total_amount;
        item["status"]       = t.status;
        item["timestamp"]    = t.created_at;
        records.push_back(item);
    }

    nlohmann::json resp;
    resp["success"] = true;
    resp["records"] = records;
    return crow::response(200, resp.dump());
}

}  // namespace stock_ctrl