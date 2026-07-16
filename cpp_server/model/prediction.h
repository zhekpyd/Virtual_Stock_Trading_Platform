#pragma once
#include <cstdint>
#include <string>

struct Prediction {
    int64_t id;
    std::string stock_code;
    std::string stock_name;
    int day_offset;         // 1~5
    double predicted_rate;  // 预测涨跌幅%
    double confidence;
    double price_now;
    double actual_price;
    double actual_rate;
    std::string status;     // "pending" / "verified"
    std::string created_at;
};