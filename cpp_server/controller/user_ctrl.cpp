#include "controller/user_ctrl.h"
#include "dao/user_dao.h"
#include "dao/account_dao.h"
#include <nlohmann/json.hpp>
namespace user_ctrl {

crow::response register_user(const crow::request& req) {
    // 1. 解析请求体里的 JSON
    auto body = nlohmann::json::parse(req.body);

    std::string account  = body.value("account", "");
    std::string username = body.value("username", "");
    std::string password = body.value("password", "");
    std::string phone    = body.value("phone", "");
    if (user_dao::account_exists(account)) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"]  = "账号已存在";
        return crow::response(400, resp.dump());
    }
    int64_t user_id = user_dao::insert_user(account, username, password, phone);
    if (user_id == -1) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"]  = "注册失败";
        return crow::response(500, resp.dump());
    }
    account_dao::init_account(account);
    nlohmann::json resp;
    resp["success"] = true;
    resp["message"]  = "账号创建完毕，请前往登录";
    resp["user_id"] = user_id;
    return crow::response(200, resp.dump());
}

crow::response login_user(const crow::request& req) {
    // 1. 解析 JSON
    auto body = nlohmann::json::parse(req.body);
    std::string account  = body.value("account", "");
    std::string password = body.value("password", "");

    // 2. 从数据库查这个账号的密码
    std::string db_password = user_dao::get_password(account);

    // 3. 查出来的密码为空 → 账号不存在
    if (db_password.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"]  = "账号不存在";
        return crow::response(400, resp.dump());
    }

    // 4. 密码不匹配
    if (password != db_password) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"]  = "密码错误";

        return crow::response(400, resp.dump());
    }

    // 5. 登录成功
    nlohmann::json resp;
    resp["success"] = true;
    resp["message"]  = "登录成功!";
    return crow::response(200, resp.dump());
}

crow::response get_user_by_account(const crow::request& req) {
    // 1. 从 URL 参数取 account

    const char* acc = req.url_params.get("account");
    std::string account = acc ? acc : "";
    if (account.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "缺少 account 参数";
        return crow::response(400, resp.dump());
    }

    // 2. 查数据库
    User user = user_dao::get_user_by_account(account);

    // 3. 用户不存在
    if (user.id == 0) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "用户不存在";
        return crow::response(400, resp.dump());
    }

    // 4. 返回用户信息（平铺格式，不返回 password）
    nlohmann::json resp;
    resp["success"]  = true;
    resp["account"]  = user.account;
    resp["username"] = user.username;
    resp["phone"]    = user.phone;

    return crow::response(200, resp.dump());
}

crow::response update_username(const crow::request& req) {
    // 1. 解析 JSON
    auto body = nlohmann::json::parse(req.body);
    std::string account = body.value("account", "");
    std::string new_username = body.value("new_username", "");

    // 2. 更新用户名
    if (!user_dao::update_username(account, new_username)) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "更新用户名失败";
        return crow::response(500, resp.dump());
    }

    // 3. 返回成功信息
    nlohmann::json resp;
    resp["success"] = true;
    resp["message"] = "更新用户名成功";
    return crow::response(200, resp.dump());
}

crow::response update_password(const crow::request& req) {
    // 1. 解析 JSON
    auto body = nlohmann::json::parse(req.body);
    std::string account = body.value("account", "");
    std::string new_password = body.value("new_password", "");

    // 2. 更新密码
    if (!user_dao::update_password(account, new_password)) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "更新密码失败";
        return crow::response(500, resp.dump());
    }

    // 3. 返回成功信息
    nlohmann::json resp;
    resp["success"] = true;
    resp["message"] = "更新密码成功";
    return crow::response(200, resp.dump());
}

}  //namespace user_ctrl