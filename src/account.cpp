#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include "dbat/account.h"
#include "dbat/db.h"
#include "dbat/utils.h"

NegativeKeyGuardMap<vnum, Account> accounts;

struct Account *findAccount(const std::string &name)
{
    for (auto &[aid, account] : accounts)
    {
        if (boost::iequals(account.name, name))
        {
            return &account;
        }
    }
    return nullptr;
}

int Account::getNextID()
{
    int id = 0;
    while (accounts.contains(id))
        id++;
    return id;
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
    auto a = accounts[nextId];
    a.name = name;
    a.id = nextId;
    a.password = password;
    a.created = time(nullptr);
    a.last_login = time(nullptr);

    return findAccount(name);
}