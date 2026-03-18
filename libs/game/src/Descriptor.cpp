#include <unordered_map>
#include <memory>
#include "dbat/game/Database.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Character.hpp"
#include "dbat/game/players.hpp"


std::unordered_map<std::string, struct descriptor_data*> sessions;

int create_join_session(std::shared_ptr<GameConnectionInfo> info) {

    auto arows = dbat::db::txn->exec("SELECT * FROM users WHERE id = $1 LIMIT 1", pqxx::params{info->user_id});
    if(arows.empty()) {
        // no such user. This should never happen, but if it does, we can't create a session.
        return -1;
    }
    auto account = arows[0];

    auto ch_found = players.find(info->pc_id);
    if(ch_found == players.end()) return -1;
    auto &pl = ch_found->second;
    auto ch = pl->character;
    ch->player_flags.set(PLR_NOTDEADYET, false);
    auto exist_sess = sessions.find(info->pc_id);
    if(exist_sess != sessions.end()) {
        // a session already exists. we'll be joining it.
        auto &sess = exist_sess->second;
        if(sess->conns.empty()) {
            // the character is currently active, but link dead.
            sess->timeoutCounter = 0.0;
                ch->send_to("You have reconnected to %s from %s.\r\n", ch->getName(), info->ip_address);
        } else {
                ch->send_to("Another connection is now linked to %s, from %s.\r\n", ch->getName(), info->ip_address);
        }
        sess->conns.emplace(info->id, info);
        return sess->conns.size();
    } else {
        // no session exists. We'll have to create one.
        if(account["admin_level"].as<int>() < 1) {
            // non-admins can only have one character active at once.
            // Scan acc.descriptors for any with a character that isn't
            // this character_id.
            for(auto desc = descriptor_list; desc; desc = desc->next) {
                if(desc->id != info->pc_id) {
                    return -2;
                }
            }
        }

        auto desc = new descriptor_data();
        STATE(desc) = CON_LOGIN;
        desc->character = ch;
        ch->desc = desc;
        desc->id = info->pc_id;
        desc->conns.emplace(info->id, info);
        sessions.emplace(info->pc_id, desc);
        desc->next = descriptor_list;
        descriptor_list = desc;
        ch->send_to("You have connected to %s from %s.\r\n", ch->getName(), info->ip_address);
        return 1;
    }
}
