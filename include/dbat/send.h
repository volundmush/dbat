#pragma once
#include "dbat/utils.h"
#include "dbat/filter.h"

template<typename... Args>
void send_to_all(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        for(auto i = descriptor_list; i; i = i->next) {
            if(STATE(i) != CON_PLAYING) continue;
            i->sendText(formatted_string);
        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_all: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }
}

template<typename... Args>
void send_to_outdoor(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        for(auto i = descriptor_list; i; i = i->next) {
            if(STATE(i) != CON_PLAYING || !(i->character)) continue;
            //If the character's current room isn't set as indoors, then send the message
            if (i->location.getRoomFlag(ROOM_INDOORS)) continue;
            i->sendText(formatted_string);
        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_outdoor: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }
}

template<typename... Args>
void send_to_moon(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        for(auto i = descriptor_list; i; i = i->next) {
            if(STATE(i) != CON_PLAYING || !(i->character)) continue;
            if (!AWAKE(i->character) || i->character->location.getEnvironment(ENV_MOONLIGHT) <= 0.0) continue;
            i->sendText(formatted_string);
        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_moon: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }
}

template<typename... Args>
void send_to_planet(int type, WhereFlag planet, fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        for(auto i = descriptor_list; i; i = i->next) {
            if(STATE(i) != CON_PLAYING || !(i->character)) continue;
            if (!AWAKE(i->character) || !WHERE_FLAGGED(IN_ROOM(i->character), planet)) continue;
            if(type == 0) {
                i->sendText(formatted_string);
            } else if (OUTSIDE(i->character) && GET_SKILL(i->character, SKILL_SPOT) >= axion_dice(-5)) {
                i->sendText(formatted_string);
            }
        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_planet: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }
}


template<typename... Args>
void send_to_range(room_vnum start, room_vnum finish, fmt::string_view format, Args&&... args) {
    if (start > finish) {
        basic_mud_log("send_to_range passed start room value greater then finish.");
        return;
    }

    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        for(auto r = start; r <= finish; r++) {
            auto room = get_room(r);
            if(room) room->sendText(formatted_string);

        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_imm: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }

    

}

template<typename... Args>
void send_to_imm(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        for(auto i = descriptor_list; i; i = i->next) {
            if(STATE(i) != CON_PLAYING) continue;
            if (GET_ADMLEVEL(i->character) == 0) continue;
            if (!PRF_FLAGGED(i->character, PRF_LOG2)) continue;
            if (PLR_FLAGGED(i->character, PLR_WRITING)) continue;
            i->output += "@g[ Log: ";
            i->output += formatted_string;
            i->output += " ]@n\r\n";
        }
        basic_mud_log("%s", formatted_string.c_str());
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_imm: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }
}

template<typename... Args>
void game_info(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        auto messg = "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W" + formatted_string + "@n\r\n@R>>>@GMake sure to pick up your bed items and save.@n\r\n";

        for(auto i = descriptor_list; i; i = i->next) {
            if (STATE(i) != CON_PLAYING && (STATE(i) != CON_REDIT && STATE(i) != CON_OEDIT && STATE(i) != CON_MEDIT))
                continue;
            if (!(i->character))
                continue;
            i->output += messg;
        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in game_info: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }

}
