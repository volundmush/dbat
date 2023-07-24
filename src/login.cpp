#include "login.h"
#include "db.h"
#include "config.h"
#include "dg_comm.h"
#include "utils.h"
#include "ban.h"
#include "fmt/format.h"
#include "account.h"
#include "accmenu.h"
#include <boost/algorithm/string.hpp>

namespace net {
    void LoginParser::start() {
        sendText(GREETANSI);
        sendText("\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
        sendText(fmt::format("@D                 ---(@CPeak Logon Count Today@W: @w{:>4}@D)---@n\r\n", PCOUNT));
        sendText(fmt::format("@D                 ---(@CHighest Logon Count   @W: @w{:>4}@D)---@n\r\n", HIGHPCOUNT));
        sendText(fmt::format("@D                 ---(@CTotal Era {} Characters@W: @w{:>4}@D)---@n\r\n", CURRENT_ERA,
                  add_commas(ERAPLAYERS)));
        parse("");
    }

    void LoginParser::parse(const std::string &txt) {
        if(txt == "return") {
            state = LoginState::GetName;
            name.clear();
            email.clear();
            password.clear();
            account = nullptr;
            start();
        }

        switch(state) {
            case LoginState::GetName:
                getName(txt);
                break;
            case LoginState::GetPassword:
                getPassword(txt);
                break;
            case LoginState::ConfName:
                confName(txt);
                break;
            case LoginState::ConfPassword:
                confPassword(txt);
                break;
        }
    }

    void LoginParser::getName(const std::string &txt) {
        if(txt.empty()) {
            sendText("\r\n@cEnter your desired username or the username you have already made.\r\n@CEnter Username:@n\r\n");
            return;
        }
        if(!Valid_Name((char*)txt.c_str())) {
            sendText("Invalid name. Username?\r\n");
            return;
        }
        if (boost::contains(txt, " ")) {
            sendText("No spaces. Username?\r\n");
            return;
        }
        if(!boost::algorithm::all(txt, boost::algorithm::is_alpha())) {
            sendText("No special symbols or number. Username?\r\n");
            return;
        }
        if (txt.size() < 3) {
            sendText("Name must at least be 3 characters long, username?\r\n");
            return;
        }
        if (txt.size() > 10) {
            sendText("Name must be at most 10 characters long, username?\r\n");
            return;
        }
        account = findAccount(txt);
        name = txt;
        if(account) {
            state = LoginState::GetPassword;
            parse("");
            return;
        } else {
            state = LoginState::ConfName;
            parse("");
            return;
        }
    }

    void LoginParser::confName(const std::string &txt) {
        if(txt.empty()) {
            sendText(fmt::format("You want you user name to be, {}?\r\n", name));
            sendText("Yes or no: \r\n");
            return;
        }
        if(boost::iequals(txt, "yes")) {
            state = LoginState::GetPassword;
            parse("");
            return;
        } else if (boost::iequals(txt, "no")) {
            state = LoginState::GetName;
            name.clear();
            parse("");
            return;
        }
    }

    void LoginParser::getPassword(const std::string &txt) {
        if(txt.empty()) {
            sendText("Enter Password or Return:\r\n");
            sendText("(Return will ask for a different username)\r\n");
            return;
        }
        if(account) {
            if(account->checkPassword(txt)) {
                // successful login!
                conn->account = account;
                conn->setParser(new AccountMenu(conn));
                return;
            } else {
                sendText("Password is wrong. Password or Return?\r\n");
                sendText("(Return will ask for a different username)\r\n");
                send_to_imm("Username, %s, password failure!", (char*)account->name.c_str());
                log("%s BAD PASSWORD", (char*)account->name.c_str());
                return;
            }
        } else {
            password = txt;
            state = LoginState::ConfPassword;
            parse("");
            return;
        }
    }

    void LoginParser::confPassword(const std::string &txt) {
        if(txt.empty()) {
            sendText("Enter password again to verify: \r\n");
            return;
        }
        if(txt == password) {
            auto id = next_acc_id();
            auto &a = accounts[id];
            a.vn = id;
            a.name = name;
            a.setPassword(password);
            a.created = time(nullptr);
            a.lastLogin = time(nullptr);
            // Create a new user!
            conn->account = &a;
            dirty_accounts.insert(a.vn);
            conn->setParser(new AccountMenu(conn));
            return;
        }
    }
}