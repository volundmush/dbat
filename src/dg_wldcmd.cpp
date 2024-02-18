/**************************************************************************
*  File: dg_wldcmd.c                                                      *
*  Usage: contains the command_interpreter for rooms,                     *
*         room commands.                                                  *
*                                                                         *
*                                                                         *
*  $Author: galion/Mark A. Heilpern/egreen/Welcor $                       *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"

/*
 * Local functions
 */

#define WCMD(name)  \
    void (name)(Room *room, char *argument, int cmd, int subcmd)

void wld_log(Room *room, const char *format, ...);

void act_to_room(char *str, Room *room);

WCMD(do_wasound);

WCMD(do_wecho);

WCMD(do_wsend);

WCMD(do_wzoneecho);

WCMD(do_wrecho);

WCMD(do_wdoor);

WCMD(do_wteleport);

WCMD(do_wforce);

WCMD(do_wpurge);

WCMD(do_wload);

WCMD(do_wdamage);

WCMD(do_wat);

WCMD(do_weffect);

void wld_command_interpreter(Room *room, char *argument);


struct wld_command_info {
    char *command;

    void (*command_pointer)
            (Room *room, char *argument, int cmd, int subcmd);

    int subcmd;
};


/* do_wsend */
#define SCMD_WSEND        0
#define SCMD_WECHOAROUND  1


/* attaches room vnum to msg and sends it to script_log */
void wld_log(Room *room, const char *format, ...) {
    va_list args;
    char output[MAX_STRING_LENGTH];

    snprintf(output, sizeof(output), "Room %d :: %s", room->getVN(), format);

    va_start(args, format);
    script_vlog(output, args);
    va_end(args);
}

/* sends str to room */
void act_to_room(char *str, Room *room) {
    /* no one is in the room */
    auto people = room->getPeople();
    if (people.empty()) {
        wld_log(room, "No one is in the room.");
        return;
    }

    /*
     * since you can't use act(..., TO_ROOM) for an room, send it
     * TO_ROOM and TO_CHAR for some char in the room.
     * (just dont use $n or you might get strange results)
     */
    act(str, false, people.front(), nullptr, nullptr, TO_ROOM);
    act(str, false, people.front(), nullptr, nullptr, TO_CHAR);
}



/* World commands */

/* changes gravity, light, or lava levels of a room. */
WCMD(do_weffect) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int num = 0;
    room_rnum target, nr;

    two_arguments(argument, arg, arg2);

    if (!*arg) {
        wld_log(room, "weffect called without type argument");
        return;
    }

    if (!*arg2) {
        wld_log(room, "weffect called without setting argument");
        return;
    }

    num = atoi(arg2);

    nr = num;
    target = real_room(nr);

    if (!strcasecmp(arg, "gravity")) { /* Set gravity */
        if (num < 0 || num > 10000) {
            wld_log(room, "weffect setting out of bounds, 0 - 10000 only.");
            return;
        } else {
            if(num == 0) room->gravity.reset();
            else room->gravity = num;
        }
    } else if (!strcasecmp(arg, "light")) {
        if (target == NOWHERE) {
            wld_log(room, "weffect target is NOWHERE.");
            return;
        } else {
            room->flipFlag(FlagType::Room, ROOM_INDOORS);
        }
    } else if (!strcasecmp(arg, "lava")) {
        if (target == NOWHERE) {
            wld_log(room, "weffect target is NOWHERE.");
            return;
        } else {
            if (room->geffect != 0) {
                room->geffect = 5;
            } else {
                wld_log(room, "weffect target already has lava.");
                return;
            }
        }
    }

}

/* prints the argument to all the rooms aroud the room */
WCMD(do_wasound) {
    int door;

    skip_spaces(&argument);

    if (!*argument) {
        wld_log(room, "wasound called with no argument");
        return;
    }

    for (auto &[dir, ex] : room->getExits()) {
        auto dest = ex->getDestination();
        if(!dest) continue;

        act_to_room(argument, dest);
            
    }
}


WCMD(do_wecho) {
    skip_spaces(&argument);

    if (!*argument)
        wld_log(room, "wecho called with no args");

    else
        act_to_room(argument, room);
}


