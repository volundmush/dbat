#pragma once
#include "dbat/accmenu.h"
#include "dbat/charmenu.h"
#include "dbat/db.h"
#include "dbat/players.h"
#include "fmt/format.h"
#include "dbat/chargen.h"
#include "dbat/utils.h"

namespace net {

    std::string AccountMenu::getName() {
        return "AccountMenu";
    }

    nlohmann::json AccountMenu::serialize() {
        auto j = ConnectionParser::serialize();
        j["state"] = 0;
        return j;
    }

    void AccountMenu::deserialize(const nlohmann::json& j) {
        if(j.contains("state")) state = static_cast<AccountMenuState>(j.at("state").get<int>());
    }

    void AccountMenu::displayMenu() {
        auto a = conn->account;
        sendText("                 @RUser Menu@n\n");
        sendText("@D=============================================@n\r\n");
        sendText(fmt::format("@D|@gUser Account  @D: @w{:>27}@D|@n\n", a->name));
        sendText(fmt::format("@D|@gEmail Address @D: @w{:>27}@D|@n\n", a->email));
        sendText(fmt::format("@D|@gMax Characters@D: @w{:>27}@D|@n\n", a->slots));
        sendText(fmt::format("@D|@gRP Points     @D: @w{:>27}@D|@n\n", a->rpp));

        sendText("@D=============================================@n\r\n\r\n");
        sendText("      @D[@y------@YActive Connections@y------@D]@n\n");
        for(const auto &c : a->connections) {
            auto &cap = c->capabilities;
            std::string line = fmt::format("[{}] {} over ({}) {} version {}", c->connId, cap.hostAddress, magic_enum::enum_name(cap.protocol), cap.clientName, cap.clientVersion);
            if(c->desc) {
                line += fmt::format(" as {}@n\r\n", c->desc->character->name);
            } else {
                line += "@n\r\n";
            }
            sendText(line);
        }

        sendText("@D=============================================@n\r\n\r\n");
        sendText("      @D[@y------@YAvailable Characters@y------@D]@n\n");
        int counter = 0;
        for(auto charID : a->characters) {
            auto found = players.find(charID);
            if(found == players.end()) continue;
            auto ch = found->second.character;
            if(!ch) continue;
            std::string line = fmt::format("                @B(@W{}@B) @C{}@n", ++counter, ch->name);
            if(ch->desc) {
                line += fmt::format(" @D[@y{} Connections@D]@n\r\n", ch->desc->conns.size());
            } else {
                line += "\r\n";
            }
            sendText(line);

        }
        while(counter < conn->account->slots) {
            sendText(fmt::format("                @B(@W{}@B) @C{}@n\r\n", ++counter, "<empty>"));
        }

        sendText("      @D[@y---- @YSelect Another Option @y----@D]@n\r\n");
        sendText("                @B(@WA@B) @CCreate New Character@n\r\n");
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
        if(boost::iequals(txt, "return") || boost::istarts_with(txt, "return ")) {
            start();
            return;
        }

        if(!txt.empty() && is_numeric(txt)) {
            auto num = std::stoi(txt);
            auto slot = num - 1;

            if(num < 0 || num > conn->account->characters.size()) {
                sendText("No such slot.\r\n");
                return;
            }

            auto ref = conn->account->characters[slot];
            auto found = players.find(ref);
            if(found == players.end()) {
                sendText("ERROR: Character not found. Please alert staff.\r\n");
                return;
            }
            auto c = found->second.character;

            conn->setParser(new CharacterMenu(conn, c));
            return;
        }

        if(boost::iequals(txt, "A") || boost::istarts_with(txt, "A ")) {
            // Start chargen...
            if(conn->account->slots <= conn->account->characters.size()) {
                sendText("Sorry, your slots are all full.\r\n");
                return;
            }
            conn->setParser(new ChargenParser(conn));
            return;
        }

        if (boost::iequals(txt, "B") || boost::istarts_with(txt, "B ")) {
            // Buying a slot.
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
            // no need to set account dirty since modRPP does that.
            sendText("New slot purchased. [-15 RPP].\r\n");
            return;
        }

        if(boost::iequals(txt, "D") || boost::istarts_with(txt, "D ")) {
            sendText("Deleting your account will also delete ALL ASSOCIATED CHARACTERS.\r\n@rThis action cannot be undone.@n\r\n");
            sendText(fmt::format("To verify account deletion, the true command is:\r\n@rdelete {}@n\r\n", conn->account->name));

            return;
        }

        if(boost::iequals(txt, fmt::format("delete {}", conn->account->name))) {
            if(!conn->account->canBeDeleted()) {
                sendText("Your account can only be deleted when all characters are logged off.");
                return;
            }
            auto vn = conn->account->vn;
            basic_mud_log("%s deleted their user account.", conn->account->name);
            sendText("Your account has been deleted. Farewell.\r\n", net::SendBuffer::BF_CLOSE_AFTER_SEND);
            deleteUserAccount(vn);

            return;
        }

        if(boost::iequals(txt, "Q") || boost::istarts_with(txt, "Q ")) {
            sendText("Goodbye.\r\n", net::SendBuffer::BF_CLOSE_AFTER_SEND);
            return;
        }

        sendText("Invalid option.\r\n");
        start();
    }
}