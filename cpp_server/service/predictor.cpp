#include "service/predictor.h"
#include "dao/prediction_dao.h"
#include "dao/stock_dao.h"
#include "dao/news_dao.h"
#include <iostream>
#include <cmath>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif
#include <nlohmann/json.hpp>

static double tech_momentum(const std::string& code) {
    auto stocks = stock_dao::get_all_stocks();
    for (const auto& s : stocks) if (s.code == code) return s.change_pct;
    return 0.0;
}

static double news_bias(const std::string& code) {
    auto all = news_dao::get_recent(24);
    double total = 0;
    int count = 0;
    for (const auto& n : all) {
        if (n.stocks_impact.empty() || n.stocks_impact == "[]") continue;
        try {
            auto arr = nlohmann::json::parse(n.stocks_impact);
            for (const auto& s : arr) {
                std::string c = s.value("code", "");
                if (c == code) { total += s.value("score", 0.0); count++; }
            }
        } catch (...) {}
    }
    if (count == 0) return 0.0;
    return total / count;
}

static void predict_one(const Stock& s) {
    double momentum = tech_momentum(s.code);
    double news = news_bias(s.name);
    double base = momentum * 0.7 + news * 0.3;

    // 波动率差异化衰减：涨跌幅越大，趋势衰减越快
    double abs_m = std::abs(momentum);
    double decay = (abs_m > 5.0) ? 0.45 : (abs_m > 2.0) ? 0.55 : (abs_m > 1.0) ? 0.65 : 0.75;

    // 加随机扰动（±0.3%），让曲线不机械
    double noise = 0.001;

    for (int day = 1; day <= 5; day++) {
        Prediction p;
        p.stock_code    = s.code;
        p.stock_name    = s.name;
        p.day_offset    = day;
        double trend = base * pow(decay, day - 1);
        double jitter = ((rand() % 2000) - 1000) / 1000000.0;  // ±0.1%
        p.predicted_rate = trend + jitter;
        p.confidence    = 0.7 * pow(decay, day - 1);
        p.price_now     = s.price;
        prediction_dao::insert(p);
    }
}

static void verify_old() {
    for (int day = 1; day <= 5; day++) {
        auto pending = prediction_dao::get_pending(day);
        if (pending.empty()) continue;
        auto stocks = stock_dao::get_all_stocks();
        int done = 0;
        for (const auto& p : pending) {
            double cur = 0;
            for (const auto& s : stocks) if (s.code == p.stock_code) { cur = s.price; break; }
            if (cur == 0) continue;
            prediction_dao::verify(p.id, cur);
            done++;
        }
        std::cout << "[验证] day=" << day << " 验证 " << done << " 条" << std::endl;
    }
}

void predictor_loop() {
    std::cout << "[预测系统] 启动(5天投影)" << std::endl;
    while (true) {
        verify_old();
        auto stocks = stock_dao::get_all_stocks();
        std::cout << "[预测] 预测 " << stocks.size() << " 只股票 x 5天..." << std::endl;
        for (const auto& s : stocks) predict_one(s);
        std::cout << "[预测] 完成 " << stocks.size() << " x5 = " << stocks.size()*5 << " 条" << std::endl;
        std::this_thread::sleep_for(std::chrono::minutes(30));
    }
}