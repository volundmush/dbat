#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>
#include <nlohmann/json_fwd.hpp>

#include "Typedefs.hpp"

struct descriptor_data;

struct Account {
    Account() = default;
    std::string id; // a UUID
    std::string name;
    time_t created{};
    int admin_level{};
    int rpp{};
    int slots{3};
    std::vector<std::string> customs;
    std::vector<std::string> characters;
    std::unordered_set<descriptor_data*> descriptors;

    void modRPP(int amt);

    bool canBeDeleted();

};

extern std::map<std::string, std::shared_ptr<Account>> accounts;

struct Account *findAccount(const std::string &name);

void to_json(nlohmann::json& j, const Account& a);
void from_json(const nlohmann::json& j, Account& a);
