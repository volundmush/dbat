/**************************************************************************
*  File: dg_objcmd.c                                                      *
*  Usage: contains the command_interpreter for objects,                   *
*         object commands.                                                *
*                                                                         *
*                                                                         *
*  $Author: galion/Mark A. Heilpern/egreen/Welcor $                       *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include "dbat/structs.h"
#include "dbat/screen.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/constants.h"
#include "dbat/act.wizard.h"
#include "dbat/entity.h"

/*
 * Local functions
 */
#define OCMD(name)  \
   void (name)(Object *obj, char *argument, int cmd, int subcmd)

void obj_log(Object *obj, const char *format, ...);

room_rnum find_obj_target_room(Object *obj, char *rawroomstr);

OCMD(do_oecho);

OCMD(do_oforce);

OCMD(do_ozoneecho);

OCMD(do_osend);

OCMD(do_orecho);

OCMD(do_otimer);

OCMD(do_otransform);

OCMD(do_opurge);

OCMD(do_dupe);

OCMD(do_oteleport);

OCMD(do_dgoload);

OCMD(do_odamage);

OCMD(do_oasound);

OCMD(do_ogoto);

OCMD(do_odoor);

OCMD(do_osetval);

OCMD(do_oat);


struct obj_command_info {
    char *command;

    void (*command_pointer)(Object *obj, char *argument, int cmd, int subcmd);

    int subcmd;
};


/* do_osend */
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1


/* attaches object name and vnum to msg and sends it to script_log */
void obj_log(Object *obj, const char *format, ...) {
    va_list args;
    char output[MAX_STRING_LENGTH];

    snprintf(output, sizeof(output), "Obj (%s [%d], VNum %d):: %s", obj->getShortDesc().c_str(), obj->getUID(), GET_OBJ_VNUM(obj), format);

    va_start(args, format);
    script_vlog(output, args);
    va_end(args);
}

/* returns the real room number that the object or object's carrier is in */
room_rnum obj_room(Object *obj) {
    auto absolute = obj->getAbsoluteRoom();
    if (absolute) return absolute->getVN();
    return NOWHERE;
}


/* returns the real room number, or NOWHERE if not found or invalid */
room_rnum find_obj_target_room(Object *obj, char *rawroomstr) {
    int tmp;
    room_rnum location;
    Character *target_mob;
    Object *target_obj;
    char roomstr[MAX_INPUT_LENGTH];

    one_argument(rawroomstr, roomstr);

    if (!*roomstr)
        return NOWHERE;

    if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
        tmp = atoi(roomstr);
        if ((location = real_room(tmp)) == NOWHERE)
            return NOWHERE;
    } else if ((target_mob = get_char_by_obj(obj, roomstr)))
        location = IN_ROOM(target_mob);
    else if ((target_obj = get_obj_by_obj(obj, roomstr))) {
        if (IN_ROOM(target_obj) != NOWHERE)
            location = IN_ROOM(target_obj);
        else
            return NOWHERE;
    } else
        return NOWHERE;

    /* a room has been found.  Check for permission */
    if (ROOM_FLAGGED(location, ROOM_GODROOM) ||
        #ifdef ROOM_IMPROOM
        ROOM_FLAGGED(location, ROOM_IMPROOM) ||
        #endif
        ROOM_FLAGGED(location, ROOM_PRIVATE))
        return NOWHERE;

    return location;
}



/* Object commands */

OCMD(do_oecho) {
    skip_spaces(&argument);

    if (!*argument)
        obj_log(obj, "oecho called with no args");

    else if (auto room = obj->getRoom(); room) {
        room->sendTextContents(argument);
    } else
        obj_log(obj, "oecho called by object in NOWHERE");
}


OCMD(do_oforce) {
    Character *ch, *next_ch;
    char arg1[MAX_INPUT_LENGTH], *line;

    line = one_argument(argument, arg1);

    if (!*arg1 || !*line) {
        obj_log(obj, "oforce called with too few args");
        return;
    }

    if (!strcasecmp(arg1, "all")) {
        if (auto room = obj->getRoom(); room)
            obj_log(obj, "oforce called by object in NOWHERE");
        else {
            for (auto ch : room->getPeople()) {
                if (valid_dg_target(ch, 0)) {
                    ch->executeCommand(line);
                }
            }
        }
    } else {
        if ((ch = get_char_by_obj(obj, arg1))) {
            if (valid_dg_target(ch, 0)) {
                ch->executeCommand(line);
            }
        } else
            obj_log(obj, "oforce: no target found");
    }
}

