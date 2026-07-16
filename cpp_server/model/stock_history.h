#pragma once
#include <cstdint>
#include <string>

struct StockHistory {
    int64_t id;
    std::string stock_name;
    std::string trade_date;
    double open_price;
    double close_price;
    double high_price;
    double low_price;
    int64_t volume;
};