WCMD(do_wsend) {
    char buf[MAX_INPUT_LENGTH], *msg;
    BaseCharacter *ch;

    msg = any_one_arg(argument, buf);

    if (!*buf) {
        wld_log(room, "wsend called with no args");
        return;
    }

    skip_spaces(&msg);

    if (!*msg) {
        wld_log(room, "wsend called without a message");
        return;
    }

    if ((ch = get_char_by_room(room, buf))) {
        if (subcmd == SCMD_WSEND)
            sub_write(msg, ch, true, TO_CHAR);
        else if (subcmd == SCMD_WECHOAROUND)
            sub_write(msg, ch, true, TO_ROOM);
    } else
        wld_log(room, "no target found for wsend");
}

WCMD(do_wzoneecho) {
    zone_rnum zone;
    char room_num[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

    msg = any_one_arg(argument, room_num);
    skip_spaces(&msg);

    if (!*room_num || !*msg)
        wld_log(room, "wzoneecho called with too few args");

    else if ((zone = real_zone_by_thing(atoi(room_num))) == NOWHERE)
        wld_log(room, "wzoneecho called for nonexistant zone");

    else {
        sprintf(buf, "%s\r\n", msg);
        send_to_zone(buf, zone);
    }
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
WCMD(do_wrecho) {
    char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

    msg = two_arguments(argument, start, finish);

    skip_spaces(&msg);

    if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
        wld_log(room, "wrecho: too few args");
    else
        send_to_range(atoi(start), atoi(finish), "%s\r\n", msg);

}

WCMD(do_wdoor) {
    char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
    char field[MAX_INPUT_LENGTH], *value;
    Room *rm;
    Exit *newexit;
    int dir, fd, to_room;

    const char *door_field[] = {
            "purge",
            "description",
            "flags",
            "key",
            "name",
            "room",
            "\n"
    };


    argument = two_arguments(argument, target, direction);
    value = one_argument(argument, field);
    skip_spaces(&value);

    if (!*target || !*direction || !*field) {
        wld_log(room, "wdoor called with too few args");
        return;
    }
    if ((rm = get_room(target)) == nullptr) {
        wld_log(room, "wdoor: invalid target");
        return;
    }
    if (atoi(direction) >= 0 && atoi(direction) <= 11) {
        dir = atoi(direction);
    } else if (atoi(direction) < 0 && atoi(direction) > 11) {
        wld_log(room, "wdoor: invalid direction");
        return;
    }
    /*if ((dir = search_block(direction, dirs, FALSE)) < 0 && (dir = search_block(direction, abbr_dirs, FALSE)) < 0) {
        wld_log(room, "wdoor: invalid direction %d %s", dir, direction);
        return;
    }*/

    if ((fd = search_block(field, door_field, false)) == -1) {
        wld_log(room, "wdoor: invalid field");
        return;
    }

    auto exists = rm->getExits();

    Exit *ex = exists[dir];
    /* purge exit */
    if (fd == 0) {
        if (ex) {
            ex->extractFromWorld();
        }
    } else {
        if (!exists[dir]) {
            ex = new Exit();
            ex->uid = getNextUID();
            ex->script = std::make_shared<script_data>(ex);
            ex->addToLocation(rm, dir);
        }

        bitvector_t flags = 0;

        switch (fd) {
            case 1:  /* description */
                ex->setLookDesc(value);
                break;
            case 2:  /* flags       */
                flags = asciiflag_conv(value);
                for(auto i = 0; i < NUM_EXIT_FLAGS; i++) ex->setFlag(FlagType::Exit, i, IS_SET(flags, 1 << i));
                break;
            case 3:  /* key         */
                newexit->key = atoi(value);
                break;
            case 4:  /* name        */
                ex->setAlias(value);
                break;
            case 5:  /* room        */
                if ((to_room = real_room(atoi(value))) != NOWHERE)
                    newexit->destination = getWorld<Room>(to_room);
                else
                    wld_log(room, "wdoor: invalid door target");
                break;
        }
    }
}


WCMD(do_wteleport) {
    BaseCharacter *ch, *next_ch;
    room_rnum target, nr;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        wld_log(room, "wteleport called with too few args");
        return;
    }

    nr = atoi(arg2);
    target = real_room(nr);

    if (target == NOWHERE) {
        wld_log(room, "wteleport target is an invalid room");
        return;
    }
    auto r = getWorld<Room>(target);

    if (!strcasecmp(arg1, "all")) {
        if (nr == room->getUID()) {
            wld_log(room, "wteleport all target is itself");
            return;
        }

        for (auto ch : room->getPeople()) {
            if (!valid_dg_target(ch, DG_ALLOW_GODS))
                continue;
            ch->removeFromLocation();
            ch->addToLocation(r);
            enter_wtrigger(r, ch, -1);
        }
    } else {
        if ((ch = get_char_by_room(room, arg1))) {
            if (valid_dg_target(ch, DG_ALLOW_GODS)) {
                ch->removeFromLocation();
                ch->addToLocation(r);
                enter_wtrigger(r, ch, -1);
            }
        } else
            wld_log(room, "wteleport: no target found");
    }
}


WCMD(do_wforce) {
    BaseCharacter *ch, *next_ch;
    char arg1[MAX_INPUT_LENGTH], *line;

    line = one_argument(argument, arg1);

    if (!*arg1 || !*line) {
        wld_log(room, "wforce called with too few args");
        return;
    }

    if (!strcasecmp(arg1, "all")) {
        for (auto ch : room->getPeople()) {

            if (valid_dg_target(ch, 0)) {
                command_interpreter(ch, line);
            }
        }
    } else {
        if ((ch = get_char_by_room(room, arg1))) {
            if (valid_dg_target(ch, 0)) {
                command_interpreter(ch, line);
            }
        } else
            wld_log(room, "wforce: no target found");
    }
}


/* purge all objects an npcs in room, or specified object or mob */
WCMD(do_wpurge) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *ch, *next_ch;
    Object *obj, *next_obj;

    one_argument(argument, arg);

    if (!*arg) {
        /* purge all */
        for (auto ch : room->getPeople()) {
            if (IS_NPC(ch))
                extract_char(ch);
        }

        for (auto o : room->getInventory()) {
            obj->extractFromWorld();
        }

        return;
    }

    if (*arg == UID_CHAR)
        ch = get_char(arg);
    else
        ch = get_char_in_room(room, arg);

    if (!ch) {
        if (*arg == UID_CHAR)
            obj = get_obj(arg);
        else
            obj = get_obj_in_room(room, arg);

        if (obj) {
            obj->extractFromWorld();
        } else
            wld_log(room, "wpurge: bad argument");

        return;
    }

    if (!IS_NPC(ch)) {
        wld_log(room, "wpurge: purging a PC");
        return;
    }

    extract_char(ch);
}


