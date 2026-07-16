#pragma once
#include <cstdint>
#include <string>

struct Trade {
    int64_t id;
    std::string account;
    std::string stock_name;
    std::string type;
    int quantity;
    double price;
    double total_amount;
    std::string status;
    std::string created_at;
};
