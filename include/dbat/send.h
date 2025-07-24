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
            i->output += formatted_string;
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
            auto room = i->character->getRoom();
            if(!room) continue;
            //If the character's current room isn't set as indoors, then send the message
            if (!(ROOM_FLAGGED(room, ROOM_INDOORS))) {
                i->output += formatted_string;
            }
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
            if (!AWAKE(i->character) || i->character->getLocationEnvironment(ENV_MOONLIGHT) <= 0.0) continue;
            i->output += formatted_string;
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
                i->output += formatted_string;
            } else if (OUTSIDE(i->character) && GET_SKILL(i->character, SKILL_SPOT) >= axion_dice(-5)) {
                i->output += formatted_string;
            }
        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_planet: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }
}

template<typename... Args>
void send_to_room(struct room_data *room, fmt::string_view format, Args&&... args) {
    if(!room) return;

    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        for(auto i : filter_raw(room->getPeople())) {
            if(!i->desc) continue;
            i->desc->output += formatted_string;
        }

        for(auto d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CON_PLAYING)
                continue;

            if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
                if (arena_watch(d->character) == room->vn) {
                    d->output += "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n";
                    d->output += formatted_string;
                }
            }
            if (auto eaves = GET_EAVESDROP(d->character); eaves > 0) {
                int roll = rand_number(1, 101);
                if (eaves == room->vn && GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
                    d->output += "@c-----Eavesdrop-----@n\r\n%s\r\n@c-----Eavesdrop-----@n\r\n";
                    d->output += formatted_string;
                }
            }

        }
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in send_to_room: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
    }
}

template<typename... Args>
void send_to_room(room_rnum room, fmt::string_view format, Args&&... args) {
    if(auto r = get_room(room); r) {
        send_to_room(r, format, std::forward<Args>(args)...);
    }   
}

template<typename... Args>
void send_to_range(room_vnum start, room_vnum finish, fmt::string_view format, Args&&... args) {
    if (start > finish) {
        basic_mud_log("send_to_range passed start room value greater then finish.");
        return;
    }

    for(auto r = start; r <= finish; r++) {
        send_to_room(r, format, std::forward<Args>(args)...);
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

template<typename... Args>
size_t write_to_output(struct descriptor_data *t, fmt::string_view format, Args&&... args) {
    // Use fmt to format the string with the given arguments.
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        // Now send the formatted_string to wherever it needs to go.
        t->output += formatted_string;
        return formatted_string.size();
    }
    catch(const std::exception &e) {
        basic_mud_log("SYSERR: Format error in write_to_output: %s", e.what());
        basic_mud_log("Template was: %s", format.data());
        return 0;
    }
}

template<typename... Args>
size_t send_to_char(struct char_data *ch, fmt::string_view format, Args&&... args) {
    if(ch->desc) {
        return write_to_output(ch->desc, format, std::forward<Args>(args)...);
    }
    return 0;
}