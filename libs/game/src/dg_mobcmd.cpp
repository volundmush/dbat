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
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/game/db.hpp"
#include "dbat/game/send.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/act.wizard.hpp"
#include "dbat/game/fight.hpp"
#include "dbat/game/transformation.hpp"
#include "volcano/util/FilterWeak.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/Random.hpp"

/*
 * Local functions.
 */
void mob_log(Character *mob, const char *format, ...);

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
void mob_log(Character *mob, const char *format, ...)
{
    va_list args;
    char output[MAX_STRING_LENGTH];

    snprintf(output, sizeof(output), "Mob (%s [%ld], VNum %d):: %s",
             GET_SHORT(mob), mob->id, GET_MOB_VNUM(mob), format);

    va_start(args, format);
    script_vlog(output, args);
    va_end(args);
}

/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch) \
    (IS_NPC(ch) && (!(ch)->desc || GET_ADMLEVEL((ch)->desc->original) >= ADMLVL_IMPL))

/* mob commands */

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound)
{

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (!*argument)
    {
        mob_log(ch, "masound called with no argument");
        return;
    }

    skip_spaces(&argument);

    auto loc = ch->location;
    int counter = 0;
    for (auto &[door, ex] : loc.getExits())
    {
        if (ex == loc)
            continue;
        ch->leaveLocation();
        ch->moveToLocation(ex);
        sub_write(argument, ch, true, TO_ROOM);
        counter++;
    }

    if (counter > 0)
    {
        // put the mob back where it started.
        ch->leaveLocation();
        ch->moveToLocation(loc);
    }
}

/* Heals a stat of the mob */
ACMD(do_mheal)
{
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        mob_log(ch, "mheal called without an argument");
        return;
    }

    int64_t amount = 0;
    double num = atoi(arg2);
    double perc = num * 0.01;

    amount = GET_MAX_HIT(ch) * perc;

    if (boost::iequals(arg, "pl"))
    {
        ch->modCurVitalDam(CharVital::health, -num);
    }
    else if (boost::iequals(arg, "ki"))
    {
        ch->modCurVitalDam(CharVital::ki, -num);
    }
    else if (boost::iequals(arg, "st"))
    {
        ch->modCurVitalDam(CharVital::stamina, -num);
    }
    else
    {
        mob_log(ch, "mheal called with wrong argument [pl | ki | st]");
        return;
    }
}

/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg)
    {
        mob_log(ch, "mkill called with no argument");
        return;
    }

    if (*arg == UID_CHAR)
    {
        if (!(victim = get_char(arg)))
        {
            mob_log(ch, "mkill: victim (%s) not found", arg);
            return;
        }
    }
    else if (!(victim = get_char_room_vis(ch, arg, nullptr)))
    {
        mob_log(ch, "mkill: victim (%s) not found", arg);
        return;
    }

    if (victim == ch)
    {
        mob_log(ch, "mkill: victim is self");
        return;
    }

    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_NOHASSLE))
    {
        mob_log(ch, "mkill: target has nohassle on");
        return;
    }

    sprintf(buf, "%s", GET_NAME(victim));
    if (IS_HUMANOID(ch))
    {
        switch (Random::get<int>(1, 7))
        {
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
        } // end switch
    }
    else
    {
        do_bite(ch, buf, 0, 0);
    } // end humanoid if
    return;
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk)
{
    char arg[MAX_INPUT_LENGTH];
    int pos, junk_all = 0;
    Object *obj;
    Object *obj_next;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg)
    {
        mob_log(ch, "mjunk called with no argument");
        return;
    }

    if (boost::iequals(arg, "all"))
        junk_all = 1;

    if ((find_all_dots(arg) != FIND_INDIV) && !junk_all)
    {
        /* Thanks to Carlos Myers for fixing the line below */
        if ((pos = get_obj_pos_in_equip_vis(ch, arg, nullptr, ch->getEquipment())) >= 0)
        {
            extract_obj(unequip_char(ch, pos));
            return;
        }
        if ((obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
            extract_obj(obj);
        return;
    }
    else
    {
        auto con = ch->getInventory();
        for (auto obj : volcano::util::filter_raw(con))
        {
            if (arg[3] == '\0' || isname(arg + 4, obj->getName()))
            {
                extract_obj(obj);
            }
        }
        /* Thanks to Carlos Myers for fixing the line below */
        while ((pos = get_obj_pos_in_equip_vis(ch, arg, nullptr, ch->getEquipment())) >= 0)
            extract_obj(unequip_char(ch, pos));
    }
    return;
}

/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    char *p;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    p = one_argument(argument, arg);
    skip_spaces(&p);

    if (!*arg)
    {
        mob_log(ch, "mechoaround called with no argument");
        return;
    }

    if (*arg == UID_CHAR)
    {
        if (!(victim = get_char(arg)))
        {
            mob_log(ch, "mechoaround: victim (%s) does not exist", arg);
            return;
        }
    }
    else if (!(victim = get_char_room_vis(ch, arg, nullptr)))
    {
        mob_log(ch, "mechoaround: victim (%s) does not exist", arg);
        return;
    }

    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "%s", p);
    search_replace(buf, GET_NAME(victim), "$n");
    act(buf, true, victim, nullptr, nullptr, TO_ROOM);
    /*sub_write(p, victim, TRUE, TO_ROOM);*/
}

