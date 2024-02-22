/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/
/**************************************************************************
*  File: dg_mobcmd.c                                                      *
*  Usage: contains the mobile script commands.                            *
*                                                                         *
*                                                                         *
*  $Author: N'Atas-ha/Mark A. Heilpern/egreen/Welcor $                    *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/db.h"
#include "dbat/utils.h"
#include "dbat/handler.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/constants.h"
#include "dbat/act.wizard.h"
#include "dbat/fight.h"
#include "dbat/transformation.h"

/*
 * Local functions.
 */
void mob_log(BaseCharacter *mob, const char *format, ...);

ACMD(do_masound);

ACMD(do_mkill);

ACMD(do_mheal);

ACMD(do_mjunk);

ACMD(do_mechoaround);

ACMD(do_msend);

ACMD(do_mecho);

ACMD(do_mzoneecho);

ACMD(do_mload);

ACMD(do_mpurge);

ACMD(do_mgoto);

ACMD(do_mat);

ACMD(do_mteleport);

ACMD(do_mdamage);

ACMD(do_mforce);

ACMD(do_mremember);

ACMD(do_mforget);

ACMD(do_mtransform);

ACMD(do_mdoor);

ACMD(do_mfollow);

ACMD(do_mrecho);

/* attaches mob's name and vnum to msg and sends it to script_log */
void mob_log(BaseCharacter *mob, const char *format, ...) {
    va_list args;
    char output[MAX_STRING_LENGTH];

    snprintf(output, sizeof(output), "Mob (%s [%d], VNum %d):: %s",
             GET_SHORT(mob), mob->getUID(), GET_MOB_VNUM(mob), format);

    va_start(args, format);
    script_vlog(output, args);
    va_end(args);
}

/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch) \
  (IS_NPC(ch) && (!(ch)->desc || GET_ADMLEVEL((ch)->desc->original)>=ADMLVL_IMPL))

/* mob commands */

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound) {
    room_rnum was_in_room;
    int door;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (!*argument) {
        mob_log(ch, "masound called with no argument");
        return;
    }

    skip_spaces(&argument);

    auto r = ch->getRoom();
    for (auto &[door, ex] : r->getExits()) {
        auto dest = ex->getDestination();
        if(!dest) continue;
        dest->sendTextContents(argument);
    }

}

/* Heals a stat of the mob */
ACMD(do_mheal) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
        mob_log(ch, "mheal called without an argument");
        return;
    }

    int64_t amount = 0;
    double num = atoi(arg2);
    double perc = num * 0.01;

    amount = GET_MAX_HIT(ch) * perc;

    if (!strcasecmp(arg, "pl")) {
        ch->incCurHealthPercent(num);
    } else if (!strcasecmp(arg, "ki")) {
        ch->incCurKIPercent(num);
    } else if (!strcasecmp(arg, "st")) {
        ch->incCurSTPercent(num);
    } else {
        mob_log(ch, "mheal called with wrong argument [pl | ki | st]");
        return;
    }
}

/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *victim;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mkill called with no argument");
        return;
    }

    if (*arg == UID_CHAR) {
        if (!(victim = get_char(arg))) {
            mob_log(ch, "mkill: victim (%s) not found", arg);
            return;
        }
    } else if (!(victim = get_char_room_vis(ch, arg, nullptr))) {
        mob_log(ch, "mkill: victim (%s) not found", arg);
        return;
    }

    if (victim == ch) {
        mob_log(ch, "mkill: victim is self");
        return;
    }

    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_NOHASSLE)) {
        mob_log(ch, "mkill: target has nohassle on");
        return;
    }

    sprintf(buf, "%s", GET_NAME(victim));
    if (IS_HUMANOID(ch)) {
        switch (rand_number(1, 7)) {
            case 1:
                do_punch(ch, buf, 0, 0);
                break;
            case 2:
                do_kick(ch, buf, 0, 0);
                break;
            case 3:
                do_elbow(ch, buf, 0, 0);
                break;
            case 4:
                do_knee(ch, buf, 0, 0);
                break;
            case 5:
                do_kick(ch, buf, 0, 0);
                break;
            default:
                do_punch(ch, buf, 0, 0);
                break;
        }//end switch
    } else {
        do_bite(ch, buf, 0, 0);
    }//end humanoid if
    return;
}


