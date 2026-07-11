#pragma once
#include <string>
#include <cstdint>

struct Stock {
    int64_t id;
    std::string name;
    double price;
    double change_pct;
    double high_day;
    double low_day;
    double high_hist;
    double low_hist;
    int64_t volume;
    int64_t total_shares;
};