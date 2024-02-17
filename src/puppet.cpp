#include "dbat/puppet.h"
#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/class.h"

namespace net {
    PuppetParser::PuppetParser(std::shared_ptr<Connection>& co, BaseCharacter *c) : ConnectionParser(co) {
        ch = c;
    }

    void PuppetParser::start() {
        ch->clearFlag(FlagType::PC, PLR_NOTDEADYET);
        if(ch->desc) {
            if(STATE(ch->desc) == CON_COPYOVER) return;

            // The character already has a descriptor! Let's just join up with it.
            if(ch->desc->conns.empty()) {
                // We are reviving a session that was in timeout.
                ch->desc->timeoutCounter = 0.0;
            }
            conn->account->descriptors.insert(ch->desc);
            conn->desc = ch->desc;
            conn->desc->conns[conn->connId] = conn;
            conn->sendText("Joining existing session...\r\n");
            return;
        }

        auto desc = new descriptor_data();
        STATE(desc) = CON_LOGIN;
        desc->character = ch;
        desc->id = ch->getUID();
        ch->desc = desc;
        conn->desc = desc;
        desc->account = conn->account;
        conn->account->descriptors.insert(desc);
        desc->conns[conn->connId] = conn;
        sessions[ch->getUID()] = desc;

        desc->next = descriptor_list;
        descriptor_list = desc;
    }

    void PuppetParser::parse(const std::string &txt) {
        conn->desc->raw_input_queue.push_back(txt);
    }
}