/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk) {
    char arg[MAX_INPUT_LENGTH];
    int pos, junk_all = 0;
    Object *obj;
    Object *obj_next;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mjunk called with no argument");
        return;
    }

    if (!strcasecmp(arg, "all")) junk_all = 1;

    if ((find_all_dots(arg) != FIND_INDIV) && !junk_all) {
        /* Thanks to Carlos Myers for fixing the line below */
        if ((pos = get_obj_pos_in_equip_vis(ch, arg, nullptr, ch->getEquipment())) >= 0) {
            unequip_char(ch, pos)->extractFromWorld();
            return;
        }
        if ((obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())) != nullptr)
            obj->extractFromWorld();
        return;
    } else {
        for (auto obj : ch->getInventory()) {
            if (arg[3] == '\0' || isname(arg + 4, obj->getName().c_str())) {
                obj->extractFromWorld();
            }
        }
        /* Thanks to Carlos Myers for fixing the line below */
        while ((pos = get_obj_pos_in_equip_vis(ch, arg, nullptr, ch->getEquipment())) >= 0)
            unequip_char(ch, pos)->extractFromWorld();
    }
    return;
}


/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *victim;
    char *p;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    p = one_argument(argument, arg);
    skip_spaces(&p);

    if (!*arg) {
        mob_log(ch, "mechoaround called with no argument");
        return;
    }

    if (*arg == UID_CHAR) {
        if (!(victim = get_char(arg))) {
            mob_log(ch, "mechoaround: victim (%s) does not exist", arg);
            return;
        }
    } else if (!(victim = get_char_room_vis(ch, arg, nullptr))) {
        mob_log(ch, "mechoaround: victim (%s) does not exist", arg);
        return;
    }

    char buf[MAX_STRING_LENGTH];

    sprintf(buf, p);
    search_replace(buf, GET_NAME(victim), "$n");
    act(buf, true, victim, nullptr, nullptr, TO_ROOM);
    /*sub_write(p, victim, TRUE, TO_ROOM);*/
}


/* sends the message to only the victim */
ACMD(do_msend) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *victim;
    char *p;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    p = one_argument(argument, arg);
    skip_spaces(&p);

    if (!*arg) {
        mob_log(ch, "msend called with no argument");
        return;
    }

    if (*arg == UID_CHAR) {
        if (!(victim = get_char(arg))) {
            mob_log(ch, "msend: victim (%s) does not exist", arg);
            return;
        }
    } else if (!(victim = get_char_room_vis(ch, arg, nullptr))) {
        mob_log(ch, "msend: victim (%s) does not exist", arg);
        return;
    }

    sub_write(p, victim, true, TO_CHAR);
}


/* prints the message to the room at large */
ACMD(do_mecho) {
    char *p;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (!*argument) {
        mob_log(ch, "mecho called with no arguments");
        return;
    }
    p = argument;
    skip_spaces(&p);

    sub_write(p, ch, true, TO_ROOM);
}

