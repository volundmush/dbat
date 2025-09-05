/**************************************************************************
 *  File: dg_variables.c                                                   *
 *  Usage: contains the functions dealing with variable substitution.      *
 *                                                                         *
 *                                                                         *
 *  $Author: Mark A. Heilpern/egreen/Welcor $                              *
 *  $Date: 2004/10/11 12:07:00 $                                           *
 *  $Revision: 1.0.14 $                                                    *
 **************************************************************************/
#include "dbat/CharacterUtils.h"
#include "dbat/ObjectUtils.h"
#include "dbat/RoomUtils.h"
#include "dbat/DgScript.h"
#include "dbat/DgScriptPrototype.h"
#include "dbat/ObjectPrototype.h"
#include "dbat/CharacterPrototype.h"
#include "dbat/Location.h"
#include "dbat/Zone.h"
#include "dbat/dg_scripts.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"
#include "dbat/db.h"
#include "dbat/screen.h"
#include "dbat/constants.h"
#include "dbat/spells.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/TimeInfo.h"
#include "dbat/UID.h"
#include "dbat/filter.h"
#include "dbat/Random.h"
#include "dbat/utils.h"
#include "dbat/weather.h"

#include "dbat/const/Environment.h"

/* Utility functions */

/*
 * Thanks to James Long for his assistance in plugging the memory leak
 * that used to be here.   -- Welcor
 */

/* perhaps not the best place for this, but I didn't want a new file */
char *skill_percent(Character *ch, char *skill)
{
    static char retval[16];
    int skillnum;

    skillnum = find_skill_num(skill, SKTYPE_SKILL);
    if (skillnum <= 0)
        return ("unknown skill");

    snprintf(retval, sizeof(retval), "%d", GET_SKILL(ch, skillnum));
    return retval;
}

/*
   search through all the persons items, including containers
   and 0 if it doesnt exist, and greater then 0 if it does!
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000

   Now also searches by vnum -- Welcor
   Now returns the number of matching objects -- Welcor 02/04
*/

int item_in_list(char *item, const std::vector<std::weak_ptr<Object>> &list)
{
    int count = 0;

    if (list.empty())
        return 0;

    if (*item == UID_CHAR)
    {
        auto uidResult = resolveUID(item);
        if (!uidResult)
            return 0;
        auto obj = std::dynamic_pointer_cast<Object>(uidResult).get();
        if (!obj)
            return 0;

        for (auto i : filter_raw(list))
        {
            if (i == obj)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->getInventory());
        }
    }
    else if (is_number(item) > -1)
    { /* check for vnum */
        obj_vnum ovnum = atof(item);

        for (auto i : filter_raw(list))
        {
            if (GET_OBJ_VNUM(i) == ovnum)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->getInventory());
        }
    }
    else
    {
        for (auto i : filter_raw(list))
        {
            if (isname(item, i->getName()))
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->getInventory());
        }
    }
    return count;
}

/*
   BOOLEAN return, just check if a player or mob
   has an item of any sort, searched for by name
   or id.
   searching equipment as well as inventory,
   and containers.
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000
*/

int char_has_item(char *item, Character *ch)
{

    /* If this works, no more searching needed */
    if (get_object_in_equip(ch, item))
        return 1;

    if (item_in_list(item, ch->getInventory()) == 0)
        return 0;
    else
        return 1;
}

int text_processed(char *field, char *subfield, char *value,
                   char *str, size_t slen)
{
    char *p, *p2;
    char tmpvar[MAX_STRING_LENGTH];

    if (boost::iequals(field, "strlen"))
    { /* strlen    */
        char limit[200];
        sprintf(limit, "%" SZT, strlen(value));
        snprintf(str, slen, "%d", atoi(limit));
        return true;
    }
    else if (boost::iequals(field, "trim"))
    { /* trim      */
        /* trim whitespace from ends */
        snprintf(tmpvar, sizeof(tmpvar) - 1, "%s", value); /* -1 to use later*/
        p = tmpvar;
        p2 = tmpvar + strlen(tmpvar) - 1;
        while (*p && isspace(*p))
            p++;
        while ((p <= p2) && isspace(*p2))
            p2--;
        if (p > p2)
        { /* nothing left */
            *str = '\0';
            return true;
        }
        *(++p2) = '\0'; /* +1 ok (see above) */
        snprintf(str, slen, "%s", p);
        return true;
    }
    else if (boost::iequals(field, "contains"))
    { /* contains  */
        if (str_str(value, subfield))
            strcpy(str, "1");
        else
            strcpy(str, "0");
        return true;
    }
    else if (boost::iequals(field, "car"))
    { /* car       */
        char *car = value;
        while (*car && !isspace(*car))
            *str++ = *car++;
        *str = '\0';
        return true;
    }
    else if (boost::iequals(field, "cdr"))
    { /* cdr       */
        char *cdr = value;
        while (*cdr && !isspace(*cdr))
            cdr++; /* skip 1st field */
        while (*cdr && isspace(*cdr))
            cdr++; /* skip to next */

        snprintf(str, slen, "%s", cdr);
        return true;
    }
    else if (boost::iequals(field, "charat"))
    { /* CharAt    */
        size_t len = strlen(value), dgindex = atoi(subfield);
        if (dgindex > len || dgindex < 1)
            strcpy(str, "");
        else
            snprintf(str, slen, "%c", value[dgindex - 1]);
        return true;
    }
    else if (boost::iequals(field, "mudcommand"))
    {
        /* find the mud command returned from this text */
        /* NOTE: you may need to replace "cmd_info" with "complete_cmd_info", */
        /* depending on what patches you've got applied.                      */

        /* on older source bases:    extern struct command_info *cmd_info; */
        int length, cmd;
        for (length = strlen(value), cmd = 0;
             *cmd_info[cmd].command != '\n'; cmd++)
            if (!strncmp(cmd_info[cmd].command, value, length))
                break;

        if (*cmd_info[cmd].command == '\n')
            *str = '\0';
        else
            snprintf(str, slen, "%s", cmd_info[cmd].command);
        return true;
    }

    return false;
}