/* sends the message to only the victim */
ACMD(do_msend)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    char *p;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    p = one_argument(argument, arg);
    skip_spaces(&p);

    if (!*arg)
    {
        mob_log(ch, "msend called with no argument");
        return;
    }

    if (*arg == UID_CHAR)
    {
        if (!(victim = get_char(arg)))
        {
            mob_log(ch, "msend: victim (%s) does not exist", arg);
            return;
        }
    }
    else if (!(victim = get_char_room_vis(ch, arg, nullptr)))
    {
        mob_log(ch, "msend: victim (%s) does not exist", arg);
        return;
    }

    sub_write(p, victim, true, TO_CHAR);
}

/* prints the message to the room at large */
ACMD(do_mecho)
{
    char *p;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (!*argument)
    {
        mob_log(ch, "mecho called with no arguments");
        return;
    }
    p = argument;
    skip_spaces(&p);

    sub_write(p, ch, true, TO_ROOM);
}

ACMD(do_mzoneecho)
{
    int zone;
    char room_number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

    msg = any_one_arg(argument, room_number);
    skip_spaces(&msg);

    if (!*room_number || !*msg)
    {
        mob_log(ch, "mzoneecho called with too few args");
        return;
    }

    auto r = get_room(atoi(room_number));
    if (!r)
    {
        mob_log(ch, "mzoneecho called for nonexistant zone");
        return;
    }

    r->getZone()->send_to("%s\r\n", msg);
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
ACMD(do_mload)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0;
    Character *mob;
    Object *object;
    char *target;
    Character *tch;
    Object *cnt;
    int pos;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL)
        return;

    target = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0))
    {
        mob_log(ch, "mload: bad syntax");
        return;
    }

    /* load mob to target room - Jamie Nelson, April 13 2004 */
    if (is_abbrev(arg1, "mob"))
    {
        room_rnum rnum;
        if (!target || !*target)
        {
            rnum = IN_ROOM(ch);
        }
        else
        {
            if (!isdigit(*target) || (rnum = real_room(atoi(target))) == NOWHERE)
            {
                mob_log(ch, "mload: room target vnum doesn't exist "
                            "(loading mob vnum %d to room %s)",
                        number, target);
                return;
            }
        }
        if ((mob = read_mobile(number, VIRTUAL)) == nullptr)
        {
            mob_log(ch, "mload: bad mob vnum");
            return;
        }
        mob->moveToLocation(rnum);
        if (SCRIPT(ch))
        { /* It _should_ have, but it might be detached. */
            ch->setVariable("lastloaded", mob->getUID(true));
        }
        load_mtrigger(mob);
    }
    else if (is_abbrev(arg1, "obj"))
    {
        if ((object = read_object(number, VIRTUAL)) == nullptr)
        {
            mob_log(ch, "mload: bad object vnum");
            return;
        }
        if (SCRIPT(ch))
        { /* It _should_ have, but it might be detached. */
            ch->setVariable("lastloaded", object->getUID(true));
        }
        randomize_eq(object);
        /* special handling to make objects able to load on a person/in a container/worn etc. */
        if (!target || !*target)
        {
            if (CAN_WEAR(object, ITEM_WEAR_TAKE))
            {
                ch->addToInventory(object);
            }
            else
            {
                object->moveToLocation(ch);
            }
            load_otrigger(object);
            return;
        }
        two_arguments(target, arg1, arg2); /* recycling ... */
        tch = (arg1[0] == UID_CHAR) ? get_char(arg1) : get_char_room_vis(ch, arg1, nullptr);
        if (tch)
        {
            if (arg2[0] &&
                (pos = find_eq_pos_script(arg2)) >= 0 &&
                !GET_EQ(tch, pos) &&
                can_wear_on_pos(object, pos))
            {
                equip_char(tch, object, pos);
                load_otrigger(object);
                return;
            }
            tch->addToInventory(object);
            load_otrigger(object);
            return;
        }
        cnt = (arg1[0] == UID_CHAR) ? get_obj(arg1) : get_obj_vis(ch, arg1, nullptr);
        if (cnt && GET_OBJ_TYPE(cnt) == ITEM_CONTAINER)
        {
            cnt->addToInventory(object);
            load_otrigger(object);
            return;
        }
        /* neither char nor container found - just dump it in room */
        object->moveToLocation(ch);
        load_otrigger(object);
        return;
    }
    else
        mob_log(ch, "mload: bad type");
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    Object *obj;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    one_argument(argument, arg);

    if (!*arg)
    {
        /* 'purge' */
        Character *vnext;
        Object *obj_next;

        auto people = ch->location.getPeople();
        for (auto victim : volcano::util::filter_raw(people))
        {
            if (IS_NPC(victim) && victim != ch)
                extract_char(victim);
        }

        auto locobjs = ch->location.getObjects();
        for (auto obj : volcano::util::filter_raw(locobjs))
        {
            extract_obj(obj);
        }

        return;
    }

    if (*arg == UID_CHAR)
        victim = get_char(arg);
    else
        victim = get_char_room_vis(ch, arg, nullptr);

    if (victim == nullptr)
    {
        if (*arg == UID_CHAR)
            obj = get_obj(arg);
        else
            obj = get_obj_vis(ch, arg, nullptr);

        if (obj)
        {
            extract_obj(obj);
            obj = nullptr;
        }
        else
            mob_log(ch, "mpurge: bad argument");

        return;
    }

    if (!IS_NPC(victim))
    {
        mob_log(ch, "mpurge: purging a PC");
        return;
    }

    if (victim == ch)
        dg_owner_purged = 1;

    extract_char(victim);
}

