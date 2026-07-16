#pragma once
#include "model/position.h"
#include "model/trade.h"
#include <vector>
#include <string>

namespace position_dao {

// 查询用户的所有持仓
std::vector<Position> get_positions(const std::string& account);

bool buy_stock(const std::string& account, const std::string& stock_name, int quantity);

bool sell_stock(const std::string& account, const std::string& stock_name, int quantity);

// 插入交易记录
void insert_trade(const std::string& account, const std::string& stock_name,
                  const std::string& type, int quantity, double price,
                  double total_amount);

// 查询交易记录
std::vector<Trade> get_trades(const std::string& account);

}  // namespace position_dao