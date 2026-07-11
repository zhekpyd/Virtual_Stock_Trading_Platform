#include "controller/asset_ctrl.h"
#include "dao/account_dao.h"
#include "dao/position_dao.h"
#include "dao/stock_dao.h"
#include "dao/user_dao.h"
#include <nlohmann/json.hpp>

namespace asset_ctrl {

crow::response get_asset(const crow::request& req) {
    // 1. 取 account 参数
    const char* acc = req.url_params.get("account");
    std::string account = acc ? acc : "";
    if (account.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "缺少 account 参数";
        return crow::response(400, resp.dump());
    }

    // 2. 查现金
    double cash = account_dao::get_cash_balance(account);

    // 3. 查持仓
    auto positions = position_dao::get_positions(account);

    // 4. 算股票市值
    double stock_value = 0.0;
    nlohmann::json stocks_json = nlohmann::json::array();

    for (const auto& pos : positions) {
        Stock stock = stock_dao::get_stock_by_name(pos.stock_name);
        double market_value = stock.price * pos.quantity;
        stock_value += market_value;

        nlohmann::json item;
        item["name"]         = pos.stock_name;
        item["quantity"]     = pos.quantity;
        item["market_value"] = market_value;
        stocks_json.push_back(item);
    }

    // 5. 返回
    nlohmann::json resp;
    resp["success"]      = true;
    resp["total_asset"]  = cash + stock_value;
    resp["cash_balance"] = cash;
    resp["stock_value"]  = stock_value;
    resp["stocks"]       = stocks_json;
    return crow::response(200, resp.dump());
}

crow::response deposit(const crow::request& req) {
    auto body = nlohmann::json::parse(req.body);
    std::string account  = body.value("account", "");
    double amount        = body.value("amount", 0.0);
    std::string password = body.value("password", "");

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

    if (!account_dao::deposit(account, amount)) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "充值失败";
        return crow::response(500, resp.dump());
    }

    nlohmann::json resp;
    resp["success"] = true;
    resp["message"] = "充值完成！";
    return crow::response(200, resp.dump());
}

crow::response withdraw(const crow::request& req) {
    auto body = nlohmann::json::parse(req.body);
    std::string account  = body.value("account", "");
    double amount        = body.value("amount", 0.0);
    std::string password = body.value("password", "");

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
    double balance = account_dao::get_cash_balance(account);
    if (balance < amount) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "余额不足";
        return crow::response(400, resp.dump());
    }

    if (!account_dao::withdraw(account, amount)) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "提现失败";
        return crow::response(500, resp.dump());
    }

    nlohmann::json resp;
    resp["success"] = true;
    resp["message"] = "提现操作完成";
    return crow::response(200, resp.dump());
}

}  // namespace asset_ctrl