ACMD(do_mzoneecho) {
    int zone;
    char room_number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

    msg = any_one_arg(argument, room_number);
    skip_spaces(&msg);

    if (!*room_number || !*msg)
        mob_log(ch, "mzoneecho called with too few args");

    else if ((zone = real_zone_by_thing(atoi(room_number))) == NOWHERE)
        mob_log(ch, "mzoneecho called for nonexistant zone");

    else {
        sprintf(buf, "%s\r\n", msg);
        send_to_zone(buf, zone);
    }
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
ACMD(do_mload) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0;
    BaseCharacter *mob;
    Object *object;
    char *target;
    BaseCharacter *tch;
    Object *cnt;
    int pos;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL)
        return;

    target = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
        mob_log(ch, "mload: bad syntax");
        return;
    }

    /* load mob to target room - Jamie Nelson, April 13 2004 */
    if (is_abbrev(arg1, "mob")) {
        room_rnum rnum;
        if (!target || !*target) {
            rnum = IN_ROOM(ch);
        } else {
            if (!isdigit(*target) || (rnum = real_room(atoi(target))) == NOWHERE) {
                mob_log(ch, "mload: room target vnum doesn't exist "
                            "(loading mob vnum %d to room %s)", number, target);
                return;
            }
        }
        if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
            mob_log(ch, "mload: bad mob vnum");
            return;
        }
        mob->addToLocation(getEntity(rnum));
        if (SCRIPT(ch)) { /* It _should_ have, but it might be detached. */
            ch->script->addVar("lastloaded", mob);
        }
        load_mtrigger(mob);
    } else if (is_abbrev(arg1, "obj")) {
        if ((object = read_object(number, VIRTUAL)) == nullptr) {
            mob_log(ch, "mload: bad object vnum");
            return;
        }
        if (SCRIPT(ch)) { /* It _should_ have, but it might be detached. */
            ch->script->addVar("lastloaded", object);
        }
        randomize_eq(object);
        /* special handling to make objects able to load on a person/in a container/worn etc. */
        if (!target || !*target) {
            if (CAN_WEAR(object, ITEM_WEAR_TAKE)) {
                object->addToLocation(ch);
            } else {
                object->addToLocation(ch->getRoom());
            }
            load_otrigger(object);
            return;
        }
        two_arguments(target, arg1, arg2); /* recycling ... */
        tch = (arg1 != nullptr && *arg1 == UID_CHAR) ? get_char(arg1) : get_char_room_vis(ch, arg1, nullptr);
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
        cnt = (arg1 != nullptr && *arg1 == UID_CHAR) ? get_obj(arg1) : get_obj_vis(ch, arg1, nullptr);
        if (cnt && GET_OBJ_TYPE(cnt) == ITEM_CONTAINER) {
            object->addToLocation(cnt);
            load_otrigger(object);
            return;
        }
        /* neither char nor container found - just dump it in room */
        object->addToLocation(ch->getRoom());
        load_otrigger(object);
        return;
    } else
        mob_log(ch, "mload: bad type");
}


/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *victim;
    Object *obj;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        /* 'purge' */
        BaseCharacter *vnext;

        for (auto victim : ch->getRoom()->getPeople()) {
            if (IS_NPC(victim) && victim != ch)
                extract_char(victim);
        }

        for (auto obj : ch->getRoom()->getInventory()) {
            obj->extractFromWorld();
        }

        return;
    }

    if (*arg == UID_CHAR)
        victim = get_char(arg);
    else victim = get_char_room_vis(ch, arg, nullptr);

    if (victim == nullptr) {
        if (*arg == UID_CHAR)
            obj = get_obj(arg);
        else
            obj = get_obj_vis(ch, arg, nullptr);

        if (obj) {
            obj->extractFromWorld();
            obj = nullptr;
        } else
            mob_log(ch, "mpurge: bad argument");

        return;
    }

    if (!IS_NPC(victim)) {
        mob_log(ch, "mpurge: purging a PC");
        return;
    }

    extract_char(victim);
}


/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto) {
    char arg[MAX_INPUT_LENGTH];
    room_rnum location;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mgoto called with no argument");
        return;
    }

    if ((location = find_target_room(ch, arg)) == NOWHERE && GET_MOB_VNUM(ch) != 3) {
        mob_log(ch, "mgoto: invalid location");
        return;
    } else if ((location = find_target_room(ch, arg)) == NOWHERE) {
        return;
    }

    auto r = getEntity<Room>(location);

    if (FIGHTING(ch))
        stop_fighting(ch);

    ch->removeFromLocation();
    ch->addToLocation(r);
    enter_wtrigger(r, ch, -1);
}


/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat) {
    char arg[MAX_INPUT_LENGTH];
    room_rnum location;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    argument = one_argument(argument, arg);

    if (!*arg || !*argument) {
        mob_log(ch, "mat: bad argument");
        return;
    }

    if ((location = find_target_room(ch, arg)) == NOWHERE) {
        mob_log(ch, "mat: invalid location");
        return;
    }

    auto original = ch->getRoom();
    ch->removeFromLocation();
    ch->addToLocation(getEntity(location));
    ch->executeCommand(argument);

    /* See if 'ch' still exists before continuing! Handles 'at XXXX quit' case. */
    if (IN_ROOM(ch) == location) {
        ch->removeFromLocation();
        ch->addToLocation(original);
    }
}


