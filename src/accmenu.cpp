#pragma once
#include "accmenu.h"
#include "charmenu.h"
#include "db.h"
#include "players.h"
#include "fmt/format.h"
#include <boost/algorithm/string.hpp>

namespace net {

    void AccountMenu::displayMenu() {
        sendText("                 @RUser Menu@n\n");
        sendText("@D=============================================@n\r\n");
        sendText(fmt::format("@D|@gUser Account  @D: @w{:>27}@D|@n\n", conn->account->name));
        sendText(fmt::format("@D|@gEmail Address @D: @w{:>27}@D|@n\n", conn->account->email));
        sendText(fmt::format("@D|@gMax Characters@D: @w{:>27}@D|@n\n", conn->account->slots));
        sendText(fmt::format("@D|@gRP Points     @D: @w{:>27}@D|@n\n", conn->account->rpp));


        sendText("@D=============================================@n\r\n\r\n");
        sendText("      @D[@y------@YAvailable Characters@y------@D]@n\n");
        for(auto c : account->characters) {
            auto p = players.find(c);
            if(p == players.end()) continue;
            sendText(fmt::format("                @b-- @C%s@n\r\n", p->character->name));
        }

        sendText("      @D[@y---- @YSelect Another Option @y----@D]@n\r\n");
        sendText("                @B(@WB@B) @CBuy New C. Slot @D(@R15 RPP@D)@n\r\n");
        sendText("                @B(@WC@B) @CUser's Customs\r\n");
        sendText("                @B(@WD@B) @RDelete User@n\r\n");
        sendText("                @B(@WE@B) @CEmail@n\r\n");
        sendText("                @B(@WP@B) @CNew Password@n\r\n");
        sendText("                @B(@WQ@B) @CQuit@n\r\n");
        sendText("\r\nMake your choice: \r\n");
    }

    void AccountMenu::start() {
        state = AccountMenuState::MainMenu;
        displayMenu();
    }

    void AccountMenu::parse(const std::string &txt) {
        if(boost::iequals(txt, "return")) {
            start();
            return;
        }

        if(boost::istarts_with(txt, "select ")) {
            auto name = txt.substr(5, txt.size());
            struct char_data *c = nullptr;
            for(auto v : conn->account->characters) {
                auto p = players.find(v);
                if(p == players.end()) continue;
                if(boost::iequals(p->character->name, name)) {
                    c = p->character;
                    break;
                }
            }
            if(!c) {
                sendText("That is not a character!\r\n");
                return;
            }
            conn->setParser(new CharacterMenu(conn, c));

        } else if (boost::iequals(txt, "slot")) {
            if(conn->account->rpp < 15) {
                sendText("You need at least 15 RPP to purchase a new character slot.\r\n");
                return;
            }
            if(conn->account->slots > 4) {
                sendText("You can't buy any more character slots.\r\n");
                return;
            }
            conn->account->modRPP(-15);
            conn->account->slots++;
            sendText("New slot purchased, -15 RPP.\r\n");
            return;
        }
    }
}