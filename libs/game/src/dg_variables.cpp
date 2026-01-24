/**************************************************************************
 *  File: dg_variables.c                                                   *
 *  Usage: contains the functions dealing with variable substitution.      *
 *                                                                         *
 *                                                                         *
 *  $Author: Mark A. Heilpern/egreen/Welcor $                              *
 *  $Date: 2004/10/11 12:07:00 $                                           *
 *  $Revision: 1.0.14 $                                                    *
 **************************************************************************/
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/DgScript.hpp"
#include "dbat/game/DgScriptPrototype.hpp"
#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/CharacterPrototype.hpp"
#include "dbat/game/Location.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/dg_scripts.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/handler.hpp"
//#include "dbat/game/dg_event.hpp"
//#include "dbat/game/db.hpp"
//#include "dbat/game/screen.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/spells.hpp"
#include "dbat/game/class.hpp"
#include "dbat/game/races.hpp"
#include "dbat/game/TimeInfo.hpp"
#include "dbat/game/UID.hpp"
#include "volcano/util/FilterWeak.hpp"
#include "dbat/game/Random.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/weather.hpp"
#include "dbat/game/Parse.hpp"

#include "dbat/game/const/Environment.hpp"
#include "dbat/game/const/Condition.hpp"
#include "dbat/game/const/Skill.hpp"
#include "dbat/game/const/Direction.hpp"
#include "dbat/game/const/PlayerFlag.hpp"

#include <algorithm>

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

int item_in_list(std::string_view item, const std::vector<std::weak_ptr<Object>> &list)
{
    int count = 0;

    if (list.empty())
        return 0;

    if (item.starts_with(UID_CHAR))
    {
        auto uidResult = resolveUID(item);
        if (!uidResult)
            return 0;
        auto obj = std::dynamic_pointer_cast<Object>(uidResult).get();
        if (!obj)
            return 0;

        for (auto i : volcano::util::filter_raw(list))
        {
            if (i == obj)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->getInventory());
        }
    }
    else if (is_number(item) > -1)
    { /* check for vnum */
        auto ovnum = parseNumber<obj_vnum>(item, "item id").value_or(NOTHING);

        for (auto i : volcano::util::filter_raw(list))
        {
            if (GET_OBJ_VNUM(i) == ovnum)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->getInventory());
        }
    }
    else
    {
        for (auto i : volcano::util::filter_raw(list))
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

int char_has_item(std::string_view item, Character *ch)
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
void var_subst(DgScript *trig, char *line, char *buf)
{
    auto res = dg_substitutions(trig, line);
    snprintf(buf, MAX_STRING_LENGTH, "%s", res.c_str());
}

// this function needs to split a string like "actor.name.first" into {"actor", "name", "first"}
// However, caveat, if we're fed actor.name(%actor.target%).first we need to return
// {"actor", "name(%actor.target%)", "first"}. In other words, we ignore dots inside parens.
// Also, if we have unbalanced parens, we just return the whole string as one field, since it's probably
// an error anyway.
std::vector<std::string_view> dg_split_fields(std::string_view input) {
    std::vector<std::string_view> out;
    
    size_t start = 0;
    size_t paren_count = 0;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '(') {
            paren_count++;
        } else if (input[i] == ')') {
            if (paren_count > 0) paren_count--;
        } else if (input[i] == '.' && paren_count == 0) {
            // found a dot at top level
            out.push_back(input.substr(start, i - start));
            start = i + 1;
        }
    }
    // add the last field
    if (start < input.size()) {
        out.push_back(input.substr(start));
    }
    // if paren_count != 0, we had unbalanced parens, return the whole string as one field
    if (paren_count != 0) {
        out.clear();
        out.push_back(input);
    }

    return out;
}

// if input is a DgUID then we resolveUID it. else we return a std::string of it.
DgReturn resolveVar(std::string_view input) {
    auto res = resolveUID(input);
    if(res) return res.get();
    return std::string(input);
}

DGFUNC(dg_func_time) {
    if (boost::iequals(field, "hour")) {
        return fmt::format("{}", time_info.hours);
    } else if (boost::iequals(field, "minute")) {
        return fmt::format("{}", time_info.minutes);
    } else if(boost::iequals(field, "second")) {
        return fmt::format("{}", time_info.seconds);
    } else if(boost::iequals(field, "day")) {
        return fmt::format("{}", time_info.day + 1);
    } else if(boost::iequals(field, "month")) {
        return fmt::format("{}", time_info.month + 1); // months are 0-11
    } else if(boost::iequals(field, "year")) {
        return fmt::format("{}", time_info.year);
    }
    return "";
}

