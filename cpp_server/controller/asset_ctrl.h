#pragma once
#include <crow.h>

namespace asset_ctrl {

crow::response get_asset(const crow::request& req);

crow::response deposit(const crow::request& req);

crow::response withdraw(const crow::request& req);

}  // namespace asset_ctrl