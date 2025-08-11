#pragma once
#include "structs.h"

struct Account *findAccount(const std::string &name);

struct Account *createAccount(const std::string &name, const std::string &password);