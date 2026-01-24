#include "dbat/game/send.hpp"

#include "dbat/game/Descriptor.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/utils.hpp"
#include "volcano/util/FilterWeak.hpp"

#include "dbat/game/const/ConnectionState.hpp"
#include "dbat/game/const/Environment.hpp"

void send_to_all_impl(const std::string& message) {
    for(auto i = descriptor_list; i; i = i->next) {
        if(STATE(i) != CON_PLAYING) continue;
        i->sendText(message);
    }
}

void send_to_outdoor_impl(const std::string& message) {
    for(auto i = descriptor_list; i; i = i->next) {
        if(STATE(i) != CON_PLAYING || !(i->character)) continue;
        //If the character's current room isn't set as indoors, then send the message
        if(!AWAKE(i->character) || !OUTSIDE(i->character)) continue;
        i->sendText(message);
    }
}

void send_to_moon_impl(const std::string& message) {
    for(auto i = descriptor_list; i; i = i->next) {
        if(STATE(i) != CON_PLAYING || !(i->character)) continue;
        if (!AWAKE(i->character) || i->character->location.getEnvironment(ENV_MOONLIGHT) <= 0.0) continue;
        i->sendText(message);
    }
}

void send_to_planet_impl(int type, WhereFlag planet, const std::string& message) {
    for(auto i = descriptor_list; i; i = i->next) {
        if(STATE(i) != CON_PLAYING || !(i->character)) continue;
        if (!AWAKE(i->character) || !WHERE_FLAGGED(IN_ROOM(i->character), planet)) continue;
        if(type == 0) {
            i->sendText(message);
        } else if (OUTSIDE(i->character) && GET_SKILL(i->character, SKILL_SPOT) >= axion_dice(-5)) {
            i->sendText(message);
        }
    }
}

void send_to_range_impl(room_vnum start, room_vnum finish, const std::string& message) {
    for(auto r = start; r <= finish; r++) {
        auto room = get_room(r);
        if(room) room->sendText(message);
    }
}

void send_to_imm_impl(const std::string& message) {
    for(auto i = descriptor_list; i; i = i->next) {
            if(STATE(i) != CON_PLAYING) continue;
            if (GET_ADMLEVEL(i->character) == 0) continue;
            if (!PRF_FLAGGED(i->character, PRF_LOG2)) continue;
            if (PLR_FLAGGED(i->character, PLR_WRITING)) continue;
            i->output += "@g[ Log: ";
            i->output += message;
            i->output += " ]@n\r\n";
        }
        LINFO("%s", message);
}

void game_info_impl(const std::string& message) {
    auto messg = "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W" + message + "@n\r\n@R>>>@GMake sure to pick up your bed items and save.@n\r\n";

        for(auto i = descriptor_list; i; i = i->next) {
            if (STATE(i) != CON_PLAYING && (STATE(i) != CON_REDIT && STATE(i) != CON_OEDIT && STATE(i) != CON_MEDIT))
                continue;
            if (!(i->character))
                continue;
            i->output += messg;
        }
}