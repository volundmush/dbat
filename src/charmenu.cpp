#include "dbat/charmenu.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/interpreter.h"
#include "dbat/puppet.h"
#include "dbat/accmenu.h"
#include "dbat/players.h"

namespace net {

    std::string CharacterMenu::getName() {
        return "CharacterMenu";
    }

    nlohmann::json CharacterMenu::serialize() {
        auto j = ConnectionParser::serialize();
        j["state"] = state;
        if(ch) j["ch"] = ch->id;

        return j;
    }

    void CharacterMenu::deserialize(const nlohmann::json& j) {
        if(j.contains("state")) state = j.at("state").get<int>();
        if(j.contains("ch")) {
            auto id = j["ch"].get<int>();
            ch = uniqueCharacters.at(id).get();
        }

    }

    CharacterMenu::CharacterMenu(const std::shared_ptr<Connection>& co, char_data *c) : ConnectionParser(co) {
        ch = c;
    }

    void CharacterMenu::start() {
        sendText(CONFIG_MENU);
    }

    void CharacterMenu::parse(const std::string &txt) {
        if(txt.empty()) return;
        auto arg = atoi(txt.c_str());

        switch (arg) {
            case 0:
                sendText("Goodbye.\r\n", net::SendBuffer::BF_CLOSE_AFTER_SEND);
                break;

            case 1:
                if(!ch->desc && !conn->account->descriptors.empty() && conn->account->adminLevel < 1) {
                    sendText("You have reached the maximum number of active characters.\r\n");
                    return;
                }
                conn->setParser(new PuppetParser(conn, ch));
                break;

            case 2:
                sendText("Use the desc command in-game for this now.\r\n");
                break;
            case 3:
                conn->setParser(new AccountMenu(conn));
                break;
            case 4:
                sendText(fmt::format("To delete {}, please enter the following command: @rdelete {}@n\r\n", ch->name, ch->name));
                break;
            default:
                if(txt.starts_with("delete ")) {
                    if(txt.ends_with(fmt::format(" {}", ch->name))) {
                        if(!canDeleteCharacter(ch->shared())) {
                            sendText("Having trouble deleting that character. Make sure they're fully logged off.\r\n");
                            break;
                        }
                        sendText(fmt::format("Good bye, {}!\r\n", ch->name));
                        basic_mud_log("%s deleted their character: %s", conn->account->name, ch->name);
                        deletePlayerCharacter(ch->shared());
                        conn->setParser(new AccountMenu(conn));
                        break;
                    } else {
                        sendText(fmt::format("To delete {}, please enter the following command: @rdelete {}@n\r\n", ch->name, ch->name));
                        break;
                    }
                }
                sendText(fmt::format("\r\nThat's not a menu choice!\r\n{}\r\n{}", motd, CONFIG_MENU));
                break;
        }

    }
}