static char *send_cmd[] = {"msend ", "osend ", "wsend "};
static char *echo_cmd[] = {"mecho ", "oecho ", "wecho "};
static char *echoaround_cmd[] = {"mechoaround ", "oechoaround ", "wechoaround "};
static char *door[] = {"mdoor ", "odoor ", "wdoor "};
static char *force[] = {"mforce ", "oforce ", "wforce "};
static char *load[] = {"mload ", "oload ", "wload "};
static char *purge[] = {"mpurge ", "opurge ", "wpurge "};
static char *teleport[] = {"mteleport ", "oteleport ", "wteleport "};
/* the x kills a 'shadow' warning in gcc. */
static char *xdamage[] = {"mdamage ", "odamage ", "wdamage "};
static char *zoneecho[] = {"mzoneecho ", "ozoneecho ", "wzoneecho "};
static char *asound[] = {"masound ", "oasound ", "wasound "};
static char *at[] = {"mat ", "oat ", "wat "};
/* there is no such thing as wtransform, thus the wecho below  */
static char *transform[] = {"mtransform ", "otransform ", "wecho "};
static char *recho[] = {"mrecho ", "orecho ", "wrecho "};

/* sets str to be the value of var.field */
void find_replacement(HasDgScripts *go, HasDgScripts *sc, DgScript *trig, UnitType type, char *var, char *field, char *subfield,
                      char *str, size_t slen)
{

    Character *ch, *c = nullptr, *rndm;
    Object *obj, *o = nullptr;
    Room *room, *r = nullptr;
    char *name;
    int num, count, i, j, doors;

    auto unit = (HasDgScripts *)go;

    *str = '\0';

    std::optional<std::string> found;

    /* X.global() will have a nullptr trig */
    if (trig)
    {
        found = trig->getVariable(var);
    }

    /* some evil waitstates could crash the mud if sent here with sc==nullptr*/
    if (!found && sc)
    {
        found = sc->getVariable(var);
    }

    if (!*field)
    {
        if (found)
            snprintf(str, slen, "%s", found->c_str());
        else
        {
            auto col = static_cast<int>(type);
            if (boost::iequals(var, "self"))
            {
                auto uid = unit->getUID(true);
                snprintf(str, slen, "%s", uid.c_str());
            }
            else if (boost::iequals(var, "global"))
            {
                /* so "remote varname %global%" will work */
                snprintf(str, slen, "%s", get_room(0)->getUID(true).c_str());
                return;
            }
            else if (boost::iequals(var, "ctime"))
                snprintf(str, slen, "%ld", time(nullptr));
            else if (boost::iequals(var, "door"))
                snprintf(str, slen, "%s", door[col]);
            else if (boost::iequals(var, "force"))
                snprintf(str, slen, "%s", force[col]);
            else if (boost::iequals(var, "load"))
                snprintf(str, slen, "%s", load[col]);
            else if (boost::iequals(var, "purge"))
                snprintf(str, slen, "%s", purge[col]);
            else if (boost::iequals(var, "teleport"))
                snprintf(str, slen, "%s", teleport[col]);
            else if (boost::iequals(var, "damage"))
                snprintf(str, slen, "%s", xdamage[col]);
            else if (boost::iequals(var, "send"))
                snprintf(str, slen, "%s", send_cmd[col]);
            else if (boost::iequals(var, "echo"))
                snprintf(str, slen, "%s", echo_cmd[col]);
            else if (boost::iequals(var, "echoaround"))
                snprintf(str, slen, "%s", echoaround_cmd[col]);
            else if (boost::iequals(var, "zoneecho"))
                snprintf(str, slen, "%s", zoneecho[col]);
            else if (boost::iequals(var, "asound"))
                snprintf(str, slen, "%s", asound[col]);
            else if (boost::iequals(var, "at"))
                snprintf(str, slen, "%s", at[col]);
            else if (boost::iequals(var, "transform"))
                snprintf(str, slen, "%s", transform[col]);
            else if (boost::iequals(var, "recho"))
                snprintf(str, slen, "%s", recho[col]);
            else
                *str = '\0';
        }

        return;
    }
    else
    {
        if (found)
        {

            name = (char *)found->c_str();

            switch (type)
            {
            case MOB_TRIGGER:
                ch = (Character *)go;

                if ((o = get_object_in_equip(ch, name)))
                    ;
                else if ((o = get_obj_in_list(name, ch->getInventory())))
                    ;
                else if (IN_ROOM(ch) != NOWHERE && (c = get_char_in_room(ch->getRoom(), name)))
                    ;
                else if ((o = get_obj_in_list(name, ch->location.getObjects())))
                    ;
                else if ((c = get_char(name)))
                    ;
                else if ((o = get_obj(name)))
                    ;
                else if ((r = get_room(name)))
                {
                }

                break;
            case OBJ_TRIGGER:
                obj = (Object *)go;

                if ((c = get_char_by_obj(obj, name)))
                    ;
                else if ((o = get_obj_by_obj(obj, name)))
                    ;
                else if ((r = get_room(name)))
                {
                }

                break;
            case WLD_TRIGGER:
                room = (Room *)go;

                if ((c = get_char_by_room(room, name)))
                    ;
                else if ((o = get_obj_by_room(room, name)))
                    ;
                else if ((r = get_room(name)))
                {
                }

                break;
            }
        }
        else
        {
            if (boost::iequals(var, "self"))
            {
                c = nullptr;
                r = nullptr;
                o = nullptr;
                switch (type)
                {
                case MOB_TRIGGER:
                    c = (Character *)go;
                    break; /* the room.  - Welcor        */
                case OBJ_TRIGGER:
                    o = (Object *)go;
                    break;
                case WLD_TRIGGER:
                    r = (Room *)go;
                    break;
                }
            }
            else if (boost::iequals(var, "global"))
            {
                HasDgScripts *thescript = SCRIPT(get_room(0));
                *str = '\0';
                if (!thescript)
                {
                    script_log("Attempt to find global var. Apparently the void has no script.");
                    return;
                }
                for (const auto &[key, val] : thescript->variables)
                    if (boost::iequals(key.c_str(), field))
                    {
                        found = val;
                        break;
                    }

                if (found)
                    snprintf(str, slen, "%s", found->c_str());

                return;
            }
            else if (boost::iequals(var, "people"))
            {
                snprintf(str, slen, "%d", ((num = atoi(field)) > 0) ? trgvar_in_room(num) : 0);
                return;
            }
            else if (boost::iequals(var, "time"))
            {
                if (boost::iequals(field, "hour"))
                    snprintf(str, slen, "%d", time_info.hours);
                else if (boost::iequals(field, "minute"))
                    snprintf(str, slen, "%d", time_info.minutes);
                else if (boost::iequals(field, "second"))
                    snprintf(str, slen, "%d", time_info.seconds);
                else if (boost::iequals(field, "day"))
                    snprintf(str, slen, "%d", time_info.day + 1);
                else if (boost::iequals(field, "month"))
                    snprintf(str, slen, "%d", time_info.month + 1);
                else if (boost::iequals(field, "year"))
                    snprintf(str, slen, "%ld", time_info.year);
                else
                    *str = '\0';
                return;
            }
            /*

                  %findobj.<room vnum X>(<object vnum/id/name>)%
                    - count number of objects in room X with this name/id/vnum
                  %findmob.<room vnum X>(<mob vnum Y>)%
                    - count number of mobs in room X with vnum Y

            for example you want to check how many PC's are in room with vnum 1204.
            as PC's have the vnum -1...
            you would type:
            in any script:
            %echo% players in room 1204: %findmob.1204(-1)%

            Or say you had a bank, and you want a script to check the number of
            bags
            of gold (vnum: 1234)
            in the vault (vnum: 453) now and then. you can just use
            %findobj.453(1234)% and it will return the number of bags of gold.

            **/

            /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
            else if (boost::iequals(var, "findmob"))
            {
                if (!field || !*field || !subfield || !*subfield)
                {
                    script_log("findmob.vnum(mvnum) - illegal syntax");
                    strcpy(str, "0");
                }
                else
                {
                    auto fi = atoi(field);
                    room_rnum rrnum = real_room(fi);
                    mob_vnum mvnum = atoi(subfield);

                    if (rrnum == NOWHERE)
                    {
                        script_log("findmob.vnum(ovnum): No room with vnum %d", fi);
                        strcpy(str, "0");
                    }
                    else
                    {
                        i = 0;
                        auto people = get_room(rrnum)->getPeople().snapshot_weak();
                        for (auto ch : filter_raw(people))
                            if (GET_MOB_VNUM(ch) == mvnum)
                                i++;

                        snprintf(str, slen, "%d", i);
                    }
                }
            }
            /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
            else if (boost::iequals(var, "findobj"))
            {
                if (!field || !*field || !subfield || !*subfield)
                {
                    script_log("findobj.vnum(ovnum) - illegal syntax");
                    strcpy(str, "0");
                }
                else
                {
                    auto fi = atoi(field);
                    room_rnum rrnum = real_room(fi);

                    if (rrnum == NOWHERE)
                    {
                        script_log("findobj.vnum(ovnum): No room with vnum %d", fi);
                        strcpy(str, "0");
                    }
                    else
                    {
                        /* item_in_list looks within containers as well. */
                        snprintf(str, slen, "%d", item_in_list(subfield, Location(rrnum).getObjects()));
                    }
                }
            }
            else if (boost::iequals(var, "random"))
            {
                if (boost::iequals(field, "char"))
                {
                    rndm = nullptr;
                    count = 0;

                    if (type == MOB_TRIGGER)
                    {
                        ch = (Character *)go;
                        auto people = ch->location.getPeople();
                        for (auto c : filter_raw(people))
                            if ((c != ch) && valid_dg_target(c, DG_ALLOW_GODS) &&
                                ch->canSee(c))
                            {
                                if (!Random::get<int>(0, count))
                                    rndm = c;
                                count++;
                            }
                    }
                    else if (type == OBJ_TRIGGER)
                    {
                        auto people = get_room(obj_room((Object *)go))->getPeople().snapshot_weak();
                        for (auto c : filter_raw(people))
                            if (valid_dg_target(c, DG_ALLOW_GODS))
                            {
                                if (!Random::get<int>(0, count))
                                    rndm = c;
                                count++;
                            }
                    }
                    else if (type == WLD_TRIGGER)
                    {
                        auto people = ((Room *)go)->getPeople().snapshot_weak();
                        for (auto c : filter_raw(people))
                            if (valid_dg_target(c, DG_ALLOW_GODS))
                            {

                                if (!Random::get<int>(0, count))
                                    rndm = c;
                                count++;
                            }
                    }

                    if (rndm)
                        snprintf(str, slen, "%s", ((rndm)->getUID(true).c_str()));
                    else
                        *str = '\0';
                }
                else if (boost::iequals(field, "dir"))
                {
                    room_rnum in_room = NOWHERE;

                    switch (type)
                    {
                    case WLD_TRIGGER:
                        in_room = real_room(((Room *)go)->getVnum());
                        break;
                    case OBJ_TRIGGER:
                        in_room = obj_room((Object *)go);
                        break;
                    case MOB_TRIGGER:
                        in_room = IN_ROOM(((Character *)go));
                        break;
                    }
                    if (in_room == NOWHERE)
                    {
                        *str = '\0';
                    }
                    else
                    {
                        std::vector<int> available;
                        room = get_room(in_room);
                        for (auto &[d, e] : room->getDirections())
                            available.push_back(static_cast<int>(d));

                        if (available.empty())
                        {
                            *str = '\0';
                        }
                        else
                        {
                            auto dir = Random::get(available);
                            snprintf(str, slen, "%s", dirs[*dir]);
                        }
                    }
                }
                else
                    snprintf(str, slen, "%d", ((num = atoi(field)) > 0) ? Random::get<int>(1, num) : 0);

                return;
            }
        }

        char *value = nullptr;
        if (found)
            value = (char *)found->c_str();
        if (text_processed(field, subfield, value, str, slen))
            return;

        if (c)
        {

            if (boost::iequals(field, "global"))
            { /* get global of something else */
                if (IS_NPC(c))
                {
                    find_replacement(go, c, nullptr, MOB_TRIGGER,
                                     subfield, nullptr, nullptr, str, slen);
                }
            }
            /* set str to some 'non-text' first */
            *str = '\x1';

            if (auto result = c->dgCallMember(field, subfield ? subfield : ""); result)
            {
                snprintf(str, slen, "%s", result.value().c_str());
                return;
            }

            switch (tolower(*field))
            {
            case 'a':
                if (boost::iequals(field, "aaaaa"))
                {
                    strcpy(str, "0");
                }
                else if (boost::iequals(field, "affect"))
                {
                    if (subfield && *subfield)
                    {
                        int affect = get_flag_by_name(affected_bits, subfield);
                        if (affect != NOFLAG && AFF_FLAGGED(c, affect))
                            strcpy(str, "1");
                        else
                            strcpy(str, "0");
                    }
                    else
                        strcpy(str, "0");
                }
                else if (boost::iequals(field, "alias"))
                    snprintf(str, slen, "%s", GET_PC_NAME(c));

                else if (boost::iequals(field, "align"))
                {
                    if (subfield && *subfield)
                    {
                        int addition = atof(subfield);
                        c->setBaseStat("good_evil", std::clamp<int>(addition, -1000, 1000));
                    }
                    snprintf(str, slen, "%d", GET_ALIGNMENT(c));
                }
                break;
            case 'c':
                if (boost::iequals(field, "canbeseen"))
                {
                    if ((type == MOB_TRIGGER) && !((Character *)go)->canSee(c))
                        strcpy(str, "0");
                    else
                        strcpy(str, "1");
                }
                else if (boost::iequals(field, "carry"))
                {
                    if (!IS_NPC(c) && CARRYING(c))
                        strcpy(str, "1");
                    else
                        strcpy(str, "0");
                }
                else if (boost::iequals(field, "clan"))
                {
                    strcpy(str, "0");
                }
                else if (boost::iequals(field, "class"))
                {
                    if (!IS_NPC(c))
                        snprintf(str, slen, "%s", sensei::getName(c->sensei).c_str());
                    else
                        snprintf(str, slen, "blank");
                }
                break;
            case 'd':
                if (boost::iequals(field, "death"))
                {
                    snprintf(str, slen, "%ld", GET_DTIME(c));
                }
                else if (boost::iequals(field, "drag"))
                {
                    if (!IS_NPC(c) && DRAGGING(c))
                        strcpy(str, "1");
                    else
                        strcpy(str, "0");
                }
                break;
            case 'e':
                if (boost::iequals(field, "eq"))
                {
                    int pos;
                    if (!subfield || !*subfield)
                        *str = '\0';
                    else if (*subfield == '*')
                    {
                        for (i = 0, j = 0; i < NUM_WEARS; i++)
                            if (GET_EQ(c, i))
                            {
                                j++;
                                break;
                            }
                        if (j > 0)
                            strcpy(str, "1");
                        else
                            *str = '\0';
                    }
                    else if ((pos = find_eq_pos_script(subfield)) < 0 || !GET_EQ(c, pos))
                        *str = '\0';
                    else
                        snprintf(str, slen, "%s", ((((c)->getEquipSlot(pos)))->getUID(true).c_str()));
                }
                if (boost::iequals(field, "exp"))
                {
                    if (subfield && *subfield)
                    {
                        int64_t addition = std::max<int64_t>(0, atof(subfield));

                        c->modExperience(addition);
                    }
                    snprintf(str, slen, "%" I64T "", GET_EXP(c));
                }
                break;
            case 'f':
                if (boost::iequals(field, "fighting"))
                {
                    if (FIGHTING(c))
                        snprintf(str, slen, "%s", ((((c)->fighting))->getUID(true).c_str()));
                    else
                        *str = '\0';
                }
                else if (boost::iequals(field, "follower"))
                {
                    if (!c->followers)
                        *str = '\0';
                    else
                        snprintf(str, slen, "%s", ((c->followers.head())->getUID(true).c_str()));
                }
                break;
            case 'h':
                if (boost::iequals(field, "has_item"))
                {
                    if (!(subfield && *subfield))
                        *str = '\0';
                    else
                        snprintf(str, slen, "%d", char_has_item(subfield, c));
                }
                else if (boost::iequals(field, "hisher"))
                    snprintf(str, slen, "%s", HSHR(c));

                else if (boost::iequals(field, "heshe"))
                    snprintf(str, slen, "%s", HSSH(c));

                else if (boost::iequals(field, "himher"))
                    snprintf(str, slen, "%s", HMHR(c));

                else if (boost::iequals(field, "hitp"))
                {
                    if (subfield && *subfield)
                    {
                        int64_t addition = atof(subfield);
                        if (addition > 0)
                        {
                            c->modCurVital(CharVital::health, addition);
                        }
                        else
                        {
                            c->modCurVital(CharVital::health, -addition);
                        }

                        update_pos(c);
                    }
                    snprintf(str, slen, "%" I64T "", GET_HIT(c));
                }
                break;
            case 'i':
                if (boost::iequals(field, "id"))
                    snprintf(str, slen, "%s", c->getUID(true).c_str());

                /* new check for pc/npc status */
                else if (boost::iequals(field, "is_pc"))
                {
                    strcpy(str, !IS_NPC(c) ? "1" : "0");
                }
                else if (boost::iequals(field, "inventory"))
                {
                    if (subfield && *subfield)
                    {
                        auto con = c->getInventory();
                        auto oid = atof(subfield);
                        for (auto obj : filter_raw(con))
                        {
                            if (GET_OBJ_VNUM(obj) == oid)
                            {
                                snprintf(str, slen, "%s", ((obj)->getUID(true).c_str())); /* arg given, found */
                                return;
                            }
                        }
                    }
                    else
                    { /* no arg given */
                        if (auto con = c->getInventory(); !con.empty())
                        {
                            for (auto o : filter_raw(con))
                            {
                                snprintf(str, slen, "%s", o->getUID(true).c_str());
                                return;
                            }
                        }
                    }
                    *str = '\0'; /* arg given, not found */
                    return;
                }
                break;
            case 'l':
                if (boost::iequals(field, "level"))
                    snprintf(str, slen, "%d", GET_LEVEL(c));
                break;
            case 'm':
                if (boost::iequals(field, "maxhitp"))
                {
                    if (subfield && *subfield)
                    {
                        int64_t addition = atof(subfield);
                        // GET_MAX_HIT(c) = MAX(GET_MAX_HIT(c) + addition, 1);
                    }
                    snprintf(str, slen, "%" I64T "", GET_MAX_HIT(c));
                }
                else if (boost::iequals(field, "mana"))
                {
                    if (subfield && *subfield)
                    {
                        int64_t addition = atof(subfield);
                        if (addition > 0)
                        {
                            c->modCurVital(CharVital::ki, addition);
                        }
                        else
                        {
                            c->modCurVital(CharVital::ki, -addition);
                        }
                    }
                    snprintf(str, slen, "%" I64T "", (c->getCurVital(CharVital::ki)));
                }
                else if (boost::iequals(field, "maxmana"))
                {
                    if (subfield && *subfield)
                    {
                        int64_t addition = atof(subfield);
                        // GET_MAX_MANA(c) = MAX(GET_MAX_MANA(c) + addition, 1);
                    }
                    snprintf(str, slen, "%" I64T "", GET_MAX_MANA(c));
                }
                else if (boost::iequals(field, "move"))
                {
                    if (subfield && *subfield)
                    {
                        int64_t addition = atof(subfield);
                        if (addition > 0)
                        {
                            c->modCurVital(CharVital::stamina, addition);
                        }
                        else
                        {
                            c->modCurVital(CharVital::stamina, -addition);
                        }
                    }
                    snprintf(str, slen, "%" I64T "", (c->getCurVital(CharVital::stamina)));
                }
                else if (boost::iequals(field, "maxmove"))
                {
                    if (subfield && *subfield)
                    {
                        int64_t addition = atof(subfield);
                        // GET_MAX_MOVE(c) = MAX(GET_MAX_MOVE(c) + addition, 1);
                    }
                    snprintf(str, slen, "%" I64T "", GET_MAX_MOVE(c));
                }
                else if (boost::iequals(field, "master"))
                {
                    if (!c->master)
                        *str = '\0';
                    else
                        snprintf(str, slen, "%s", (c->master->getUID(true).c_str()));
                }
                break;
            case 'n':
                if (boost::iequals(field, "name"))
                {
                    snprintf(str, slen, "%s", GET_NAME(c));
                }
                else if (boost::iequals(field, "next_in_room"))
                {
                    if (auto people = c->location.getPeople(); !people.empty())
                    {
                        auto found = false;
                        for (auto p : filter_raw(people))
                        {
                            if (found)
                            {
                                snprintf(str, slen, "%s", p->getUID(true).c_str());
                                return;
                            }
                            if (p == c)
                            {
                                found = true;
                            }
                        }
                    }
                    *str = '\0';
                    return;
                }
                break;
            case 'p':
                /* Thanks to Christian Ejlertsen for this idea
                   And to Ken Ray for speeding the implementation up :)*/
                if (boost::iequals(field, "pos"))
                {
                    if (subfield && *subfield)
                    {
                        for (i = POS_SLEEPING; i <= POS_STANDING; i++)
                        {
                            /* allows : Sleeping, Resting, Sitting, Fighting, Standing */
                            if (!strncasecmp(subfield, position_types[i], strlen(subfield)))
                            {
                                c->setBaseStat("combo", i);
                                break;
                            }
                        }
                    }
                    snprintf(str, slen, "%s", position_types[GET_POS(c)]);
                }
                else if (boost::iequals(field, "prac"))
                {
                    if (IS_NPC(c))
                    {
                        if (IN_ROOM(c) != NOWHERE)
                        {
                            c->location.sendText("Error!: Report this trigger error to the coding authorities!\r\n");
                        }
                    }
                    if (subfield && *subfield)
                    {
                        int addition = atof(subfield);
                        c->modPractices(addition);
                    }
                    snprintf(str, slen, "%d", GET_PRACTICES(c));
                }
                else if (boost::iequals(field, "plr"))
                {
                    if (subfield && *subfield)
                    {
                        int plr = get_flag_by_name(player_bits, subfield);
                        if (plr != NOFLAG && PLR_FLAGGED(c, plr))
                            strcpy(str, "1");
                        else
                            strcpy(str, "0");
                    }
                    else
                        strcpy(str, "0");
                }
                else if (boost::iequals(field, "pref"))
                {
                    if (subfield && *subfield)
                    {
                        int pref = get_flag_by_name(preference_bits, subfield);
                        if (pref != NOFLAG && PRF_FLAGGED(c, pref))
                            strcpy(str, "1");
                        else
                            strcpy(str, "0");
                    }
                    else
                        strcpy(str, "0");
                }
                break;
            case 'r':
                if (boost::iequals(field, "room"))
                { /* in NOWHERE, return the void */
/* see note in dg_scripts.h */
#ifdef ACTOR_ROOM_IS_UID
                    if (auto roomFound = c->getRoom(); roomFound)
                    {
                        snprintf(str, slen, "%s", roomFound->getUID(true).c_str());
                    }
#else
                    snprintf(str, slen, "%d", (IN_ROOM(c) != NOWHERE) ? c->getRoom()->number : 0);
#endif
                }
#ifdef GET_RACE
                else if (boost::iequals(field, "race"))
                {
                    snprintf(str, slen, "%s", race::getName(c->race).c_str());
                }
#endif
                else if (boost::iequals(field, "rpp"))
                {
                    if (subfield && *subfield)
                    {
                        int addition = atof(subfield);
                        c->modRPP(addition);
                    }

                    snprintf(str, slen, "%d", c->getRPP());
                }

                break;
            case 's':
                if (boost::iequals(field, "sex"))
                    snprintf(str, slen, "%s", genders[(int)GET_SEX(c)]);

                else if (boost::iequals(field, "size"))
                {
                    if (subfield && *subfield)
                    {
                        int ns;
                        if ((ns = search_block(subfield, size_names, false)) > -1)
                        {
                            (c)->setSize(ns);
                        }
                    }
                    sprinttype(get_size(c), size_names, str, slen);
                }
                else if (boost::iequals(field, "skill"))
                    snprintf(str, slen, "%s", skill_percent(c, subfield));

                else if (boost::iequals(field, "skillset"))
                {
                    if (!IS_NPC(c) && subfield && *subfield)
                    {
                        char skillname[MAX_INPUT_LENGTH], *amount;
                        amount = one_word(subfield, skillname);
                        skip_spaces(&amount);
                        if (amount && *amount && is_number(amount))
                        {
                            int skillnum = find_skill_num(skillname, SKTYPE_SKILL);
                            if (skillnum > 0)
                            {
                                int new_value = std::clamp<double>(atof(amount), 0, 100);
                                SET_SKILL(c, skillnum, new_value);
                            }
                        }
                    }
                    *str = '\0'; /* so the parser know we recognize 'skillset' as a field */
                }
                break;
            case 't':
                if (boost::iequals(field, "tnl"))
                {
                    snprintf(str, slen, "%ld", level_exp(c, GET_LEVEL(c) + 1));
                }
                break;
            case 'v':
                if (boost::iequals(field, "vnum"))
                {
                    if (subfield && *subfield)
                    {
                        snprintf(str, slen, "%d", IS_NPC(c) ? (int)(GET_MOB_VNUM(c) == atof(subfield)) : -1);
                    }
                    else
                    {
                        if (IS_NPC(c))
                            snprintf(str, slen, "%d", GET_MOB_VNUM(c));
                        else
                            /*
                             * for compatibility with unsigned indexes
                             * - this is deprecated - use %actor.is_pc% to check
                             * instead of %actor.vnum% == -1  --Welcor 09/03
                             */
                            strcpy(str, "-1");
                    }
                }
                else if (boost::iequals(field, "varexists"))
                {
                    snprintf(str, slen, "%d", c->variables.contains(subfield));
                }

                break;
            case 'w':
                if (boost::iequals(field, "weight"))
                    snprintf(str, slen, "%s", fmt::format("{}", c->getEffectiveStat("weight")).c_str());

                break;
            } /* switch *field */

            if (*str == '\x1')
            { /* no match found in switch */
                if (auto found = c->getVariable(field); found)
                {
                    snprintf(str, slen, "%s", found->c_str());
                }
                else
                {
                    *str = '\0';
                    script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                               GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
                }
            }

        } /* if (c) ...*/

        else if (o)
        {

            *str = '\x1';
            switch (tolower(*field))
            {
            case 'a':
                if (boost::iequals(field, "affects"))
                {
                    if (subfield && *subfield)
                    {
                        if (check_flags_by_name_ar(GET_OBJ_PERM(o).getAllAsSet(), NUM_AFF_FLAGS, subfield, affected_bits) > 0)
                            snprintf(str, slen, "1");
                        else
                            snprintf(str, slen, "0");
                    }
                    else
                        snprintf(str, slen, "0");
                }
                break;
            case 'c':
                if (boost::iequals(field, "cost"))
                {
                    if (subfield && *subfield)
                    {
                        int addition = atof(subfield);
                        o->setBaseStat<int>("cost", std::max<int>(0, addition + GET_OBJ_COST(o)));
                    }
                    snprintf(str, slen, "%d", GET_OBJ_COST(o));
                }
                else if (boost::iequals(field, "cost_per_day"))
                {
                    if (subfield && *subfield)
                    {
                        int addition = atof(subfield);
                        o->setBaseStat<int>("cost_per_day", std::max<int>(0, addition + GET_OBJ_RENT(o)));
                    }
                    snprintf(str, slen, "%d", GET_OBJ_RENT(o));
                }
                else if (boost::iequals(field, "carried_by"))
                {
                    if (auto carriedby = o->getCarriedBy(); carriedby)
                        snprintf(str, slen, "%s", carriedby->getUID(true).c_str());
                    else
                        *str = '\0';
                }
                else if (boost::iequals(field, "contents"))
                {
                    if (auto con = o->getInventory(); !con.empty())
                    {
                        for (auto obj : filter_raw(con))
                        {
                            snprintf(str, slen, "%s", obj->getUID(true).c_str());
                            return;
                        }
                    }
                    else
                        *str = '\0';
                }
                /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
                else if (boost::iequals(field, "count"))
                {
                    if (GET_OBJ_TYPE(o) == ITEM_CONTAINER)
                        snprintf(str, slen, "%d", item_in_list(subfield, o->getInventory()));
                    else
                        strcpy(str, "0");
                }
                break;
            case 'e':
                if (boost::iequals(field, "extra"))
                {
                    if (subfield && *subfield)
                    {
                        if (check_flags_by_name_ar(GET_OBJ_EXTRA(o).getAllAsSet(), NUM_ITEM_FLAGS, subfield, extra_bits) > 0)
                            snprintf(str, slen, "1");
                        else
                            snprintf(str, slen, "0");
                    }
                    else
                        snprintf(str, slen, "0");
                }
                else
                {
                    snprintf(str, slen, "%s", GET_OBJ_EXTRA(o).getFlagNames().c_str());
                }
                break;
            case 'h':
                /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
                if (boost::iequals(field, "has_in"))
                {
                    if (GET_OBJ_TYPE(o) == ITEM_CONTAINER)
                        snprintf(str, slen, "%s", (item_in_list(subfield, o->getInventory()) ? "1" : "0"));
                    else
                        strcpy(str, "0");
                }
                if (boost::iequals(field, "health"))
                {
                    if (subfield && *subfield)
                    {
                        int addition = atoi(subfield);
                        SET_OBJ_VAL(o, VAL_ALL_HEALTH, std::max<int>(1, addition + GET_OBJ_VAL(o, VAL_ALL_HEALTH)));
                        if (OBJ_FLAGGED(o, ITEM_BROKEN) && GET_OBJ_VAL(o, VAL_ALL_HEALTH) >= 100)
                            o->item_flags.set(ITEM_BROKEN, false);
                    }
                    snprintf(str, slen, "%ld", GET_OBJ_VAL(o, VAL_ALL_HEALTH));
                }
                break;
            case 'i':
                if (boost::iequals(field, "id"))
                    snprintf(str, slen, "%s", o->getUID(true).c_str());

                else if (boost::iequals(field, "is_inroom"))
                {
                    if (auto roomFound = o->getRoom(); roomFound)
                        snprintf(str, slen, "%s", roomFound->getUID(true).c_str());
                    else
                        *str = '\0';
                }
                else if (boost::iequals(field, "is_pc"))
                {
                    strcpy(str, "-1");
                }
                else if (boost::iequals(field, "itemflag"))
                {
                    if (subfield && *subfield)
                    {
                        int item = get_flag_by_name(extra_bits, subfield);
                        if (item != NOFLAG && OBJ_FLAGGED(o, item))
                            strcpy(str, "1");
                        else
                            strcpy(str, "0");
                    }
                    else
                        strcpy(str, "0");
                }
                break;
            case 'l':
                if (boost::iequals(field, "level"))
                    snprintf(str, slen, "%d", GET_OBJ_LEVEL(o));
                break;

            case 'n':
                if (boost::iequals(field, "name"))
                {
                    if (!subfield || !*subfield)
                        snprintf(str, slen, "%s", o->getName());
                    else
                    {
                        char blah[500];
                        sprintf(blah, "%s %s", o->getName(), subfield);
                        o->strings["name"] = blah;
                    }
                }
                else if (boost::iequals(field, "next_in_list"))
                {
                    if (auto con = o->location.getObjects(); !con.empty())
                    {
                        auto found = false;
                        for (auto ob : filter_raw(con))
                        {
                            if (ob == o)
                            {
                                found = true;
                                continue;
                            }
                            if (found)
                            {
                                snprintf(str, slen, "%s", ob->getUID(true).c_str());
                                return;
                            }
                        }
                    }

                    *str = '\0';
                    return;
                }
                break;
            case 'r':
                if (boost::iequals(field, "room"))
                {
                    if (auto roomFound = get_room(obj_room(o)); roomFound)
                        snprintf(str, slen, "%s", roomFound->getUID(true).c_str());
                    else
                        *str = '\0';
                }
                break;
            case 's':
                if (boost::iequals(field, "shortdesc"))
                {
                    if (!subfield || !*subfield)
                        snprintf(str, slen, "%s", o->getShortDescription());
                    else
                    {
                        char blah[500];
                        sprintf(blah, "%s @wnicknamed @D(@C%s@D)@n", o->getShortDescription(), subfield);
                        o->strings["short_description"] = blah;
                    }
                }
                else if (boost::iequals(field, "setaffects"))
                {
                    if (subfield && *subfield)
                    {
                        int ns;
                        if ((ns = check_flags_by_name_ar(GET_OBJ_PERM(o).getAllAsSet(), NUM_AFF_FLAGS, subfield, affected_bits)) >
                            0)
                        {
                            o->affect_flags.toggle(ns);
                            snprintf(str, slen, "1");
                        }
                    }
                }
                else if (boost::iequals(field, "setextra"))
                {
                    if (subfield && *subfield)
                    {
                        int ns;
                        if ((ns = check_flags_by_name_ar(GET_OBJ_EXTRA(o).getAllAsSet(), NUM_ITEM_FLAGS, subfield, extra_bits)) >
                            0)
                        {
                            o->item_flags.toggle(ns);
                            snprintf(str, slen, "1");
                        }
                    }
                }
                else if (boost::iequals(field, "size"))
                {
                    if (subfield && *subfield)
                    {
                        int ns;
                        if ((ns = search_block(subfield, size_names, false)) > -1)
                        {
                            (o)->size = static_cast<Size>(ns);
                        }
                    }
                    sprinttype(static_cast<int>(GET_OBJ_SIZE(o)), size_names, str, slen);
                }
                break;
            case 't':
                if (boost::iequals(field, "type"))
                    sprinttype(GET_OBJ_TYPE(o), item_types, str, slen);

                else if (boost::iequals(field, "timer"))
                    snprintf(str, slen, "%d", GET_OBJ_TIMER(o));
                break;
            case 'v':
                if (boost::iequals(field, "vnum"))
                    if (subfield && *subfield)
                    {
                        snprintf(str, slen, "%d", (int)(GET_OBJ_VNUM(o) == atof(subfield)));
                    }
                    else
                    {
                        snprintf(str, slen, "%d", GET_OBJ_VNUM(o));
                    }
                else if (boost::iequals(field, "value") && subfield && *subfield)
                    snprintf(str, slen, "%ld", GET_OBJ_VAL(o, subfield));
                break;
            case 'w':
                if (boost::iequals(field, "weight"))
                {
                    if (subfield && *subfield)
                    {
                        auto addition = atof(subfield);
                        if (addition < 0 || addition > 0)
                        {
                            o->setBaseStat<weight_t>("weight", std::max<double>(0, addition + GET_OBJ_WEIGHT(o)));
                        }
                        else
                        {
                            o->setBaseStat<weight_t>("weight", 0);
                        }
                    }
                    snprintf(str, slen, "%s", fmt::format("{}", GET_OBJ_WEIGHT(o)).c_str());
                }
                else if (boost::iequals(field, "worn_by"))
                {
                    if (auto worn = o->getWornBy())
                        snprintf(str, slen, "%s", worn->getUID(true).c_str());
                    else
                        *str = '\0';
                }
                break;
            } /* switch *field */

            if (*str == '\x1')
            { /* no match in switch */
                if (auto found = o->getVariable(field); found)
                {
                    snprintf(str, slen, "%s", found->c_str());
                }
                else
                {
                    *str = '\0';
                    if (!boost::iequals(GET_TRIG_NAME(trig), "Rename Object"))
                    {
                        script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
                                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), static_cast<int>(type), field);
                    }
                }
            }
        } /* if (o) ... */

        else if (r)
        {

            if (auto result = r->dgCallMember(field, subfield ? subfield : ""); result)
            {
                snprintf(str, slen, "%s", result->c_str());
                return;
            }

            /* special handling of the void, as it stores all 'full global' variables */
            if (r->getVnum() == 0)
            {
                if (auto found = r->getVariable(field); found)
                    snprintf(str, slen, "%s", found->c_str());
                else
                    *str = '\0';
            }
            else if (boost::iequals(field, "name"))
                snprintf(str, slen, "%s", r->getName());

            else if (boost::iequals(field, "sector"))
                sprinttype(static_cast<int>(r->sector_type), sector_types, str, slen);

            else if (boost::iequals(field, "gravity")) {
                Location loc(r);
                snprintf(str, slen, "%d", (int)loc.getEnvironment(ENV_GRAVITY));
            }

            else if (boost::iequals(field, "vnum"))
            {
                if (subfield && *subfield)
                {
                    snprintf(str, slen, "%d", (int)(r->getVnum() == atof(subfield)));
                }
                else
                {
                    snprintf(str, slen, "%d", r->getVnum());
                }
            }
            else if (boost::iequals(field, "contents"))
            {
                if (subfield && *subfield)
                {
                    auto con = r->getObjects().snapshot_weak();
                    for (auto obj : filter_raw(con))
                    {
                        if (GET_OBJ_VNUM(obj) == atof(subfield))
                        {
                            /* arg given, found */
                            snprintf(str, slen, "%s", ((obj)->getUID(true).c_str()));
                            return;
                        }
                    }
                    if (!obj)
                        *str = '\0'; /* arg given, not found */
                }
                else
                { /* no arg given */
                    if (auto con = r->getObjects().snapshot_weak(); !con.empty())
                    {
                        for (auto obj : filter_raw(con))
                        {
                            snprintf(str, slen, "%s", obj->getUID(true).c_str());
                            return;
                        }
                    }
                    else
                    {
                        *str = '\0';
                    }
                }
            }
            else if (boost::iequals(field, "people"))
            {
                if (auto people = r->getPeople().snapshot_weak(); !people.empty())
                {
                    for (auto p : filter_raw(people))
                    {
                        snprintf(str, slen, "%s", p->getUID(true).c_str());
                        return;
                    }
                }
                *str = '\0';
                return;
            }
            else if (boost::iequals(field, "id"))
            {
                if (r->getVnum() != NOWHERE)
                    snprintf(str, slen, "%s", r->getUID(true).c_str());
                else
                    *str = '\0';
            }
            else if (boost::iequals(field, "weather"))
            {
                const char *sky_look[] = {
                    "sunny",
                    "cloudy",
                    "rainy",
                    "lightning"};

                if (!r->room_flags.get(ROOM_INDOORS))
                    snprintf(str, slen, "%s", sky_look[weather_info.sky]);
                else
                    *str = '\0';
            }
            else if (boost::iequals(field, "fishing"))
            {
                if (ROOM_FLAGGED(r, ROOM_FISHING))
                    snprintf(str, slen, "1");
                else
                    snprintf(str, slen, "0");
            }
            else if (boost::iequals(field, "zonenumber"))
                snprintf(str, slen, "%d", r->zone->number);
            else if (boost::iequals(field, "zonename"))
                snprintf(str, slen, "%s", r->zone->name.c_str());
            else if (boost::iequals(field, "roomflag"))
            {
                if (subfield && *subfield)
                {
                    if (check_flags_by_name_ar(r->room_flags.getAllAsSet(), NUM_ROOM_FLAGS, subfield, room_bits) > 0)
                        snprintf(str, slen, "1");
                    else
                        snprintf(str, slen, "0");
                }
                else
                    snprintf(str, slen, "0");
            }
            else
            {
                if (auto found = r->getVariable(field); found)
                    snprintf(str, slen, "%s", found->c_str());
                else
                {
                    *str = '\0';
                    script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
                               GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), static_cast<int>(type), field);
                }
            }
        } /* if (r).. */
    }
}

