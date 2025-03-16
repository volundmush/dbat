#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include "dbat/account.h"
#include "dbat/db.h"
#include "dbat/utils.h"

std::map<vnum, account_data> accounts;

struct account_data *findAccount(const std::string &name) {
    for (auto &[aid, account] : accounts) {
        if (boost::iequals(account.name, name)) {
            return &account;
        }
    }
    return nullptr;
}

int account_data::getNextID() {
    int id = 0;
    while(accounts.contains(id)) id++;
    return id;
}

account_data *createAccount(const std::string &name, const std::string &password) {
    if(name.empty()) throw std::invalid_argument("Username cannot be blank.");
    if(password.empty()) throw std::invalid_argument("Password cannot be blank.");

    if(auto found = findAccount(name); found) {
        throw std::invalid_argument("Username already exists.");
    }

    auto nextId = account_data::getNextID();
    auto a = accounts[nextId];
    a.name = name;
    a.vn = nextId;
    a.passHash = password;
    a.created = time(nullptr);
    a.lastLogin = time(nullptr);

    return findAccount(name);
}