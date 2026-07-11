#pragma once
#include <string>

namespace account_dao {

double get_cash_balance(const std::string& account);

bool deposit(const std::string& account, double amount);

bool withdraw(const std::string& account, double amount);

bool init_account(const std::string& account);

}  // namespace account_dao