OCMD(do_ozoneecho) {
    int zone;
    char room_number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

    msg = any_one_arg(argument, room_number);
    skip_spaces(&msg);

    if (!*room_number || !*msg)
        obj_log(obj, "ozoneecho called with too few args");

    else if ((zone = real_zone_by_thing(atoi(room_number))) == NOWHERE)
        obj_log(obj, "ozoneecho called for nonexistant zone");

    else {
        sprintf(buf, "%s\r\n", msg);
        send_to_zone(buf, zone);
    }
}

OCMD(do_osend) {
    char buf[MAX_INPUT_LENGTH], *msg;
    Character *ch;

    msg = any_one_arg(argument, buf);

    if (!*buf) {
        obj_log(obj, "osend called with no args");
        return;
    }

    skip_spaces(&msg);

    if (!*msg) {
        obj_log(obj, "osend called without a message");
        return;
    }

    if ((ch = get_char_by_obj(obj, buf))) {
        if (subcmd == SCMD_OSEND)
            sub_write(msg, ch, true, TO_CHAR);
        else if (subcmd == SCMD_OECHOAROUND) {
            char buf[MAX_STRING_LENGTH];

            sprintf(buf, msg);
            search_replace(buf, GET_NAME(ch), "$n");
            act(buf, true, ch, nullptr, nullptr, TO_ROOM);
            /*sub_write(msg, ch, TRUE, TO_ROOM);*/
        }
    } else
        obj_log(obj, "no target found for osend");
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
OCMD(do_orecho) {
    char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

    msg = two_arguments(argument, start, finish);

    skip_spaces(&msg);

    if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
        obj_log(obj, "orecho: too few args");
    else
        send_to_range(atoi(start), atoi(finish), "%s\r\n", msg);

}


/* set the object's timer value */
OCMD(do_otimer) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg)
        obj_log(obj, "otimer: missing argument");
    else if (!isdigit(*arg))
        obj_log(obj, "otimer: bad argument");
    else
        GET_OBJ_TIMER(obj) = atoi(arg);
}


/* transform into a different object */
/* note: this shouldn't be used with containers unless both objects */
/* are containers! */
OCMD(do_otransform) {
    obj_log(obj, "otransform: currently disabled");
}

OCMD(do_dupe) {

    obj->setFlag(FlagType::Item, ITEM_DUPLICATE);
}

/* purge all objects an npcs in room, or specified object or mob */
OCMD(do_opurge) {
    char arg[MAX_INPUT_LENGTH];
    Character *ch, *next_ch;

    one_argument(argument, arg);

    if (!*arg) {
        /* purge all */
        if (auto rm = obj->getRoom(); rm) {
            for (auto ch : rm->getPeople()) {
                if (IS_NPC(ch))
                    extract_char(ch);
            }

            for (auto o : rm->getInventory()) {
                if (o != obj)
                    o->extractFromWorld();
            }
        }

        return;
    } /* no arg */

    ch = get_char_by_obj(obj, arg);
    if (!ch) {
        auto o = get_obj_by_obj(obj, arg);
        if (o) {
            o->extractFromWorld();
        } else
            obj_log(obj, "opurge: bad argument");

        return;
    }

    if (!IS_NPC(ch)) {
        obj_log(obj, "opurge: purging a PC");
        return;
    }

    extract_char(ch);
}

/* (ogoto) Written by Iovan -- 4/5/10 
   Allows objects to move into specific rooms
   on their own.                             */
OCMD(do_ogoto) {
    room_rnum target;
    char arg1[MAX_INPUT_LENGTH];

    one_argument(argument, arg1);

    if (!*arg1) {
        obj_log(obj, "ogoto called with too few args");
        return;
    }

    target = find_obj_target_room(obj, arg1);

    if (target == NOWHERE) {
        obj_log(obj, "ogoto target '%s' is an invalid room", arg1);
    } else if (IN_ROOM(obj) == NOWHERE) {
        obj_log(obj, "ogoto tried to leave nowhere");
    } else {
        obj->removeFromLocation();
        obj->addToLocation(getEntity(target));
    }
}

