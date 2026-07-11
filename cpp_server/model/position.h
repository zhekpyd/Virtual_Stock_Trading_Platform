#pragma once
#include <string>
#include <cstdint>

struct Position {
    int64_t id;
    std::string account;
    std::string stock_name;
    int quantity;
};