/* loads a mobile or object into the room */
WCMD(do_wload) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0;
    BaseCharacter *mob;
    Object *object;
    char *target;
    BaseCharacter *tch;
    Object *cnt;
    int pos;

    target = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
        wld_log(room, "wload: bad syntax");
        return;
    }

    /* load mob to target room - Jamie Nelson, April 13 2004 */
    if (is_abbrev(arg1, "mob")) {
        room_rnum rnum;
        if (!target || !*target) {
            rnum = real_room(room->getUID());
        } else {
            if (!isdigit(*target) || (rnum = real_room(atoi(target))) == NOWHERE) {
                wld_log(room, "wload: room target vnum doesn't exist (loading mob vnum %d to room %s)", number, target);
                return;
            }
        }
        if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
            wld_log(room, "mload: bad mob vnum");
            return;
        }
        mob->addToLocation(getWorld(rnum));
        if (SCRIPT(room)) { /* It _should_ have, but it might be detached. */
            room->script->addVar("lastloaded", mob);
        }
        load_mtrigger(mob);
    } else if (is_abbrev(arg1, "obj")) {
        if ((object = read_object(number, VIRTUAL)) == nullptr) {
            wld_log(room, "wload: bad object vnum");
            return;
        }
        /* special handling to make objects able to load on a person/in a container/worn etc. */
        if (!target || !*target) {
            object->addToLocation(room);
            if (SCRIPT(room)) { /* It _should_ have, but it might be detached. */
                room->script->addVar("lastloaded", object);
            }
            load_otrigger(object);
            return;
        }

        two_arguments(target, arg1, arg2); /* recycling ... */
        tch = get_char_in_room(room, arg1);
        if (tch) {
            if (arg2 != nullptr && *arg2 &&
                (pos = find_eq_pos_script(arg2)) >= 0 &&
                !GET_EQ(tch, pos) &&
                can_wear_on_pos(object, pos)) {
                equip_char(tch, object, pos);
                load_otrigger(object);
                return;
            }
            object->addToLocation(tch);
            load_otrigger(object);
            return;
        }
        cnt = get_obj_in_room(room, arg1);
        if (cnt && GET_OBJ_TYPE(cnt) == ITEM_CONTAINER) {
            object->addToLocation(cnt);
            load_otrigger(object);
            return;
        }
        /* neither char nor container found - just dump it in room */
        object->addToLocation(room);
        load_otrigger(object);
        return;
    } else
        wld_log(room, "wload: bad type");
}

