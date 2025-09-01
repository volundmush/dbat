#pragma once
#include "sysdep.h"

struct Account {
    Account() = default;
    int id{NOTHING};
    std::string name;
    std::string password;
    std::string email;
    time_t created{};
    time_t last_login{};
    time_t last_logout{};
    time_t last_change_password{};
    double playtime{};
    std::string disabled_reason;
    time_t disabled_until{0};
    int admin_level{};
    int rpp{};
    int slots{3};
    std::vector<std::string> customs;
    std::vector<int> characters;
    std::unordered_set<descriptor_data*> descriptors;
    // this is used by Cython.
    std::unordered_map<int64_t, std::string> connections;

    void modRPP(int amt);

    bool canBeDeleted();

    static int getNextID();

};

struct Account *findAccount(const std::string &name);

struct Account *createAccount(const std::string &name, const std::string &password);