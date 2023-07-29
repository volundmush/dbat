#include "dbat/charmenu.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/interpreter.h"
#include "dbat/puppet.h"
#include "dbat/accmenu.h"

namespace net {
    CharacterMenu::CharacterMenu(std::shared_ptr<Connection>& co, char_data *c) : ConnectionParser(co) {
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
                sendText("Goodbye.\r\n");
                conn->close();
                break;

            case 1:
                if(!ch->desc && conn->account->descriptors.size() > 1 && conn->account->adminLevel < 1) {
                    sendText("You have reached the maximum number of active characters.\r\n");
                    return;
                }
                if (lockRead(GET_NAME(ch)) && conn->account->adminLevel <= 0) {
                    sendText("That character has been locked out for rule violations. Play another character.\n");
                    return;
                }
                conn->setParser(new PuppetParser(conn, ch));
                break;

            case 2:
                sendText("Temporarily disabled, sorry.\r\n");
                break;
            case 3:
                conn->setParser(new AccountMenu(conn));
                break;
            case 4:
                sendText("Temporarily disabled, sorry.\r\n");
                break;

            default:
                sendText(fmt::format("\r\nThat's not a menu choice!\r\n{}\r\n{}", motd, CONFIG_MENU));
                break;
        }

    }
}