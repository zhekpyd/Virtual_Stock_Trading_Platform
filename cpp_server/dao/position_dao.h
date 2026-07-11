#pragma once
#include "model/position.h"
#include <vector>
#include <string>

namespace position_dao {

// 查询用户的所有持仓
std::vector<Position> get_positions(const std::string& account);

bool buy_stock(const std::string& account, const std::string& stock_name, int quantity);

bool sell_stock(const std::string& account, const std::string& stock_name, int quantity);

}  // namespace position_dao