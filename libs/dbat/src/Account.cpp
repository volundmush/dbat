#include <ctime>
#include <boost/algorithm/string.hpp>

#include "dbat/Account.h"
#include "dbat/ID.h"


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

Account *createAccount(const std::string &name, const std::string &password)
{
    if (name.empty())
        throw std::invalid_argument("Username cannot be blank.");
    if (password.empty())
        throw std::invalid_argument("Password cannot be blank.");

    if (auto found = findAccount(name); found)
    {
        throw std::invalid_argument("Username already exists.");
    }

    auto nextId = Account::getNextID();
    auto a = std::make_shared<Account>();
    accounts[nextId] = a;
    a->name = name;
    a->id = nextId;
    a->password = password;
    a->created = time(nullptr);
    a->last_login = time(nullptr);

    a.get();
}

void Account::modRPP(int amt)
{
    rpp += amt;
    if (rpp < 0)
    {
        rpp = 0;
    }
}