/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto)
{
    char arg[MAX_INPUT_LENGTH];
    Location location;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg)
    {
        mob_log(ch, "mgoto called with no argument");
        return;
    }

    if(auto loc = Location(arg)) {
        if (FIGHTING(ch))
            stop_fighting(ch);
        ch->leaveLocation();
        ch->moveToLocation(loc);
        return;
    }

    if (!(location = find_target_location(ch, arg)) && GET_MOB_VNUM(ch) != 3)
    {
        mob_log(ch, "mgoto: invalid location");
        return;
    }
    else if (!(location = find_target_location(ch, arg)))
    {
        return;
    }

    if (FIGHTING(ch))
        stop_fighting(ch);

    ch->leaveLocation();
    ch->moveToLocation(location);
    enter_wtrigger(ch->getRoom(), ch, -1);
}

/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat)
{
    char arg[MAX_INPUT_LENGTH];
    Location location;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    argument = one_argument(argument, arg);

    if (!*arg || !*argument)
    {
        mob_log(ch, "mat: bad argument");
        return;
    }

    if (!(location = find_target_location(ch, arg)))
    {
        mob_log(ch, "mat: invalid location");
        return;
    }

    auto original = ch->location;
    ch->leaveLocation();
    ch->moveToLocation(location);
    command_interpreter(ch, argument);

    /* See if 'ch' still exists before continuing! Handles 'at XXXX quit' case. */
    if (ch->location == location)
    {
        ch->leaveLocation();
        ch->moveToLocation(original);
    }
}

