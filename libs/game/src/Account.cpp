#include <ctime>
#include <boost/algorithm/string.hpp>

#include "dbat/game/Account.hpp"
#include "dbat/game/ID.hpp"


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