DGFUNC(dg_func_global) {
    if(!subfield.empty()) {
        dgGlobalVariables.setVariable(field, subfield);
    }
    return resolveVar(dgGlobalVariables.getVariable(field).value_or(""));
}

DGFUNC(dg_func_people) {
    auto numRes = parseNumber<int>(field, "func people");
    if(!numRes) return "0";
    return fmt::format("{}", trgvar_in_room(*numRes));

}

DGFUNC(dg_func_random) {
    if(boost::iequals(field, "char")) {
        std::vector<std::weak_ptr<Character>> chars;
        switch(trig->getAttachType()) {
            case UnitType::character: {
                auto c = (Character*)trig->owner.get();
                if(!c) return "";
                auto candidates = c->location.getPeople();
                for(auto ch : volcano::util::filter_raw(candidates)) {
                    if(ch != c && c->canSee(ch)) {
                        chars.push_back(ch->shared_from_this());
                    }
                }
            }
            break;
            case UnitType::object: {
                auto o = (Object*)trig->owner.get();
                if(!o) return "";
                auto candidates = get_room(obj_room(o))->getPeople().snapshot_weak();
                for(auto ch : volcano::util::filter_raw(candidates)) {
                    if(valid_dg_target(ch, DG_ALLOW_GODS)) {
                        chars.push_back(ch->shared_from_this());
                    }
                }
            }
            break;
            case UnitType::room: {
                auto r = (Room*)trig->owner.get();
                if(!r) return "";
                auto candidates = r->getPeople().snapshot_weak();
                for(auto ch : volcano::util::filter_raw(candidates)) {
                    if(valid_dg_target(ch, DG_ALLOW_GODS)) {
                        chars.push_back(ch->shared_from_this());
                    }
                }
            }
            break;
            default:
                break;
        }
        auto res = Random::get(chars);
        if(res != chars.end()) {
            auto c = res->lock();
            return c.get();
        } else {
            return "";
        }
    }
    if(boost::iequals(field, "dir")) {
        Location loc{};
        switch(trig->getAttachType()) {
            case UnitType::character: {
                auto c = (Character*)trig->owner.get();
                if(!c) return "";
                loc = c->location;
            }
            break;
            case UnitType::object: {
                auto o = (Object*)trig->owner.get();
                if(!o) return "";
                loc = o->getAbsoluteLocation();
            }
            break;
            case UnitType::room: {
                auto r = (Room*)trig->owner.get();
                if(!r) return "";
                loc = Location(r);
            }
            break;
            default:
                break;
        }

        std::vector<int> available;
        for (auto &[d, e] : loc.getExits())
            available.push_back(static_cast<int>(d));
        if(available.empty()) return "";
        auto dir = Random::get(available);
        return dirs[*dir];
    }

    if(auto numRes = parseNumber<int>(field, "func random")) {
        auto num = *numRes;
        if(num <= 0) return "0";
        return fmt::format("{}", Random::get<int>(1, num));
    }
    return "0";
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

*/

DGFUNC(dg_func_findmob) {
    auto numRes = parseNumber<int>(field, "func findmob");
    if(!numRes) return "0";
    auto mvnumRes = parseNumber<mob_vnum>(subfield, "func findmob");

    auto r = get_room(*numRes);
    if(!r) return "0";
    auto people = r->getPeople().snapshot_weak();
    int count = 0;
    if(mvnumRes) {
        auto mv = *mvnumRes;
        for(auto ch : volcano::util::filter_raw(people)) {
            if(ch->vn == mv) {
                count++;
            }
        }
    } else {
        // not a number, so search by name/id
        for(auto ch : volcano::util::filter_raw(people)) {
            if(isname(subfield, ch->getName())) {
                count++;
            }
        }
    }
    return fmt::format("{}", count);
}

DGFUNC(dg_func_findobj) {
    auto numRes = parseNumber<int>(field, "func findobj");
    if(!numRes) return "0";
    auto r = get_room(*numRes);
    if(!r) return "0";

    int count = 0;
    auto objs = r->getObjects().snapshot_weak();
    if(auto ovnumRes = parseNumber<obj_vnum>(subfield, "func findobj")) {
        auto ov = *ovnumRes;
        for(auto obj : volcano::util::filter_raw(objs)) {
            if(obj->vn == ov) {
                count++;
            }
        }
    } else {
        // not a number, so search by name/id
        for(auto obj : volcano::util::filter_raw(objs)) {
            if(isname(subfield, obj->getName())) {
                count++;
            }
        }
    }
    return fmt::format("{}", count);

}

using DgVar = std::variant<DgFunc, DgReturn>;

// input can look like "name" or "name(%actor.target%), or even name(%find.character(John)%)"
// in the case of the first, we return {"name", ""}
// for the second, we return {"name", "%actor.target%"}
// in the last, we return {"name", "%find.character(John%)"}
std::pair<std::string_view, std::string_view> dg_parse_field(DgScript* trig, std::string_view input) {
    auto paren_start = input.find('(');
    if(paren_start == std::string_view::npos) {
        return {input, ""};
    }
    // found a paren, so we need to find the matching closing paren.
    size_t paren_count = 1;
    size_t i = paren_start + 1;
    for(; i < input.size(); ++i) {
        if(input[i] == '(') {
            paren_count++;
        } else if(input[i] == ')') {
            paren_count--;
            if(paren_count == 0) break;
        }
    }
    if(paren_count != 0) {
        // unbalanced parens, return whole string as field
        return {input, ""};
    }
    // now i is at the closing paren
    auto field = input.substr(0, paren_start);
    auto subfield = input.substr(paren_start + 1, i - paren_start - 1);
    // if subfield contains %, we need to process it for substitutions.
    return {field, subfield};
}


std::string dg_text_func(DgScript* trig, std::string_view input, std::string_view field, std::string_view subfield) {
    if(boost::iequals(field, "strlen")) {
        return fmt::format("{}", input.size());
    }

    if(boost::iequals(field, "trim")) {
        auto start = std::string(input);
        boost::trim(start);
        return start;
    }

    if(boost::iequals(field, "contains")) {
        return boost::icontains(input, subfield) ? "1" : "0";
    }

    if(boost::iequals(field, "car")) {
        auto start = std::string(input);
        auto first_space = start.find(' ');
        if(first_space == std::string::npos) {
            return start;
        } else {
            return start.substr(0, first_space);
        }
    }

    if(boost::iequals(field, "cdr")) {
        auto start = std::string(input);
        auto first_space = start.find(' ');
        if(first_space == std::string::npos) {
            return "";
        } else {
            auto next_non_space = start.find_first_not_of(' ', first_space);
            if(next_non_space == std::string::npos) {
                return "";
            } else {
                return start.substr(next_non_space);
            }
        }
    }

    if(boost::iequals(field, "charat")) {
        auto indexRes = parseNumber<size_t>(subfield, "text func charat");
        if(!indexRes) return "";
        size_t index = *indexRes;
        if(index < 1 || index > input.size()) return "";
        return std::string(1, input[index - 1]);
    }

    if(boost::iequals(field, "mudcommand")) {
        int length, cmd;
        for (length = input.size(), cmd = 0;
             *cmd_info[cmd].command != '\n'; cmd++)
            if (!strncmp(cmd_info[cmd].command, input.data(), length))
                break;

        if (*cmd_info[cmd].command == '\n')
            return "";
        else
            return std::string(cmd_info[cmd].command);
    }


    return "";
}


// This is the top-level replace operation for handling %blah% substitutions.
// The reason we need a top-level is that while many scripts just do %actor.name% or %here.name%
// there exist special functions like %time.hour% and %find.character(John)% which need to be
// handled specially. So this will look for those special cases.
std::string dg_replace_fields(DgScript* trig, std::string_view input) {
    auto col = static_cast<int>(trig->getAttachType());
    std::unordered_map<std::string_view, DgVar> specialFuncs = {
        // first the functions...
        {"time", dg_func_time},
        {"global", dg_func_global},
        {"people", dg_func_people},
        {"findmob", dg_func_findmob},
        {"findobj", dg_func_findobj},
        {"random", dg_func_random},

        // the special "self" reference...
        {"self", trig->owner.get()},

        // and explicit string subsitutions used for commands.
        {"ctime", fmt::format("{}", time(nullptr))},
        {"door", door[col]},
        {"force", force[col]},
        {"load", load[col]},
        {"purge", purge[col]},
        {"teleport", teleport[col]},
        {"damage", xdamage[col]},
        {"send", send_cmd[col]},
        {"echo", echo_cmd[col]},
        {"echoaround", echoaround_cmd[col]},
        {"zoneecho", zoneecho[col]},
        {"asound", asound[col]},
        {"at", at[col]},
        {"transform", transform[col]},
        {"recho", recho[col]}
    };


    // next we need to copy in all the variables from trig that don't conflict with the above.
    // this will enable use of things like "actor" and "room" and etc set by most triggers.
    for (const auto& [key, val] : trig->variables) {
        if(specialFuncs.contains(key)) continue;
        specialFuncs[key] = resolveVar(val);
    }
    // now we split the input into fields
    auto fields = dg_split_fields(input);
    if(fields.empty()) return ""; // should not happen..

    // now for the tricky part. we need to process the fields one by one while keeping track of the
    // entity being referenced in a chain.

    DgVar current = "";
    auto iter = fields.begin();
    std::string_view first = *iter;

    for(auto& [k, v] : specialFuncs) {
        if(boost::iequals(k, first)) {
            current = v;
            break;
        }
    }

    iter++;
    while(iter != fields.end()) {
        std::string_view field, subfield, usefield;
        if(iter != fields.end()) {
            std::tie(field, subfield) = dg_parse_field(trig, *iter);
        }

        // owner for a possible recursive substitution.
        std::string subbed;
        // if subfield contains a % we need to process it for substitutions.
        if(!subfield.empty() && subfield.find('%') != std::string_view::npos) {
            subbed = dg_substitutions(trig, subfield);
            usefield = subbed;
        } else {
            usefield = subfield;
        }
        
        if(std::holds_alternative<DgFunc>(current)) {
            // we have a function, so we need to call it with the next field.
            auto func = std::get<DgFunc>(current);
            current = func(trig, field, usefield);
        } else if(std::holds_alternative<DgReturn>(current)) {
            auto ret = std::get<DgReturn>(current);
            if(std::holds_alternative<std::string>(ret)) {
            // strings have a special function to handle them.
            auto input = std::get<std::string>(ret);
            if(!field.empty()) {
                current = dg_text_func(trig, input, field, usefield);
            } else {
                return input;
            }
        } else if(std::holds_alternative<HasDgScripts*>(ret)) {
                // we have an entity. it must be HasDgScripts*
                auto entity = std::get<HasDgScripts*>(ret);
                current = entity->dgCallMember(trig, field, usefield);
            }
        }
        iter++;
    }

    // at this point, current should be a string. But it's hard to say. we need to convert it if not.
    if(std::holds_alternative<DgFunc>(current)) {
        // we have a function, but no field to call it with. just return empty string
        return "";
    } else if(std::holds_alternative<DgReturn>(current)) {
        auto ret = std::get<DgReturn>(current);
        if(std::holds_alternative<std::string>(ret)) {
            return std::get<std::string>(ret);
        } else if(std::holds_alternative<HasDgScripts*>(ret)) {
            auto entity = std::get<HasDgScripts*>(ret);
            return entity->getUID(true);
        }
    }
    return "";
}


// Scan and replace all outermost %...% sections. By outermost, we mean that if we encounter
// opening parens, we stop searching for % until we find the matching closing paren.
// This allows for things like %find.character(John.%Doe%)% to work properly.
std::vector<std::pair<bool, std::string_view>> dg_find_sections(std::string_view input) {
    std::vector<std::pair<bool, std::string_view>> out;
    
    bool in_section = false;
    size_t start = 0;
    size_t i = 0;
    size_t paren_count = 0;
    while(i < input.size()) {
        if(input[i] == '(') {
            paren_count++;
            i++;
        } else if(input[i] == ')') {
            if(paren_count > 0) paren_count--;
            i++;
        } else if(input[i] == '%' && paren_count == 0) {
            // found a top-level %
            if(in_section) {
                in_section = false;
                size_t end = i;
                auto field = input.substr(start, end - start);
                out.emplace_back(true, field);
            } else {
                // copy everything up to here
                if(start < i) {
                    out.emplace_back(false, input.substr(start, i - start));
                }
                in_section = true;
            }
            i++;
            start = i;
        } else {
            i++;
        }
    }
    // copy any remaining text
    if(start < input.size()) {
        out.emplace_back(false, input.substr(start));
    }

    return out;
}

std::string dg_substitutions(DgScript* trig, std::string_view input) {
    std::string out;

    auto res = dg_find_sections(input);
    for(auto& [is_field, text] : res) {
        if(is_field) {
            out.append(dg_replace_fields(trig, text));
        } else {
            out.append(text);
        }
    }

    return out;
}

static const std::map<std::string, std::string> _attr_names = {
    {"str", "strength"},
    {"wis", "wisdom"},
    {"con", "constitution"},
    {"cha", "speed"},
    {"spd", "speed"},
    {"dex", "agility"},
    {"agi", "agility"},
    {"int", "intelligence"},

    {"strength", "strength"},
    {"wisdom", "wisdom"},
    {"constitution", "constitution"},
    {"speed", "speed"},
    {"agility", "agility"},
    {"intelligence", "intelligence"}};

static const std::map<std::string, std::string> _money_names = {
    {"bank", "money_bank"},
    {"gold", "money_carried"},
    {"zenni", "money_carried"}};

static const std::map<std::string, std::string> _cond_names = {
    {"hunger", "hunger"},
    {"thirst", "thirst"},
    {"drunk", "drunk"}};

static const std::map<std::string, int> _save_names = {
    {"saving_fortitude", SAVING_FORTITUDE},
    {"saving_reflex", SAVING_REFLEX},
    {"saving_will", SAVING_WILL}};

static const std::map<std::string, PlayerFlag> _pflags = {
    {"is_killer", PLR_KILLER},
    {"is_thief", PLR_THIEF}};

static const std::map<std::string, AffectFlag> _aflags = {
    {"dead", AFF_SPIRIT},
    {"flying", AFF_FLYING}};


static const std::map<std::string, std::string> _misc_char_stats = {
    {"align", "good_evil"},
    {"death", "death_time"},
    {"level", "level"}
};

static const std::map<std::string, CharVital> _char_vitals = {
    {"hitp", CharVital::health},
    {"mana", CharVital::ki},
    {"move", CharVital::stamina}
};

static const std::map<std::string, std::string> _char_max_vital = {
    {"maxhitp", "health"},
    {"maxmana", "ki"},
    {"maxmove", "stamina"}
};


template<typename StatType>
std::string dgCharacterHandleStat(Character* go, const std::string& stat, const std::string& arg) {
    if (!arg.empty())
    {
        auto addRes = parseNumber<StatType>(arg, "dgHandleStat").value_or(0);
        go->modBaseStat(stat, addRes);
    }
    return fmt::format("{}", go->getBaseStat<StatType>(stat));
}

template<typename EnumType>
requires std::is_enum_v<EnumType>
std::string dgHandleFlags(FlagHandler<EnumType>& flags, const std::string& arg, std::optional<bool> value = std::nullopt) {
    if(!arg.empty()) {
        auto res = volcano::util::chooseEnum<EnumType>(arg, "dgHandleFlag");
        if(res) {
            if(value.has_value()) {
                flags.set(*res, value.value());
            }
            return flags.get(*res) ? "1" : "0";
        }
    }
    return "0";
}

template<typename EnumType>
requires std::is_enum_v<EnumType>
std::string dgHandleEnum(EnumType& val, const std::string& arg) {
    if(!arg.empty()) {
        auto res = volcano::util::chooseEnum<EnumType>(arg, "dgHandleEnum");
        if(res) {
            val = *res;
        }
    }
    return std::string(enchantum::to_string(val));
}

DgReturn Character::dgCallMember(DgScript* trig, std::string_view field, std::string_view subfield) {
    std::string member(field);
    boost::trim(member);
    auto lmember = boost::to_lower_copy(member);

    std::string arg(subfield);
    boost::trim(arg);

    if(boost::iequals(lmember, "alias")) return getName();

    if (auto attr = _attr_names.find(lmember); attr != _attr_names.end())
    {
        return dgCharacterHandleStat<attribute_t>(this, attr->second, arg);
    }

    if (auto mon = _money_names.find(lmember); mon != _money_names.end())
    {
        return dgCharacterHandleStat<money_t>(this, mon->second, arg);
    }

    if (auto con = _cond_names.find(lmember); con != _cond_names.end())
    {
        return dgCharacterHandleStat<int>(this, con->second, arg);
    }

    if(auto misc = _misc_char_stats.find(lmember); misc != _misc_char_stats.end()) {
        return dgCharacterHandleStat<int>(this, misc->second, arg);
    }

    if (auto save = _save_names.find(lmember); save != _save_names.end())
    {
        return fmt::format("{}", 0);
    }

    if (auto pf = _pflags.find(lmember); pf != _pflags.end())
    {
        if (!arg.empty())
        {
            if (boost::iequals("on", arg.c_str()))
                player_flags.set(pf->second, true);
            else if (boost::iequals("off", arg.c_str()))
                player_flags.set(pf->second, false);
        }
        return player_flags.get(pf->second) ? "1" : "0";
    }

    if (auto af = _aflags.find(lmember); af != _aflags.end())
    {
        return AFF_FLAGGED(this, af->second) ? "1" : "0";
    }

    if(auto cv = _char_vitals.find(lmember); cv != _char_vitals.end()) {
        if(!arg.empty()) {
            auto addRes = parseNumber<int64_t>(arg, "dgCallMember char vital").value_or(0);
            modCurVital(cv->second, addRes);
        }
        return fmt::format("{}", getCurVital(cv->second));
    }

    if(auto cmv = _char_max_vital.find(lmember); cmv != _char_max_vital.end()) {
        if(!arg.empty()) {
            auto addRes = parseNumber<int64_t>(arg, "dgCallMember char max vital").value_or(0);
            modBaseStat(cmv->second, addRes);
        }
        return fmt::format("{}", getEffectiveStat<int64_t>(cmv->second));
    }

    if(boost::iequals(lmember, "affect")) {
        if(!arg.empty()) {
            auto res = volcano::util::chooseEnum<AffectFlag>(arg, "affect flags");
            if(res) {
                return AFF_FLAGGED(this, *res) ? "1" : "0";
            }
        }
        return "0";
    }

    if(boost::iequals(lmember, "canbeseen")) {
        if(trig->getAttachType() == UnitType::character) {
            auto actor = (Character*)trig->owner.get();
            return actor->canSee(this) ? "1" : "0";
        }
        return "0";
    }

    if(boost::iequals(lmember, "carry")) {
        return CARRYING(this) ? "1" : "0";
    }

    if(boost::iequals(lmember, "clan")) return "";

    if(boost::iequals(lmember, "class") || boost::iequals(lmember, "sensei"))
        return std::string(enchantum::to_string(sensei));

    
    if(boost::iequals(lmember, "drag"))
        return DRAGGING(this) ? "1" : "0";
    
    if(boost::iequals(lmember, "eq")) {
        auto eq = getEquipment();
        if(boost::iequals(arg, "*")) return !eq.empty() ? "1" : "0";
        if(auto numRes = parseNumber<int>(arg, "dgCallMember eq"); numRes) {
            int pos = *numRes;
            if(auto find = eq.find(pos); find != eq.end()) {
                return find->second;
            }
        }
        return "";
    }

    if(boost::iequals(lmember, "exp")) {
        if(!arg.empty()) {
            auto addRes = parseNumber<int64_t>(arg, "dgCallMember exp");
            if(addRes) {
                modExperience(*addRes);
            }
        }
        return fmt::format("{}", GET_EXP(this));
    }

    if(boost::iequals(lmember, "fighting")) {
        if(auto f = FIGHTING(this)) {
            return f;
        }
        else {return "";}
    }
    
    if(boost::iequals(lmember, "follower")) {
        if(!followers) return "";
        auto h = followers.head();
        return h;
    }

    if(boost::iequals(lmember, "has_item")) {
        if(arg.empty()) return "";
        return fmt::format("{}", char_has_item(arg, this));
    }

    if(boost::iequals(lmember, "hisher"))
        return HSHR(this);
    
    
    if(boost::iequals(lmember, "heshe"))
        return HSSH(this);

    if(boost::iequals(lmember, "himher"))
        return HMHR(this);

    if(boost::iequals(lmember, "id"))
        return getUID(true);
    
    if(boost::iequals(lmember, "is_pc"))
        return IS_NPC(this) ? "0" : "1";
    
    if(boost::iequals(lmember, "inventory")) {
        auto inv = getInventory();
        // if arg is empty, return first item...
        if(arg.empty()) {
            for(auto item : volcano::util::filter_raw(inv)) {
                return item;
            }
            return "";
        }
        auto numRes = parseNumber<obj_vnum>(arg, "dgCallMember inventory");
        if(numRes) {
            obj_vnum vnum = *numRes;
            for(auto item : volcano::util::filter_raw(inv)) {
                if(GET_OBJ_VNUM(item) == vnum) {
                    return item;
                }
            }
        }
    }

    if(boost::iequals(lmember, "master")) {
        if(master) {return master;}
        else {return "";}   
    }

    if(boost::iequals(lmember, "name")) {
        return IS_NPC(this) ? getShortDescription() : getName();
    }
    
    if(boost::iequals(lmember, "next_in_room")) {
        if (auto people = location.getPeople(); !people.empty())
        {
            auto found = false;
            for (auto p : volcano::util::filter_raw(people))
            {
                if (found)
                {
                    return p;
                }
                if (p == this)
                {
                    found = true;
                }
            }
        }
        return "";
    }

    if(boost::iequals(lmember, "pos")) {
        return dgHandleEnum(position, arg);
    }

    if(boost::iequals(lmember, "prac")) {
        if(!arg.empty()) {
            auto addRes = parseNumber<int>(arg, "dgCallMember prac");
            if(addRes) {
                modPractices(*addRes);
            }
        }
        return fmt::format("{}", GET_PRACTICES(this));
    }

    if(boost::iequals(lmember, "plr"))
        {return dgHandleFlags(player_flags, arg);}
    if(boost::iequals(lmember, "pref"))
        {return dgHandleFlags(pref_flags, arg);}
    
    if(boost::iequals(lmember, "room")) {
        if(auto r = getRoom())
            {return r;}
        else
            {return "";}
    }
    
    if(boost::iequals(lmember, "race")) {
        return std::string(enchantum::to_string(race));
    }

    if(boost::iequals(lmember, "rpp")) {
        if(!arg.empty()) {
            auto addRes = parseNumber<int>(arg, "dgCallMember rpp");
            if(addRes) {
                modRPP(*addRes);
            }
        }
        return fmt::format("{}", getRPP());
    }

    if(boost::iequals(lmember, "sex")) {
        return std::string(enchantum::to_string(sex));
    }
    
    if(boost::iequals(lmember, "skillset")) {
        if(!arg.empty()) {
            std::vector<std::string_view> parts;
            boost::split(parts, arg, boost::is_space(), boost::token_compress_on);
            if(parts.size() != 2) return "";
            auto skRes = volcano::util::chooseEnum<Skill>(parts[0], "dgCallMember skillset");
            if(!skRes) {
                return "";
            }
            auto numRes = parseNumber<int>(parts[1], "dgCallMember skillset");
            if(!numRes) {
                return "";
            }
            SET_SKILL(this, static_cast<int>(*skRes), *numRes);
        }
    }

    if(boost::iequals(lmember, "size")) {
        return dgHandleEnum(size, arg);
    }
    
    if(boost::iequals(lmember, "tnl"))
        return fmt::format("{}", level_exp(this, GET_LEVEL(this) +1));

    if(boost::iequals(lmember, "vnum"))
        return fmt::format("{}", IS_NPC(this) ? vn : NOTHING);

    if(boost::iequals(lmember, "varexists"))
        return variables.contains(arg) ? "1" : "0";
    
    if(boost::iequals(lmember, "weight"))
        return fmt::format("{}", getEffectiveStat("weight"));

    if(auto found = getVariable(member); found)
        return resolveVar(*found);
    
    script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
        GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), member.c_str());

    return "";
}