/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Location target;
    Character *vict, *next_ch;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2)
    {
        mob_log(ch, "mteleport: bad syntax");
        return;
    }

    target = find_target_location(ch, arg2);

    if (!target)
    {
        mob_log(ch, "mteleport target is an invalid room");
        return;
    }

    if (boost::iequals(arg1, "all"))
    {
        if (target == ch->location)
        {
            mob_log(ch, "mteleport all target is itself");
            return;
        }

        auto people = ch->location.getPeople();
        for (auto vict : volcano::util::filter_raw(people))
        {

            if (valid_dg_target(vict, DG_ALLOW_GODS))
            {
                vict->leaveLocation();
                vict->moveToLocation(target);
                enter_wtrigger(ch->getRoom(), ch, -1);
            }
        }
    }
    else
    {
        if (*arg1 == UID_CHAR)
        {
            if (!(vict = get_char(arg1)))
            {
                mob_log(ch, "mteleport: victim (%s) does not exist", arg1);
                return;
            }
        }
        else if (!(vict = get_char_vis(ch, arg1, nullptr, FIND_CHAR_WORLD)))
        {
            mob_log(ch, "mteleport: victim (%s) does not exist", arg1);
            return;
        }

        if (valid_dg_target(ch, DG_ALLOW_GODS))
        {
            vict->leaveLocation();
            vict->moveToLocation(target);
            enter_wtrigger(ch->getRoom(), ch, -1);
        }
    }
}

ACMD(do_mdamage)
{
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
    int dam = 0;
    Character *vict;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    two_arguments(argument, name, amount);

    /* who cares if it's a number ? if not it'll just be 0 */
    if (!*name || !*amount)
    {
        mob_log(ch, "mdamage: bad syntax");
        return;
    }

    dam = atoi(amount);
    if (*name == UID_CHAR)
    {
        if (!(vict = get_char(name)))
        {
            mob_log(ch, "mdamage: victim (%s) does not exist", name);
            return;
        }
    }
    else if (!(vict = get_char_room_vis(ch, name, nullptr)))
    {
        mob_log(ch, "mdamage: victim (%s) does not exist", name);
        return;
    }
    script_damage(vict, dam);
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce)
{
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    argument = one_argument(argument, arg);

    if (!*arg || !*argument)
    {
        mob_log(ch, "mforce: bad syntax");
        return;
    }

    if (boost::iequals(arg, "all"))
    {
        struct descriptor_data *i;
        Character *vch;

        for (i = descriptor_list; i; i = i->next)
        {
            if ((i->character != ch) && !i->connected &&
                (i->character->location == ch->location))
            {
                vch = i->character;
                if (GET_LEVEL(vch) < GET_LEVEL(ch) && ch->canSee(vch) &&
                    valid_dg_target(vch, 0))
                {
                    command_interpreter(vch, argument);
                }
            }
        }
    }
    else
    {
        Character *victim;

        if (*arg == UID_CHAR)
        {
            if (!(victim = get_char(arg)))
            {
                mob_log(ch, "mforce: victim (%s) does not exist", arg);
                return;
            }
        }
        else if ((victim = get_char_room_vis(ch, arg, nullptr)) == nullptr)
        {
            mob_log(ch, "mforce: no such victim");
            return;
        }

        if (victim == ch)
        {
            mob_log(ch, "mforce: forcing self");
            return;
        }

        if (valid_dg_target(victim, 0))
            command_interpreter(victim, argument);
    }
}

