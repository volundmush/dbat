#pragma once
#include "structs.h"

struct account_data *findAccount(const std::string &name);

struct account_data *createAccount(const std::string &name, const std::string &password);