#pragma once
#include <crow.h>

namespace user_ctrl {

// 用户注册
crow::response register_user(const crow::request& req);

crow::response login_user(const crow::request& req);

crow::response get_user_by_account(const crow::request& req);

crow::response update_username(const crow::request& req);

crow::response update_password(const crow::request& req);

}  // namespace user_ctrl