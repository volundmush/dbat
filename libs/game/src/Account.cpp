#include <ctime>
#include <boost/algorithm/string.hpp>

#include "dbat/game/Account.hpp"
#include "dbat/game/ID.hpp"
#include <nlohmann/json.hpp>


std::unordered_map<std::string, std::shared_ptr<Account>> accounts;

struct Account *findAccount(const std::string &name)
{
    for (auto &[aid, account] : accounts)
    {
        if (boost::iequals(account->name, name))
        {
            return account.get();
        }
    }
    return nullptr;
}


std::expected<struct Account*, std::string> createAccount(const std::string &id, const std::string &name)
{
    if (name.empty())
        return std::unexpected("Username cannot be blank.");

    if (auto found = findAccount(name); found)
    {
        return std::unexpected("Username already exists.");
    }

    auto a = std::make_shared<Account>();
    accounts[id] = a;
    a->name = name;
    a->id = id;

    return a.get();
}

void Account::modRPP(int amt)
{
    rpp += amt;
    if (rpp < 0)
    {
        rpp = 0;
    }
}


void to_json(nlohmann::json &j, const Account &a)
{
    if (a.id != NOTHING)
        j["id"] = a.id;
    if (!a.name.empty())
        j["name"] = a.name;
    if (a.rpp)
        j["rpp"] = a.rpp;
    if (a.slots != 3)
        j["slots"] = a.slots;
    if (a.admin_level)
        j["admin_level"] = a.admin_level;
}

void from_json(const nlohmann::json &j, Account &a)
{
    if (j.contains(+"id"))
        a.id = j["id"];
    if (j.contains(+"name"))
        a.name = j["name"];
    if (j.contains(+"created"))
        a.created = j["created"];
    if (j.contains(+"rpp"))
        a.rpp = j["rpp"];
    if (j.contains(+"slots"))
        a.slots = j["slots"];
    if (j.contains(+"admin_level"))
        a.admin_level = j["admin_level"];
}