/*
 * Now automatically checks if the variable has more then one field
 * in it. And if the field returns a name or a script UID or the like
 * it can recurse.
 * If you supply a value like, %actor.int.str% it wont blow up on you
 * either.
 * - Jamie Nelson 31st Oct 2003 01:03
 *
 * Now also lets subfields have variables parsed inside of them
 * so that:
 * %echo% %actor.gold(%actor.gold%)%
 * will double the actors gold every time its called.  etc...
 * - Jamie Nelson 31st Oct 2003 01:24
 */

/* substitutes any variables into line and returns it as buf */
void var_subst(HasDgScripts *go, HasDgScripts *sc, DgScript *trig,
               UnitType type, char *line, char *buf)
{
    char tmp[MAX_INPUT_LENGTH], repl_str[MAX_INPUT_LENGTH];
    char *var = nullptr, *field = nullptr, *p = nullptr;
    char tmp2[MAX_INPUT_LENGTH];
    char *subfield_p, subfield[MAX_INPUT_LENGTH];
    int left, len;
    int paren_count = 0;
    int dots = 0;

    /* skip out if no %'s */
    if (!strchr(line, '%'))
    {
        strcpy(buf, line);
        return;
    }
    /*lets just empty these to start with*/
    *repl_str = *tmp = *tmp2 = '\0';

    p = strcpy(tmp, line);
    subfield_p = subfield;

    left = MAX_INPUT_LENGTH - 1;

    while (*p && (left > 0))
    {

        /* copy until we find the first % */
        while (*p && (*p != '%') && (left > 0))
        {
            *(buf++) = *(p++);
            left--;
        }

        *buf = '\0';

        /* double % */
        if (*p && (*(++p) == '%') && (left > 0))
        {
            *(buf++) = *(p++);
            *buf = '\0';
            left--;
            continue;
        }

        /* so it wasn't double %'s */
        else if (*p && (left > 0))
        {

            /* search until end of var or beginning of field */
            for (var = p; *p && (*p != '%') && (*p != '.'); p++)
                ;

            field = p;
            if (*p == '.')
            {
                *(p++) = '\0';
                dots = 0;
                for (field = p; *p && ((*p != '%') || (paren_count > 0) || (dots)); p++)
                {
                    if (dots > 0)
                    {
                        *subfield_p = '\0';
                        find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str));
                        if (*repl_str)
                        {
                            snprintf(tmp2, sizeof(tmp2), "eval tmpvr %s", repl_str); // temp var
                            process_eval(go, sc, trig, type, tmp2);
                            strcpy(var, "tmpvr");
                            field = p;
                            dots = 0;
                            continue;
                        }
                        dots = 0;
                    }
                    else if (*p == '(')
                    {
                        *p = '\0';
                        paren_count++;
                    }
                    else if (*p == ')')
                    {
                        *p = '\0';
                        paren_count--;
                    }
                    else if (paren_count > 0)
                    {
                        *subfield_p++ = *p;
                    }
                    else if (*p == '.')
                    {
                        *p = '\0';
                        dots++;
                    }
                } /* for (field.. */
            } /* if *p == '.' */

            *(p++) = '\0';
            *subfield_p = '\0';

            if (*subfield)
            {
                var_subst(go, sc, trig, type, subfield, tmp2);
                strcpy(subfield, tmp2);
            }

            find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str) - 1);

            strncat(buf, repl_str, left);
            len = strlen(repl_str);
            buf += len;
            left -= len;
        } /* else if *p .. */
    } /* while *p .. */
}
