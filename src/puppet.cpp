#include "puppet.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "dg_scripts.h"
#include "class.h"

namespace net {
    PuppetParser::PuppetParser(struct connection_data *co, char_data *c) : ConnectionParser(co) {
        ch = c;
    }

    void PuppetParser::start() {
        if(ch->desc) {
            // The character already has a descriptor! Let's just join up with it.
            conn->account->descriptors.insert(ch->desc);
            conn->desc = ch->desc;
            conn->desc->connection_mutex.lock();
            conn->desc->connections.insert(conn);
            conn->desc->connection_mutex.unlock();
            conn->sendText("Joining existing session...\r\n");
            return;
        }

        auto desc = new descriptor_data();
        desc->raw_input_queue = std::make_unique<Channel<std::string>>(*io, 200);
        desc->character = ch;
        ch->desc = desc;
        conn->account->descriptors.insert(desc);
        desc->connection_mutex.lock();
        desc->connections.insert(conn);
        desc->connection_mutex.unlock();
    }

    void PuppetParser::parse(const std::string &txt) {
        conn->desc->raw_input_queue.emplace_back(txt);
    }
}