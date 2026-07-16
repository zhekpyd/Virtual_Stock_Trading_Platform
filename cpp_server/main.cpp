#include <iostream>   // 加上这行
#include <crow.h>
#include "controller/user_ctrl.h"
#include "controller/asset_ctrl.h"
#include "controller/stock_ctrl.h"
#include "controller/ai_ctrl.h"
#include "controller/predict_ctrl.h"
#include "dao/database.h"
#include "service/stock_fetcher.h"
#include "service/news_fetcher.h"
#include "service/predictor.h"
#include <thread>
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
    CROW_ROUTE(app, "/api/stock/kline")
    .methods(crow::HTTPMethod::GET)([](const crow::request& req) {
        return stock_ctrl::get_kline(req);
    });
    CROW_ROUTE(app, "/api/transaction/history")
    .methods(crow::HTTPMethod::GET)([](const crow::request& req) {
        return stock_ctrl::get_trades(req);
    });
    CROW_ROUTE(app, "/api/ai/analyze")
    .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        return ai_ctrl::analyze(req);
    });
    CROW_ROUTE(app, "/api/predict")
    .methods(crow::HTTPMethod::GET)([](const crow::request& req) {
        return predict_ctrl::get_predictions(req);
    });

    CROW_ROUTE(app, "/")([]() 
    {
        crow::response res("Hello, 服务器跑起来了！");
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    // 后台线程：定时 + 初始拉取股票数据
    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));  // 先等 3 秒让服务器启动
        fetch_and_update_stocks();  // 首次拉取
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(5));
            std::cout << "定时更新股票价格..." << std::endl;
            fetch_and_update_stocks();
        }
    }).detach();

    // 在其他后台线程附近加
    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        while (true) {
            fetch_and_analyze_news();
            std::this_thread::sleep_for(std::chrono::minutes(5));
        }
    }).detach();

    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        predictor_loop();
    }).detach();

    app.port(18080).multithreaded().run();
    return 0;
}