WCMD(do_wdamage) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
    int dam = 0;
    BaseCharacter *ch;

    two_arguments(argument, name, amount);

    /* who cares if it's a number ? if not it'll just be 0 */
    if (!*name || !*amount) {
        wld_log(room, "wdamage: bad syntax");
        return;
    }

    dam = atoi(amount);
    ch = get_char_by_room(room, name);

    if (!ch) {
        wld_log(room, "wdamage: target not found");
        return;
    }

    script_damage(ch, dam);
}

WCMD(do_wat) {
    room_rnum loc = NOWHERE;
    BaseCharacter *ch;
    char arg[MAX_INPUT_LENGTH], *command;

    command = any_one_arg(argument, arg);

    if (!*arg) {
        wld_log(room, "wat called with no args");
        return;
    }

    skip_spaces(&command);

    if (!*command) {
        wld_log(room, "wat called without a command");
        return;
    }

    if (isdigit(*arg)) {
        loc = real_room(atoi(arg));
    } else if ((ch = get_char_by_room(room, arg))) {
        loc = IN_ROOM(ch);
    }

    auto r = getWorld<Room>(loc);
    if (!r) {
        wld_log(room, "wat: location not found (%s)", arg);
        return;
    }

    wld_command_interpreter(room, command);
}

const struct wld_command_info wld_cmd_info[] = {
        {"RESERVED",     nullptr,      0},/* this must be first -- for specprocs */

        {"wasound ",     do_wasound,   0},
        {"wdoor ",       do_wdoor,     0},
        {"wecho ",       do_wecho,     0},
        {"wechoaround ", do_wsend, SCMD_WECHOAROUND},
        {"wforce ",      do_wforce,    0},
        {"wload ",       do_wload,     0},
        {"wpurge ",      do_wpurge,    0},
        {"wrecho ",      do_wrecho,    0},
        {"wsend ",       do_wsend, SCMD_WSEND},
        {"wteleport ",   do_wteleport, 0},
        {"wzoneecho ",   do_wzoneecho, 0},
        {"wdamage ",     do_wdamage,   0},
        {"wat ",         do_wat,       0},
        {"weffect ",     do_weffect,   0},
        {"\n",           nullptr,      0}        /* this must be last */
};


/*
 *  This is the command interpreter used by rooms, called by script_driver.
 */
void wld_command_interpreter(Room *room, char *argument) {
    int cmd, length;
    char *line, arg[MAX_INPUT_LENGTH];

    skip_spaces(&argument);

    /* just drop to next line for hitting CR */
    if (!*argument)
        return;

    line = any_one_arg(argument, arg);


    /* find the command */
    for (length = strlen(arg), cmd = 0;
         *wld_cmd_info[cmd].command != '\n'; cmd++)
        if (!strncmp(wld_cmd_info[cmd].command, arg, length))
            break;

    if (*wld_cmd_info[cmd].command == '\n')
        wld_log(room, "Unknown world cmd: '%s'", argument);
    else
        ((*wld_cmd_info[cmd].command_pointer)
                (room, line, cmd, wld_cmd_info[cmd].subcmd));
}