/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    room_rnum target;
    BaseCharacter *vict, *next_ch;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        mob_log(ch, "mteleport: bad syntax");
        return;
    }

    target = find_target_room(ch, arg2);

    if (target == NOWHERE) {
        mob_log(ch, "mteleport target is an invalid room");
        return;
    }

    auto r = getEntity<Room>(target);

    if (!strcasecmp(arg1, "all")) {
        if (r == ch->getRoom()) {
            mob_log(ch, "mteleport all target is itself");
            return;
        }

        for (auto victim : ch->getRoom()->getPeople()) {

            if (valid_dg_target(vict, DG_ALLOW_GODS)) {
                vict->removeFromLocation();
                vict->addToLocation(r);
                enter_wtrigger(r, ch, -1);
            }
        }
    } else {
        if (*arg1 == UID_CHAR) {
            if (!(vict = get_char(arg1))) {
                mob_log(ch, "mteleport: victim (%s) does not exist", arg1);
                return;
            }
        } else if (!(vict = get_char_vis(ch, arg1, nullptr, FIND_CHAR_WORLD))) {
            mob_log(ch, "mteleport: victim (%s) does not exist", arg1);
            return;
        }

        if (valid_dg_target(ch, DG_ALLOW_GODS)) {
            vict->removeFromLocation();
            vict->addToLocation(r);
            enter_wtrigger(r, ch, -1);
        }
    }
}


ACMD(do_mdamage) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
    int dam = 0;
    BaseCharacter *vict;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    two_arguments(argument, name, amount);

    /* who cares if it's a number ? if not it'll just be 0 */
    if (!*name || !*amount) {
        mob_log(ch, "mdamage: bad syntax");
        return;
    }

    dam = atoi(amount);
    if (*name == UID_CHAR) {
        if (!(vict = get_char(name))) {
            mob_log(ch, "mdamage: victim (%s) does not exist", name);
            return;
        }
    } else if (!(vict = get_char_room_vis(ch, name, nullptr))) {
        mob_log(ch, "mdamage: victim (%s) does not exist", name);
        return;
    }
    script_damage(vict, dam);
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce) {
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    argument = one_argument(argument, arg);

    if (!*arg || !*argument) {
        mob_log(ch, "mforce: bad syntax");
        return;
    }

    if (!strcasecmp(arg, "all")) {
        struct descriptor_data *i;
        BaseCharacter *vch;

        for (i = descriptor_list; i; i = i->next) {
            if ((i->character != ch) && !i->connected &&
                (IN_ROOM(i->character) == IN_ROOM(ch))) {
                vch = i->character;
                if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) &&
                    valid_dg_target(vch, 0)) {
                    vch->executeCommand(argument);
                }
            }
        }
    } else {
        BaseCharacter *victim;

        if (*arg == UID_CHAR) {
            if (!(victim = get_char(arg))) {
                mob_log(ch, "mforce: victim (%s) does not exist", arg);
                return;
            }
        } else if ((victim = get_char_room_vis(ch, arg, nullptr)) == nullptr) {
            mob_log(ch, "mforce: no such victim");
            return;
        }

        if (victim == ch) {
            mob_log(ch, "mforce: forcing self");
            return;
        }

        if (valid_dg_target(victim, 0))
            victim->executeCommand(argument);
    }
}


/* place someone into the mob's memory list */
ACMD(do_mremember) {
    BaseCharacter *victim;
    struct script_memory *mem;
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    argument = one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mremember: bad syntax");
        return;
    }

    if (*arg == UID_CHAR) {
        if (!(victim = get_char(arg))) {
            mob_log(ch, "mremember: victim (%s) does not exist", arg);
            return;
        }
    } else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        mob_log(ch, "mremember: victim (%s) does not exist", arg);
        return;
    }

    /* create a structure and add it to the list */
    CREATE(mem, struct script_memory, 1);
    if (!SCRIPT_MEM(ch)) SCRIPT_MEM(ch) = mem;
    else {
        struct script_memory *tmpmem = SCRIPT_MEM(ch);
        while (tmpmem->next) tmpmem = tmpmem->next;
        tmpmem->next = mem;
    }

    /* fill in the structure */
    mem->id = ((victim)->getUID());
    if (argument && *argument) {
        mem->cmd = strdup(argument);
    }
}