static const std::map<std::string, std::string> _obj_misc_stats = {
    {"cost", "cost"},
    {"cost_per_day", "cost_per_day"},
    {"level", "level"},
    {"health", "health"},
    {"timer", "timer"}
};

DgReturn Object::dgCallMember(DgScript* trig, std::string_view field, std::string_view subfield) {
    std::string member(field);
    boost::trim(member);
    auto lmember = boost::to_lower_copy(member);

    std::string arg(subfield);
    boost::trim(arg);

    if(auto find = _obj_misc_stats.find(lmember); find != _obj_misc_stats.end()) {
        if(!arg.empty()) {
            auto addRes = parseNumber<int>(arg, "dgCallMember obj misc stat");
            if(addRes) {
                setBaseStat(find->second, *addRes);
            }
        }
        return fmt::format("{}", getEffectiveStat<int>(find->second));
    }

    if(boost::iequals(lmember, "affects")) {
        return dgHandleFlags(affect_flags, arg);
    }
    
    if(boost::iequals(lmember, "carried_by")) {
        if(auto c = getCarriedBy()) {return c;}
        else {return "";}
    }
    
    if(boost::iequals(lmember, "contents")) {
        if (auto con = getInventory(); !con.empty())
        {
            for (auto obj : volcano::util::filter_raw(con))
            {
                return obj;
            }
        }
        return "";
    }

    if(boost::iequals(lmember, "count")) {
        if(type_flag == ItemType::container) {
            return fmt::format("{}", getInventory().size());
        }
        return "0";
    }

    if(boost::iequals(lmember, "extra"))
        return dgHandleFlags(item_flags, arg);
    
    if(boost::iequals(lmember, "has_in")) {
        if(type_flag == ItemType::container) {
            return item_in_list((char*)arg.c_str(), getInventory()) ? "1" : "0";
        }
        return "0";
    }

    if(boost::iequals(lmember, "id"))
        return getUID(true);

    if(boost::iequals(lmember, "is_inroom") || boost::iequals(lmember, "room") || boost::iequals(lmember, "in_room")) {
        if(auto r = getRoom()) {return r;}
        else {return "";}
    }

    if(boost::iequals(lmember, "is_pc")) return "-1";

    if(boost::iequals(lmember, "itemflag")) 
        return dgHandleFlags(item_flags, arg);
    
    if(boost::iequals(lmember, "name")) {
        if(!arg.empty()) {
            name = fmt::format("{} {}", name, arg);
        }
        return getName();
    }

    if(boost::iequals(lmember, "next_in_list")) {
        if (auto con = location.getObjects(); !con.empty())
        {
            auto found = false;
            for (auto ob : volcano::util::filter_raw(con))
            {
                if (ob == this)
                {
                    found = true;
                    continue;
                }
                if (found)
                {
                    return ob;
                }
            }
        }
        return "";
    }

    if(boost::iequals(lmember, "shortdesc")) {
        if(!arg.empty()) {
            short_description = fmt::format("{} @wnicknamed @D(@C{}@D)@n", short_description, arg);
        }
        return getShortDescription();
    }

    if(boost::iequals(lmember, "setaffects")) return dgHandleFlags(affect_flags, arg, true);
    if(boost::iequals(lmember, "setextra")) return dgHandleFlags(item_flags, arg, true);
    if(boost::iequals(lmember, "size")) return std::string(enchantum::to_string(size));
    if(boost::iequals(lmember, "type")) return std::string(enchantum::to_string(type_flag));

    if(boost::iequals(lmember, "value")) {
        if(!arg.empty()) {return fmt::format("{}", GET_OBJ_VAL(this, arg));}
        else {return "";}
    }

    if(boost::iequals(lmember, "vnum")) return fmt::format("{}", getVnum());
    if(boost::iequals(lmember, "weight")) {
        if(!arg.empty()) {
            auto addRes = parseNumber<int>(arg, "dgCallMember weight");
            if(addRes) {
                setBaseStat("weight", *addRes);
            }
        }
        return fmt::format("{}", getEffectiveStat<weight_t>("weight"));
    }

    if(boost::iequals(lmember, "worn_by")) {
        auto w = getWornBy();
        if(w) return w;
        return "";
    }

    if(auto found = getVariable(member); found)
        return resolveVar(*found);

    script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
        GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), static_cast<int>(type), member.c_str());

    return "";
}

