#include <ctime>
#include <boost/algorithm/string.hpp>

#include "dbat/game/Account.hpp"
#include "dbat/game/ID.hpp"
#include <nlohmann/json.hpp>


std::map<vnum, std::shared_ptr<Account>> accounts;

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

int Account::getNextID()
{
    static int lastAccountID = 0;
    return ::getNextID(lastAccountID, accounts);
}

std::expected<struct Account*, std::string> createAccount(const std::string &name, const std::string &password)
{
    if (name.empty())
        return std::unexpected("Username cannot be blank.");
    if (password.empty())
        return std::unexpected("Password cannot be blank.");

    if (auto found = findAccount(name); found)
    {
        return std::unexpected("Username already exists.");
    }

    auto nextId = Account::getNextID();
    auto a = std::make_shared<Account>();
    accounts[nextId] = a;
    a->name = name;
    a->id = nextId;
    a->password = password;
    a->created = time(nullptr);
    a->last_login = time(nullptr);

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

bool Account::check_password(std::string_view pwd) const
{
    return password == pwd;
}

void to_json(nlohmann::json &j, const Account &a)
{
    if (a.id != NOTHING)
        j["id"] = a.id;
    if (!a.name.empty())
        j["name"] = a.name;
    if (!a.password.empty())
        j["password"] = a.password;
    if (!a.email.empty())
        j["email"] = a.email;
    if (a.created)
        j["created"] = a.created;
    if (a.last_login)
        j["last_login"] = a.last_login;
    if (a.last_logout)
        j["last_logout"] = a.last_logout;
    if (a.last_change_password)
        j["last_change_password"] = a.last_change_password;
    if (a.playtime != 0.0)
        j["playtime"] = a.playtime;
    if (!a.disabled_reason.empty())
        j["disabled_reason"] = a.disabled_reason;
    if (a.disabled_until)
        j["disabled_until"] = a.disabled_until;
    if (a.rpp)
        j["rpp"] = a.rpp;
    if (a.slots != 3)
        j["slots"] = a.slots;
    if (a.admin_level)
        j["admin_level"] = a.admin_level;
    j["characters"] = a.characters;
}

void from_json(const nlohmann::json &j, Account &a)
{
    if (j.contains(+"id"))
        a.id = j["id"];
    if (j.contains(+"name"))
        a.name = j["name"];
    if (j.contains(+"password"))
        a.password = j["password"];
    if (j.contains(+"email"))
        a.email = j["email"];
    if (j.contains(+"created"))
        a.created = j["created"];
    if (j.contains(+"lastLogin"))
        a.last_login = j["lastLogin"];
    if (j.contains(+"lastLogout"))
        a.last_logout = j["lastLogout"];
    if (j.contains(+"last_change_password"))
        a.last_change_password = j["last_change_password"];
    if (j.contains(+"playtime"))
        a.playtime = j["playtime"];
    if (j.contains(+"disabled_reason"))
        a.disabled_reason = j["disabled_reason"];
    if (j.contains(+"disabled_until"))
        a.disabled_until = j["disabled_until"];
    if (j.contains(+"rpp"))
        a.rpp = j["rpp"];
    if (j.contains(+"slots"))
        a.slots = j["slots"];
    if (j.contains(+"admin_level"))
        a.admin_level = j["admin_level"];
    if (j.contains(+"characters"))
        a.characters = j["characters"].get<std::vector<int>>();
}
