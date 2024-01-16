#pragma once
#include "structs.h"

extern std::map<vnum, account_data> accounts;

struct account_data *findAccount(const std::string &name);

struct account_data *createAccount(const std::string &name, const std::string &password);