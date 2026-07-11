#pragma once
#include <crow.h>

namespace stock_ctrl {

crow::response get_market(const crow::request& req);
crow::response buy_stock(const crow::request& req);
crow::response sell_stock(const crow::request& req);

}  // namespace stock_ctrl