/* remove someone from the list */
ACMD(do_mforget) {
    BaseCharacter *victim;
    struct script_memory *mem, *prev;
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mforget: bad syntax");
        return;
    }

    if (*arg == UID_CHAR) {
        if (!(victim = get_char(arg))) {
            mob_log(ch, "mforget: victim (%s) does not exist", arg);
            return;
        }
    } else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        mob_log(ch, "mforget: victim (%s) does not exist", arg);
        return;
    }

    mem = SCRIPT_MEM(ch);
    prev = nullptr;
    while (mem) {
        if (mem->id == ((victim)->getUID())) {
            if (mem->cmd) free(mem->cmd);
            if (prev == nullptr) {
                SCRIPT_MEM(ch) = mem->next;
                free(mem);
                mem = SCRIPT_MEM(ch);
            } else {
                prev->next = mem->next;
                free(mem);
                mem = prev->next;
            }
        } else {
            prev = mem;
            mem = mem->next;
        }
    }
}


/* transform into a different mobile */
ACMD(do_mtransform) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *m;
    NonPlayerCharacter tmpmob;
    Object *obj[NUM_WEARS];
    mob_rnum this_rnum = GET_MOB_RNUM(ch);
    int pos;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc) {
        ch->sendf("You've got no VNUM to return to, dummy! try 'switch'\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg)
        mob_log(ch, "mtransform: missing argument");
    else if (!isdigit(*arg) && *arg != '-')
        mob_log(ch, "mtransform: bad argument");
    else {
        if (isdigit(*arg))
            m = read_mobile(atoi(arg), VIRTUAL);
        else {
            m = read_mobile(atoi(arg + 1), VIRTUAL);
        }
        if (m == nullptr) {
            mob_log(ch, "mtransform: bad mobile vnum");
            return;
        }

        /* move new obj info over to old object and delete new obj */

        for (pos = 0; pos < NUM_WEARS; pos++) {
            if (GET_EQ(ch, pos))
                obj[pos] = unequip_char(ch, pos);
            else
                obj[pos] = nullptr;
        }

        /* put the mob in the same room as ch so extract will work */
        m->addToLocation(ch->getRoom());

        memcpy(&tmpmob, m, sizeof(*m));

        /* Thanks to Russell Ryan for this fix. RRfon we need to copy the
           the strings so we don't end up free'ing the prototypes later */
        if(auto n = m->getName(); !n.empty()) tmpmob.setName(n);
        if (m->title)
            tmpmob.title = strdup(m->title);
        
        if (auto sh = m->getShortDesc(); !sh.empty()) tmpmob.setShortDesc(sh);
        if (auto ld = m->getLookDesc(); !ld.empty()) tmpmob.setLookDesc(ld);
        if (auto d = m->getRoomDesc(); !d.empty()) tmpmob.setRoomDesc(d);

        tmpmob.affected = ch->affected;
        tmpmob.script = ch->script;
        tmpmob.memory = ch->memory;
        tmpmob.next = ch->next;
        tmpmob.next_fighting = ch->next_fighting;
        tmpmob.followers = ch->followers;
        tmpmob.master = ch->master;

        GET_WAS_IN(&tmpmob) = GET_WAS_IN(ch);
        tmpmob.set(CharMoney::Carried, GET_GOLD(ch));
        GET_POS(&tmpmob) = GET_POS(ch);
        FIGHTING(&tmpmob) = FIGHTING(ch);
        memcpy(ch, &tmpmob, sizeof(*ch));

        for (pos = 0; pos < NUM_WEARS; pos++) {
            if (obj[pos])
                equip_char(ch, obj[pos], pos);
        }

        ch->vn = this_rnum;
        extract_char(m);
    }
}


ACMD(do_maddtransform) {
    char name[MAX_INPUT_LENGTH], operation[MAX_INPUT_LENGTH], formName[MAX_INPUT_LENGTH];
    int dam = 0;
    BaseCharacter* vict;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    three_arguments(argument, name, operation, formName);

    /* who cares if it's a number ? if not it'll just be 0 */
    if (!*name || !*operation || !*formName) {
        mob_log(ch, "maddtransform: bad syntax (<name> <add/remove> <TFabbr>)");
        return;
    }

    if (!(vict = get_char(name))) {
        mob_log(ch, "maddtransform: victim (%s) does not exist", name);
        return;
    }

    bool add = true;
    if (operation == "add") {
        add = true;
    }
    else if (operation == "remove") {
        add = false;
    }
    else {
        mob_log(ch, "maddtransform: please specify whether to remove or add.", name);
    }

    std::string strForm(formName);

    auto foundForm = trans::findForm(ch, strForm);

    if(foundForm.has_value())
        if(add)
            ch->addTransform(*foundForm);
        else 
            ch->removeTransform(*foundForm);
}


ACMD(do_mdoor) {
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


    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    argument = two_arguments(argument, target, direction);
    value = one_argument(argument, field);
    skip_spaces(&value);

    if (!*target || !*direction || !*field) {
        mob_log(ch, "mdoor called with too few args");
        return;
    }

    if ((rm = get_room(target)) == nullptr) {
        mob_log(ch, "mdoor: invalid target");
        return;
    }

    if ((dir = search_block(direction, dirs, false)) == -1) {
        mob_log(ch, "mdoor: invalid direction");
        return;
    }

    if ((fd = search_block(field, door_field, false)) == -1) {
        mob_log(ch, "odoor: invalid field");
        return;
    }

    auto exits = rm->getExits();
    auto ex = exits[dir];

    /* purge exit */
    if (fd == 0) {
        if (ex) {
            ex->extractFromWorld();
        }
    } else {
        if (!ex) {
            ex = new Exit();
            ex->uid = getNextUID();
            setEntity(ex->getUID(), ex);
            ex->script = std::make_shared<script_data>(ex);
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
                ex->key = atoi(value);
                break;
            case 4:  /* name        */
                ex->setAlias(value);
                break;
            case 5:  /* room        */
                if ((to_room = real_room(atoi(value))) != NOWHERE)
                    ex->destination = getEntity<Room>(to_room);
                else
                    mob_log(ch, "mdoor: invalid door target");
                break;
        }
    }
}

ACMD(do_mfollow) {
    char buf[MAX_INPUT_LENGTH];
    BaseCharacter *leader;
    struct follow_type *j, *k;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, buf);

    if (!*buf) {
        mob_log(ch, "mfollow: bad syntax");
        return;
    }

    if (*buf == UID_CHAR) {
        if (!(leader = get_char(buf))) {
            mob_log(ch, "mfollow: victim (%s) does not exist", buf);
            return;
        }
    } else if (!(leader = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM))) {
        mob_log(ch, "mfollow: victim (%s) not found", buf);
        return;
    }

    if (ch->master == leader) /* already following */
        return;

    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))  /* can't override charm */
        return;


    /* stop following someone else first */
    if (ch->master) {
        if (ch->master->followers->follower == ch) {    /* Head of follower-list? */
            k = ch->master->followers;
            ch->master->followers = k->next;
            free(k);
        } else {            /* locate follower who is not head of list */
            for (k = ch->master->followers; k->next->follower != ch; k = k->next);

            j = k->next;
            k->next = j->next;
            free(j);
        }
        ch->master = nullptr;
    }

    if (ch == leader)
        return;

    if (circle_follow(ch, leader)) {
        mob_log(ch, "mfollow: Following in circles.");
        return;
    }

    ch->master = leader;

    CREATE(k, struct follow_type, 1);

    k->follower = ch;
    k->next = leader->followers;
    leader->followers = k;
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
ACMD(do_mrecho) {
    char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

    if (!MOB_OR_IMPL(ch)) {
        ch->sendf("Huh?!?\r\n");
        return;
    }
    msg = two_arguments(argument, start, finish);

    skip_spaces(&msg);

    if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
        mob_log(ch, "mrecho called with too few args");
    else
        send_to_range(atoi(start), atoi(finish), "%s\r\n", msg);
}
