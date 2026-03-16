#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Account.hpp"
#include "dbat/game/Character.hpp"



std::map<int64_t, struct descriptor_data*> sessions;

int create_join_session(std::shared_ptr<GameConnectionInfo> info) {
    auto acc_found = accounts.find(info->account_id);
    if(acc_found == accounts.end()) return -1;
    auto &acc = acc_found->second;
    auto ch_found = Character::registry.find(info->character_id);
    if(ch_found == Character::registry.end()) return -1;
    auto &ch = ch_found->second;
    ch->player_flags.set(PLR_NOTDEADYET, false);
    auto exist_sess = sessions.find(info->character_id);
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
        acc->descriptors.insert(sess);
        return sess->conns.size();
    } else {
        // no session exists. We'll have to create one.
        if(acc->admin_level < 1) {
            // non-admins can only have one character active at once.
            // Scan acc.descriptors for any with a character that isn't
            // this character_id.
            for(auto desc : acc->descriptors) {
                if(desc->id != info->character_id) {
                    return -2;
                }
            }
        }

        auto desc = new descriptor_data();
        STATE(desc) = CON_LOGIN;
        desc->character = ch.get();
        ch->desc = desc;
        desc->account = acc.get();
        desc->id = info->character_id;
        desc->pc_id = info->pc_id;
        desc->conns.emplace(info->id, info);
        acc->descriptors.insert(desc);
        sessions.emplace(info->character_id, desc);
        desc->next = descriptor_list;
        descriptor_list = desc;
        ch->send_to("You have connected to %s from %s.\r\n", ch->getName(), info->ip_address);
        return 1;
    }
}