/* place someone into the mob's memory list */
ACMD(do_mremember)
{
    Character *victim;
    struct script_memory *mem;
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    argument = one_argument(argument, arg);

    if (!*arg)
    {
        mob_log(ch, "mremember: bad syntax");
        return;
    }

    if (*arg == UID_CHAR)
    {
        if (!(victim = get_char(arg)))
        {
            mob_log(ch, "mremember: victim (%s) does not exist", arg);
            return;
        }
    }
    else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        mob_log(ch, "mremember: victim (%s) does not exist", arg);
        return;
    }

    /* create a structure and add it to the list */
    CREATE(mem, struct script_memory, 1);
    if (!SCRIPT_MEM(ch))
        SCRIPT_MEM(ch) = mem;
    else
    {
        struct script_memory *tmpmem = SCRIPT_MEM(ch);
        while (tmpmem->next)
            tmpmem = tmpmem->next;
        tmpmem->next = mem;
    }

    /* fill in the structure */
    mem->id = ((victim)->id);
    if (argument && *argument)
    {
        mem->cmd = strdup(argument);
    }
}

/* remove someone from the list */
ACMD(do_mforget)
{
    Character *victim;
    struct script_memory *mem, *prev;
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
        return;

    one_argument(argument, arg);

    if (!*arg)
    {
        mob_log(ch, "mforget: bad syntax");
        return;
    }

    if (*arg == UID_CHAR)
    {
        if (!(victim = get_char(arg)))
        {
            mob_log(ch, "mforget: victim (%s) does not exist", arg);
            return;
        }
    }
    else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        mob_log(ch, "mforget: victim (%s) does not exist", arg);
        return;
    }

    mem = SCRIPT_MEM(ch);
    prev = nullptr;
    while (mem)
    {
        if (mem->id == ((victim)->id))
        {
            if (mem->cmd)
                free(mem->cmd);
            if (prev == nullptr)
            {
                SCRIPT_MEM(ch) = mem->next;
                free(mem);
                mem = SCRIPT_MEM(ch);
            }
            else
            {
                prev->next = mem->next;
                free(mem);
                mem = prev->next;
            }
        }
        else
        {
            prev = mem;
            mem = mem->next;
        }
    }
}

