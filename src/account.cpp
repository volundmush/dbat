#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include "sodium.h"

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

static std::optional<std::string> hashPassword(const std::string& password) {
    char hashed_password[crypto_pwhash_STRBYTES];
    if(password.empty()) return std::nullopt;
    if(crypto_pwhash_str(hashed_password, password.data(), password.size(),
                         crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        return std::nullopt;
    }
    return hashed_password;

}

bool account_data::checkPassword(const std::string &password) {
    auto result = crypto_pwhash_str_verify(passHash.data(), password.data(), password.size());
    return result == 0;
}

bool account_data::setPassword(const std::string &password) {
    auto hash = hashPassword(password);
    if(!hash) return false;
    passHash = hash.value();
    lastPasswordChanged = time(nullptr);
    return true;
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

    auto hash = hashPassword(password);
    if(!hash) {
        throw std::invalid_argument("Password rejected by hashing algorithm, try another.");
    }

    auto nextId = account_data::getNextID();
    auto a = accounts[nextId];
    a.name = name;
    a.vn = nextId;
    a.passHash = hash.value();
    a.created = time(nullptr);
    a.lastLogin = time(nullptr);

    return nullptr;
}