static const std::unordered_set<int> inside_sectors = {SECT_INSIDE, SECT_UNDERWATER, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};

static const std::map<std::string, Direction> _dirNames = {
    {"north", Direction::north},
    {"east", Direction::east},
    {"south", Direction::south},
    {"west", Direction::west},
    {"up", Direction::up},
    {"down", Direction::down},
    {"northwest", Direction::northwest},
    {"northeast", Direction::northeast},
    {"southwest", Direction::southwest},
    {"southeast", Direction::southeast},
    {"inside", Direction::inside},
    {"outside", Direction::outside}

};

DgReturn Room::dgCallMember(DgScript* trig, std::string_view field, std::string_view subfield) {
    std::string member(field);
    boost::trim(member);
    auto lmember = boost::to_lower_copy(member);

    std::string arg(subfield);
    boost::trim(arg);

    if (auto d = _dirNames.find(lmember); d != _dirNames.end())
    {
        auto ex = getDirection(d->second);
        if (!ex)
        {
            return "";
        }
        if (!arg.empty())
        {
            if (boost::iequals(arg.c_str(), "vnum"))
            {
                return fmt::format("{}", ex->getVnum());
            }
            else if (boost::iequals(arg.c_str(), "key"))
                return fmt::format("{}", ex->key);
            else if (boost::iequals(arg.c_str(), "bits"))
            {
                return ex->exit_flags.getFlagNames();
            }
            else if (boost::iequals(arg.c_str(), "room"))
            {
                return fmt::format("{}", ex->getUID(true));
            }
        }
        else /* no subfield - default to bits */
        {
            return ex->exit_flags.getFlagNames();
        }
    }

    if(boost::iequals(lmember, "vnum")) return fmt::format("{}", getVnum());
    if(boost::iequals(lmember, "id")) return getUID(true);

    if(boost::iequals(lmember, "name")) return getName();
    if(boost::iequals(lmember, "sector")) return std::string(enchantum::to_string(sector_type));

    if(boost::iequals(lmember, "gravity")) {
        Location loc(this);
        return fmt::format("{}", (int)loc.getEnvironment(ENV_GRAVITY));
    }

    if(boost::iequals(lmember, "contents")) {
        auto con = getObjects().snapshot_weak();
        if(!arg.empty()) {
            // search for vnum...
            auto v = parseNumber<obj_vnum>(arg, "dgCallMember room contents").value_or(-2);
            for(auto obj : volcano::util::filter_raw(con)) {
                if(GET_OBJ_VNUM(obj) == v) {
                    return obj;
                }
            }
        } else {
            for(auto obj : volcano::util::filter_raw(con)) {
                return obj;
            }
        }
        return "";
    }

    if(boost::iequals(lmember, "people")) {
        auto ppl = getPeople().snapshot_weak();
        for(auto ch : volcano::util::filter_raw(ppl)) {
            return ch;
        }
        return "";
    }

    if(boost::iequals(lmember, "weather")) {
        if(room_flags.get(ROOM_INDOORS)) return "";
        const char *sky_look[] = {
                    "sunny",
                    "cloudy",
                    "rainy",
                    "lightning"};
        return sky_look[weather_info.sky];
    }

    if(boost::iequals(lmember, "fishing"))
        return room_flags.get(ROOM_FISHING) ? "1" : "0";

    if(boost::iequals(lmember, "zonenumber")) return fmt::format("{}", zone->number);
    if(boost::iequals(lmember, "zonename")) return fmt::format("{}", zone->name);

    if(boost::iequals(lmember, "roomflag")) return dgHandleFlags(room_flags, arg);

    if(auto found = getVariable(member); found)
        return resolveVar(*found);
    
    script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), static_cast<int>(type), member.c_str());

    return "";
}

