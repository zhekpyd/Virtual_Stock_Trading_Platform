#pragma once
#include "model/stock.h"
#include "model/stock_history.h"
#include <string>
#include <vector>

namespace stock_dao {

// 根据股票名查股票信息
Stock get_stock_by_name(const std::string& name);

std::vector<Stock> get_all_stocks();

// 查历史数据是否已存在
bool has_history(const std::string& code);

// 获取历史数据
std::vector<StockHistory> get_history(const std::string& code);

// 批量写入历史数据
void insert_history(const std::string& name, const std::vector<StockHistory>& data);

std::string get_latest_date(const std::string& stock_name);

void clear_history(const std::string& stock_name);

}  // namespace stock_dao