OCMD(do_oteleport) {
    Character *ch, *next_ch;
    room_rnum target;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        obj_log(obj, "oteleport called with too few args");
        return;
    }

    target = find_obj_target_room(obj, arg2);

    if (target == NOWHERE) {
        obj_log(obj, "oteleport target is an invalid room");
        return;
    }
    auto r = getEntity<Room>(target);

    if (!strcasecmp(arg1, "all")) {
        auto rm = obj->getRoom();
        if (target == rm->getVN())
            obj_log(obj, "oteleport target is itself");

        for (auto ch : rm->getPeople()) {
            if (!valid_dg_target(ch, DG_ALLOW_GODS))
                continue;
            ch->removeFromLocation();
            ch->addToLocation(r);
            enter_wtrigger(r, ch, -1);
        }
    } else {
        if ((ch = get_char_by_obj(obj, arg1))) {
            if (valid_dg_target(ch, DG_ALLOW_GODS)) {
                ch->removeFromLocation();
                ch->addToLocation(r);
                enter_wtrigger(r, ch, -1);
            }
        } else
            obj_log(obj, "oteleport: no target found");
    }
}


OCMD(do_dgoload) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0, room;
    Character *mob;
    Object *object;
    char *target;
    Character *tch;
    Object *cnt;
    int pos;

    target = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
        obj_log(obj, "oload: bad syntax");
        return;
    }

    if ((room = obj_room(obj)) == NOWHERE) {
        obj_log(obj, "oload: object in NOWHERE trying to load");
        return;
    }

    /* load mob to target room - Jamie Nelson, April 13 2004 */
    if (is_abbrev(arg1, "mob")) {
        room_rnum rnum;
        if (!target || !*target) {
            rnum = room;
        } else {
            if (!isdigit(*target) || (rnum = real_room(atoi(target))) == NOWHERE) {
                obj_log(obj, "oload: room target vnum doesn't exist "
                             "(loading mob vnum %d to room %s)", number, target);
                return;
            }
        }
        if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
            obj_log(obj, "oload: bad mob vnum");
            return;
        }
        mob->addToLocation(getEntity(rnum));

        if (SCRIPT(obj)) { /* It _should_ have, but it might be detached. */
            obj->script->addVar("lastloaded", mob);
        }

        load_mtrigger(mob);
    } else if (is_abbrev(arg1, "obj")) {
        if ((object = read_object(number, VIRTUAL)) == nullptr) {
            obj_log(obj, "oload: bad object vnum");
            return;
        }

        if (SCRIPT(obj)) { /* It _should_ have, but it might be detached. */
            obj->script->addVar("lastloaded", object);
        }

        /* special handling to make objects able to load on a person/in a container/worn etc. */
        object->addToLocation(getEntity(room));
        load_otrigger(object);
        return;
    } else
        obj_log(obj, "oload: bad type");

}

OCMD(do_odamage) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
    int dam = 0;
    Character *ch;

    two_arguments(argument, name, amount);

    /* who cares if it's a number ? if not it'll just be 0 */
    if (!*name || !*amount) {
        obj_log(obj, "odamage: bad syntax");
        return;
    }

    dam = atoi(amount);
    ch = get_char_by_obj(obj, name);

    if (!ch) {
        obj_log(obj, "odamage: target not found");
        return;
    }
    script_damage(ch, dam);
}


OCMD(do_oasound) {
    room_rnum room;
    int door;

    skip_spaces(&argument);

    if (!*argument) {
        obj_log(obj, "oasound called with no args");
        return;
    }

    auto r = obj->getRoom();

    if (!r) {
        obj_log(obj, "oecho called by object in NOWHERE");
        return;
    }

    for (auto &[door, ex] : r->getExits()) {
        auto dest = reg.try_get<Destination>(ex->ent);
        if(!dest) continue;
        send::textContents(dest->target, argument);
    }
}


