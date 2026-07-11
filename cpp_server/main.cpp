#include <iostream>   // 加上这行
#include <crow.h>
#include "controller/user_ctrl.h"
#include "controller/asset_ctrl.h"
#include "controller/stock_ctrl.h"
#include "dao/database.h"
#include "service/stock_fetcher.h"
#ifdef _WIN32
#include <windows.h>
#endif
int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(65001);
    #endif
    std::cout << "程序启动了!" << std::endl;   // 调试用，验证程序是否运行
    
    Database::instance().initialize("Tables.db");
    
    crow::SimpleApp app;

    CROW_ROUTE(app, "/api/register")
        .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
            return user_ctrl::register_user(req);
        });

    CROW_ROUTE(app, "/api/login")
        .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
            return user_ctrl::login_user(req);
        });
    CROW_ROUTE(app, "/api/user/info")
        .methods(crow::HTTPMethod::GET)([](const crow::request& req) {
            return user_ctrl::get_user_by_account(req);
        });
    CROW_ROUTE(app, "/api/user/username")
        .methods(crow::HTTPMethod::PUT)([](const crow::request& req) {
            return user_ctrl::update_username(req);
        });
    CROW_ROUTE(app, "/api/user/password")
        .methods(crow::HTTPMethod::PUT)([](const crow::request& req) {
            return user_ctrl::update_password(req);
        });
    CROW_ROUTE(app, "/api/asset")
        .methods(crow::HTTPMethod::GET)([](const crow::request& req) {
            return asset_ctrl::get_asset(req);
        });
    CROW_ROUTE(app, "/api/asset/recharge")
        .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        return asset_ctrl::deposit(req);
    });
    CROW_ROUTE(app, "/api/asset/withdraw")
        .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        return asset_ctrl::withdraw(req);
    });
    CROW_ROUTE(app, "/api/stock/market")
        .methods(crow::HTTPMethod::GET)([](const crow::request& req) {
        return stock_ctrl::get_market(req);
    });

    CROW_ROUTE(app, "/api/stock/buy")
        .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        return stock_ctrl::buy_stock(req);
    });
    CROW_ROUTE(app, "/api/stock/sell")
        .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        return stock_ctrl::sell_stock(req);
    });
    CROW_ROUTE(app, "/")([]() {
        crow::response res("Hello, 服务器跑起来了！");
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    fetch_and_update_stocks(); // 启动时立即获取一次股票数据

    app.port(18080).multithreaded().run();
    return 0;
}