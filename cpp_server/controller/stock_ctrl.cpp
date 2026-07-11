#include "controller/stock_ctrl.h"
#include "dao/stock_dao.h"
#include "dao/account_dao.h"
#include "dao/position_dao.h"
#include "dao/user_dao.h"
#include <nlohmann/json.hpp>

namespace stock_ctrl {

// ===== 股票行情 =====
crow::response get_market(const crow::request& req) {
    auto stocks = stock_dao::get_all_stocks();

    nlohmann::json stocks_json = nlohmann::json::array();
    for (const auto& s : stocks) {
        nlohmann::json item;
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

    nlohmann::json resp;
    resp["success"] = true;
    resp["message"] = "成功卖出 " + stock_name + " " + std::to_string(quantity) + " 股，获得 " + std::to_string(total) + " 元";
    return crow::response(200, resp.dump());
}

}  // namespace stock_ctrl