OCMD(do_odoor) {
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
        obj_log(obj, "odoor called with too few args");
        return;
    }

    if ((rm = get_room(target)) == nullptr) {
        obj_log(obj, "odoor: invalid target");
        return;
    }

    if ((dir = search_block(direction, dirs, false)) == -1) {
        obj_log(obj, "odoor: invalid direction");
        return;
    }

    if ((fd = search_block(field, door_field, false)) == -1) {
        obj_log(obj, "odoor: invalid field");
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
        if (!ex) {
            ex = new Exit();
            ex->uid = getNextUID();
            ex->script = std::make_shared<script_data>(ex);
            Destination dest;
            dest.target = rm->ent;
            dest.locationType = dir;
            ex->addToLocation(dest);
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
                if ((to_room = real_room(atoi(value))) != NOWHERE) {
                    auto &dest = reg.get_or_emplace<Destination>(newexit->ent);
                    dest.target = entities.at(to_room);
                    dest.direction = dir;
                }
                else
                    obj_log(obj, "odoor: invalid door target");
                break;
        }
    }
}


OCMD(do_osetval) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int position, new_value;

    two_arguments(argument, arg1, arg2);
    if (arg1 == nullptr || !*arg1 || arg2 == nullptr || !*arg2 ||
        !is_number(arg1) || !is_number(arg2)) {
        obj_log(obj, "osetval: bad syntax");
        return;
    }

    position = atoi(arg1);
    new_value = atoi(arg2);
    if (position >= 0 && position < NUM_OBJ_VAL_POSITIONS)
        GET_OBJ_VAL(obj, position) = new_value;
    else
        obj_log(obj, "osetval: position out of bounds!");
}

/* submitted by PurpleOnyx - tkhasi@shadowglen.com*/
OCMD(do_oat) {
    room_rnum loc = NOWHERE;
    Character *ch;
    Object *object;
    char arg[MAX_INPUT_LENGTH], *command;

    command = any_one_arg(argument, arg);

    if (!*arg) {
        obj_log(obj, "oat called with no args");
        return;
    }

    skip_spaces(&command);

    if (!*command) {
        obj_log(obj, "oat called without a command");
        return;
    }

    if (isdigit(*arg))
        loc = real_room(atoi(arg));
    else if ((ch = get_char_by_obj(obj, arg)))
        loc = IN_ROOM(ch);

    if (loc == NOWHERE) {
        obj_log(obj, "oat: location not found (%s)", arg);
        return;
    }

    if (!(object = read_object(GET_OBJ_VNUM(obj), VIRTUAL)))
        return;
    object->addToLocation(getEntity(loc));
    object->executeCommand(command);

    if (IN_ROOM(object) == loc)
        object->extractFromWorld();
}

const struct obj_command_info obj_cmd_info[] = {
        {"RESERVED",     nullptr,       0},/* this must be first -- for specprocs */

        {"oasound ",     do_oasound,    0},
        {"oat ",         do_oat,        0},
        {"odoor ",       do_odoor,      0},
        {"odupe ",       do_dupe,       0},
        {"odamage ",     do_odamage,    0},
        {"oecho ",       do_oecho,      0},
        {"oechoaround ", do_osend, SCMD_OECHOAROUND},
        {"oforce ",      do_oforce,     0},
        {"ogoto ",       do_ogoto,      0},
        {"oload ",       do_dgoload,    0},
        {"opurge ",      do_opurge,     0},
        {"orecho ",      do_orecho,     0},
        {"osend ",       do_osend, SCMD_OSEND},
        {"osetval ",     do_osetval,    0},
        {"oteleport ",   do_oteleport,  0},
        {"otimer ",      do_otimer,     0},
        {"otransform ",  do_otransform, 0},
        {"ozoneecho ",   do_ozoneecho,  0}, /* fix by Rumble */

        {"\n",           nullptr,       0}        /* this must be last */
};


/*
 *  This is the command interpreter used by objects, called by script_driver.
 */
void Object::executeCommand(const std::string& argument) {
    int cmd, length;
    char *line, arg[MAX_INPUT_LENGTH];

    std::string complete(argument);
    trim(complete);

    /* just drop to next line for hitting CR */
    if (complete.empty())
        return;

    line = any_one_arg((char*)complete.c_str(), arg);


    /* find the command */
    for (length = strlen(arg), cmd = 0;
         *obj_cmd_info[cmd].command != '\n'; cmd++)
        if (!strncmp(obj_cmd_info[cmd].command, arg, length))
            break;

    if (*obj_cmd_info[cmd].command == '\n')
        obj_log(this, "Unknown object cmd: '%s'", argument);
    else
        ((*obj_cmd_info[cmd].command_pointer)
                (this, line, cmd, obj_cmd_info[cmd].subcmd));
}