/* transform into a different mobile */
ACMD(do_mtransform)
{
    char arg[MAX_INPUT_LENGTH];
    Character *m, tmpmob;
    Object *obj[NUM_WEARS];
    mob_rnum this_rnum = GET_MOB_RNUM(ch);
    int pos;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    if (ch->desc)
    {
        ch->sendText("You've got no VNUM to return to, dummy! try 'switch'\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg)
    {
        mob_log(ch, "mtransform: missing argument");
        return;
    }
    if (!isdigit(*arg) && *arg != '-')
    {
        mob_log(ch, "mtransform: bad argument");
        return;
    }

    if (isdigit(*arg))
        m = read_mobile(atoi(arg), VIRTUAL);
    else
    {
        m = read_mobile(atoi(arg + 1), VIRTUAL);
    }
    if (!m)
    {
        mob_log(ch, "mtransform: bad mobile vnum");
        return;
    }

    // deactivate ch to deal with vnum indexes and such.
    ch->deactivate();

    // We're changing the process to copy from m onto ch...

    /* Thanks to Russell Ryan for this fix. RRfon we need to copy the
    the strings so we don't end up free'ing the prototypes later */
    ch->name = m->name;
    ch->short_description = m->short_description;
    ch->look_description = m->look_description;
    ch->room_description = m->room_description;
    
    ch->appearances = m->appearances;
    ch->stats = m->stats;
    ch->mob_flags = m->mob_flags;
    ch->bodyparts = m->bodyparts;
    ch->sensei = m->sensei;
    ch->race = m->race;
    ch->affect_flags = m->affect_flags;
    // ch->extra_descriptions = m->extra_descriptions;

    extract_char(m);

    // Reactivate the character to deal with the changed vnum.
    ch->activate();
}

ACMD(do_maddtransform)
{
    char name[MAX_INPUT_LENGTH], operation[MAX_INPUT_LENGTH], formName[MAX_INPUT_LENGTH];
    int dam = 0;
    Character *vict;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    three_arguments(argument, name, operation, formName);

    /* who cares if it's a number ? if not it'll just be 0 */
    if (!*name || !*operation || !*formName)
    {
        mob_log(ch, "maddtransform: bad syntax (<name> <add/remove> <TFabbr>)");
        return;
    }

    if (!(vict = get_char(name)))
    {
        mob_log(ch, "maddtransform: victim (%s) does not exist", name);
        return;
    }

    bool add = true;
    if (boost::iequals(operation, "add"))
    {
        add = true;
    }
    else if (boost::iequals(operation, "remove"))
    {
        add = false;
    }
    else
    {
        mob_log(ch, "maddtransform: please specify whether to remove or add.", name);
    }

    std::string strForm(formName);

    auto foundForm = trans::findForm(ch, strForm);

    if (foundForm.has_value()) {
        if (add) {ch->addTransform(*foundForm);}
        else {ch->removeTransform(*foundForm);}
    }
}

ACMD(do_mdoor)
{
    char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
    char field[MAX_INPUT_LENGTH], *value;
    Room *rm;
    int dir, fd, to_room;

    const char *door_field[] = {
        "purge",
        "description",
        "flags",
        "key",
        "name",
        "room",
        "\n"};

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    argument = two_arguments(argument, target, direction);
    value = one_argument(argument, field);
    skip_spaces(&value);

    if (!*target || !*direction || !*field)
    {
        mob_log(ch, "mdoor called with too few args");
        return;
    }

    if ((rm = get_room(target)) == nullptr)
    {
        mob_log(ch, "mdoor: invalid target");
        return;
    }

    if ((dir = search_block(direction, dirs, false)) == -1)
    {
        mob_log(ch, "mdoor: invalid direction");
        return;
    }

    if ((fd = search_block(field, door_field, false)) == -1)
    {
        mob_log(ch, "odoor: invalid field");
        return;
    }

    auto cdir = static_cast<Direction>(dir);
    auto newexit = rm->getDirection(cdir);

    /* purge exit */
    if (fd == 0)
    {
        if (newexit)
        {

            rm->deleteExit(cdir);
        }
    }
    else
    {
        if (!newexit)
        {
            newexit.emplace();
            newexit->dir = cdir;
        }

        switch (fd)
        {
        case 1: /* description */
            newexit->general_description = std::string(value) + "\r\n";
            break;
        case 2: /* flags       */
            newexit->legacyExitFlags(asciiflag_conv(value));
            break;
        case 3: /* key         */
            newexit->key = atoi(value);
            break;
        case 4: /* name        */
            newexit->keyword = value;
            break;
        case 5: /* room        */
            if (auto loc = Location(value)) {
                *newexit = loc;
            }
            else
                mob_log(ch, "mdoor: invalid door target");
            break;
        }
    }
}

ACMD(do_mfollow)
{
    char buf[MAX_INPUT_LENGTH];
    Character *leader;
    struct follow_type *j, *k;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
        return;

    one_argument(argument, buf);

    if (!*buf)
    {
        mob_log(ch, "mfollow: bad syntax");
        return;
    }

    if (*buf == UID_CHAR)
    {
        if (!(leader = get_char(buf)))
        {
            mob_log(ch, "mfollow: victim (%s) does not exist", buf);
            return;
        }
    }
    else if (!(leader = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM)))
    {
        mob_log(ch, "mfollow: victim (%s) not found", buf);
        return;
    }

    if (ch->master == leader) /* already following */
        return;

    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) /* can't override charm */
        return;

    /* stop following someone else first */
    if (ch->master)
    {
        ch->master->followers.remove(ch->shared_from_this());
        ch->master = nullptr;
    }

    if (ch == leader)
        return;

    if (circle_follow(ch, leader))
    {
        mob_log(ch, "mfollow: Following in circles.");
        return;
    }

    ch->master = leader;

    leader->followers.add(ch->shared_from_this());
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
ACMD(do_mrecho)
{
    char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

    if (!MOB_OR_IMPL(ch))
    {
        ch->sendText("Huh?!?\r\n");
        return;
    }
    msg = two_arguments(argument, start, finish);

    skip_spaces(&msg);

    if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
        mob_log(ch, "mrecho called with too few args");
    else
        send_to_range(atoi(start), atoi(finish), "%s\r\n", msg);
}
