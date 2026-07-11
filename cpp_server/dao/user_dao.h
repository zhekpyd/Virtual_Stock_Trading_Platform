#pragma once
#include "model/user.h"
#include <string>

namespace user_dao {

// 注册新用户，返回新用户的 id，失败返回 -1
int64_t insert_user(const std::string& account,
                    const std::string& username,
                    const std::string& password,
                    const std::string& phone);

// 检查账号是否已存在
bool account_exists(const std::string& account);

// 根据账号查密码，如果用户不存在返回空字符串
std::string get_password(const std::string& account);

User get_user_by_account(const std::string& account);

bool update_username(const std::string& account, const std::string& new_username);

bool update_password(const std::string& account, const std::string& new_password);

}  // namespace user_dao