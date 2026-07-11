#pragma once
#include "model/stock.h"
#include <string>
#include <vector>

namespace stock_dao {

// 根据股票名查股票信息
Stock get_stock_by_name(const std::string& name);

std::vector<Stock> get_all_stocks();

}  // namespace stock_dao