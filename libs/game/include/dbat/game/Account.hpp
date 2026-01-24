#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <expected>

#include "Typedefs.hpp"

struct descriptor_data;

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

    bool check_password(std::string_view pwd) const;

};

extern std::map<vnum, std::shared_ptr<Account>> accounts;

struct Account *findAccount(const std::string &name);

std::expected<struct Account*, std::string> createAccount(const std::string &name, const std::string &password);