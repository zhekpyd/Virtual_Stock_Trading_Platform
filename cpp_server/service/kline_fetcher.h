#pragma once
#include <string>
#include <vector>
#include "model/stock_history.h"

// 从东方财富拉取K线数据，返回历史数据列表
std::vector<StockHistory> fetch_kline(const std::string& code, const std::string& name,
                                       const std::string& type, const std::string& beg_date);
