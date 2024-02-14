/**************************************************************************
*  File: dg_variables.c                                                   *
*  Usage: contains the functions dealing with variable substitution.      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00 $                                           *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"
#include "dbat/db.h"
#include "dbat/screen.h"
#include "dbat/constants.h"
#include "dbat/spells.h"
#include "dbat/oasis.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/random.h"

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

/* Utility functions */


/* perhaps not the best place for this, but I didn't want a new file */
char *skill_percent(struct char_data *ch, char *skill) {
    static char retval[16];
    int skillnum;

    skillnum = find_skill_num(skill, SKTYPE_SKILL);
    if (skillnum <= 0) return ("unknown skill");

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

int item_in_list(char *item, std::vector<obj_data*> list) {
    int count = 0;

    if (!item || !*item)
        return 0;

    if (*item == UID_CHAR) {
        auto result = resolveUID(item);
        if(!result) return 0;
        auto obj = dynamic_cast<obj_data*>(result);
        if(!obj) return 0;

        for (auto i : list) {
            if (i == obj)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->getInventory());
        }
    } else if (is_number(item) > -1) { /* check for vnum */
        obj_vnum ovnum = atof(item);

        for (auto i : list) {
            if (GET_OBJ_VNUM(i) == ovnum)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->getInventory());
        }
    } else {
        for (auto i : list) {
            if (isname(item, i->name))
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

int char_has_item(char *item, struct char_data *ch) {

    /* If this works, no more searching needed */
    if (get_object_in_equip(ch, item) != nullptr)
        return 1;

    if (item_in_list(item, ch->getInventory()) == 0)
        return 0;
    else
        return 1;
}


std::size_t matching_percent(const std::string& line, std::size_t start) {
    int depth = 0;

    for (auto i = start+1; i < line.size(); i++) {
        auto p = line[i];
        if (p == '(')
            depth++;
        else if (p == ')')
            depth--;
        else if (p == '%' && depth == 0)
            return i;
    }

    return line.size();
}


std::string scriptTimeHolder(trig_data *trig, const std::string& field, const std::string& args) {
    if(iequals(field, "hour")) {
        return fmt::format("{}", time_info.hours);
    } else if(iequals(field, "minute")) {
        return fmt::format("{}", time_info.minutes);
    } else if(iequals(field, "second")) {
        return fmt::format("{}", time_info.seconds);
    } else if(iequals(field, "day")) {
        return fmt::format("{}", time_info.day + 1);
    } else if(iequals(field, "month")) {
        return fmt::format("{}", time_info.month + 1);
    } else if(iequals(field, "year")) {
        return fmt::format("{}", time_info.year);
    }
    return "";
}

std::optional<std::size_t> findDot(const std::string& line) {
    int depth = 0;
    bool escaped = false;
    for(std::size_t i = 0; i < line.size(); i++) {
        if(escaped) {
            escaped = false;
            continue;
        }
        switch(line[i]) {
            case '(':
                depth++;
                break;
            case ')':
                depth--;
                break;
            case '\\':
                escaped = true;
                break;
            case '.':
                if(depth == 0) return i;
                break;
        }
    }
    return {};
}

std::optional<std::size_t> findEndParen(const std::string& line, std::size_t start) {
    int depth = 0;
    bool escaped = false;
    for(std::size_t i = start; i < line.size(); i++) {
        if(escaped) {
            escaped = false;
            continue;
        }
        switch(line[i]) {
            case '(':
                depth++;
                break;
            case ')':
                depth--;
                if(depth == 0) return i;
                break;
            case '\\':
                escaped = true;
                break;
        }
    }
    return {};
}

DgResults scriptFindMob(trig_data *trig, const std::string& field, const std::string& args) {
    if(args.empty() || field.empty()) return "0";
    room_rnum rrnum = real_room(atof(field.c_str()));
    mob_vnum mvnum = atof(args.c_str());
    if (rrnum == NOWHERE) {
        script_log("findmob.vnum(ovnum): No room with vnum %d", atof(field.c_str()));
        return "0";
    }
    auto i = 0;
    for (auto ch = dynamic_cast<room_data*>(world[rrnum])->people; ch; ch = ch->next_in_room)
        if (GET_MOB_VNUM(ch) == mvnum)
            i++;
    return fmt::format("{}", i);

}

DgResults scriptFindObj(trig_data *trig, const std::string& field, const std::string& args) {
    if(args.empty() || field.empty()) return "0";
    room_rnum rrnum = real_room(atof(field.c_str()));

    if (rrnum == NOWHERE) {
        script_log("findobj.vnum(ovnum): No room with vnum %d", atof(field.c_str()));
        return "0";
    }
     /* item_in_list looks within containers as well. */
    return fmt::format("{}", item_in_list((char*)args.c_str(), world[rrnum]->getInventory()));
}

DgResults scriptGlobal(trig_data *trig, const std::string& field, const std::string& args) {
    // TODO: not implemented yet!
    return "";
}

DgResults scriptRandom(trig_data *trig, const std::string& field, const std::string& args) {
    auto type = trig->parent->attach_type;
    struct char_data *enactor;
    room_data *r;
    switch(type) {
            case MOB_TRIGGER:
                enactor = (struct char_data*)trig->sc->owner;
                r = enactor->getRoom();
                break;
            case OBJ_TRIGGER:
                r = ((struct obj_data*)trig->sc->owner)->getRoom();
                break;
            case WLD_TRIGGER:
                r = (struct room_data*)trig->sc->owner;
                break;
        }

    if(iequals(field, "char")) {
        if(!r) return "";
        std::vector<struct char_data*> candidates;
        for(auto c = r->people; c; c = c->next_in_room) {
            if(type == MOB_TRIGGER && !CAN_SEE(enactor, c)) continue;
            if(!valid_dg_target(c, DG_ALLOW_GODS)) continue;
            candidates.push_back(c);
        }
        if(candidates.empty()) return "0";
        auto can = Random::get(candidates);
        return (*can);
    } else if(iequals(field, "dir")) {
        if(!r) return "";
        std::vector<int> available;
        for (auto i = 0; i < NUM_OF_DIRS; i++)
            if (R_EXIT(r, i))
                available.push_back(i);

        if (available.empty()) {
            return "";
        } else {
            auto dir = Random::get(available);
            return dirs[*dir];
        }
    } else {
        if(auto num = atoi(field.c_str()); num >= 1) {
            return fmt::format("{}", Random::get(1, num));
        }
    }

    return "";
}

std::string scriptTextProcess(trig_data *trig, const std::string& text, const std::string& field, const std::string& args) {
        char *p, *p2;
    char tmpvar[MAX_STRING_LENGTH];

    if (iequals(field, "strlen")) {                     /* strlen    */
        return fmt::format("{}", text.size());
    } else if (iequals(field, "trim")) {                /* trim      */
        std::string clone(text);
        trim(clone);
        return clone;
    } else if (iequals(field, "contains")) {            /* contains  */
        return str_str((char*)text.c_str(), (char*)args.c_str()) ? "1" : "0";
    } else if (iequals(field, "car")) {
        auto sp = split(text, ' ');
        if(!sp.empty()) return sp[0];
        return "";
    } else if (iequals(field, "cdr")) {                 /* cdr       */
        auto cdr = text.c_str();
        while (*cdr && !isspace(*cdr)) cdr++; /* skip 1st field */
        while (*cdr && isspace(*cdr)) cdr++;  /* skip to next */
        return cdr;
    } else if (iequals(field, "charat")) {              /* CharAt    */
        size_t len = text.size(), dgindex = atoi(args.c_str());
        if (dgindex > len || dgindex < 1)
            return "";
        else
            return fmt::format("{}", text[dgindex - 1]);
    } else if (iequals(field, "mudcommand")) {
        /* find the mud command returned from this text */
/* NOTE: you may need to replace "cmd_info" with "complete_cmd_info", */
/* depending on what patches you've got applied.                      */

/* on older source bases:    extern struct command_info *cmd_info; */
        int length, cmd;
        for (length = text.size(), cmd = 0;
             *cmd_info[cmd].command != '\n'; cmd++)
            if (!strncmp(cmd_info[cmd].command, text.c_str(), length))
                break;

        if (*cmd_info[cmd].command == '\n')
            return "";
        else
            return fmt::format("{}", cmd_info[cmd].command);
    }

    return {};
}

DgResults checkForID(const std::string& text) {
    if(auto check = resolveUID(text); check) {
        return check;
    }
    return text;
}


void trig_data::handleSubst(std::vector<DgHolder> &current, const std::string& field, const std::string& args) {
    auto type = parent->attach_type;
    
    if(current.empty()) {
        // it's the first run.
        if(iequals(field, "self")) {
            current.emplace_back(sc->owner);
        }
        else if(iequals(field, "time")) {
            current.emplace_back(std::function(scriptTimeHolder));
        }
        else if (iequals(field, "global")) {
            current.emplace_back(std::function(scriptGlobal));
        }
        else if(iequals(field, "people")) {
            int num = 0;
            auto f = fmt::format("{}", ((num = atoi(args.c_str())) > 0) ? trgvar_in_room(num) : 0);
            current.emplace_back(f);
        }
        else if(iequals(field, "findmob")) {
            current.emplace_back(std::function(scriptFindMob));
        }
        else if(iequals(field, "findobj")) {
            current.emplace_back(std::function(scriptFindObj));
        }
        else if(iequals(field, "random")) {
            current.emplace_back(std::function(scriptRandom));
        }
        else if (iequals(field, "ctime"))
            current.emplace_back(fmt::format("{}", time(nullptr)));
        else if (iequals(field, "door"))
            current.emplace_back(door[type]);
        else if (iequals(field, "force"))
            current.emplace_back(force[type]);
        else if (iequals(field, "load"))
            current.emplace_back(load[type]);
        else if (iequals(field, "purge"))
            current.emplace_back(purge[type]);
        else if (iequals(field, "teleport"))
            current.emplace_back(teleport[type]);
        else if (iequals(field, "damage"))
            current.emplace_back(xdamage[type]);
        else if (iequals(field, "send"))
            current.emplace_back(send_cmd[type]);
        else if (iequals(field, "echo"))
            current.emplace_back(echo_cmd[type]);
        else if (iequals(field, "echoaround"))
            current.emplace_back(echoaround_cmd[type]);
        else if (iequals(field, "zoneecho"))
            current.emplace_back(zoneecho[type]);
        else if (iequals(field, "asound"))
            current.emplace_back(asound[type]);
        else if (iequals(field, "at"))
            current.emplace_back(at[type]);
        else if (iequals(field, "transform"))
            current.emplace_back(transform[type]);
        else if (iequals(field, "recho"))
            current.emplace_back(recho[type]);
        else if(hasVar(field)) {
            auto res = getVar(field);
            if(res.index() == 0) {
                current.emplace_back(std::get<0>(res));
            } else {
                current.emplace_back(std::get<1>(res));
            }
        } else {
            current.emplace_back("");
        }
        return;
    }

    auto back = current.back();
    switch(back.index()) {
        case 0: {
            // Strings. strings will invoke the string manipulation funcs.
            auto s = std::get<0>(back);
            current.emplace_back(scriptTextProcess(this, s, field, args));
            }
            break;
        case 1: {
            // a unit_data*.
            auto u = std::get<1>(back);
            // here we'll call the dgCallMember method and set the result into current...
            auto res = u->dgCallMember(this, field, args);
            if(res.index() == 0) {
                    current.emplace_back(std::get<0>(res));
                } else {
                    current.emplace_back(std::get<1>(res));
                }
            }
            break;
        case 2: {
                // a function.
                auto f = std::get<2>(back);
                // here we'll call the function and set the result into current...
                auto res = f(this, field, args);
                if(res.index() == 0) {
                    current.emplace_back(std::get<0>(res));
                } else {
                    current.emplace_back(std::get<1>(res));
                }
            }
            break;
    }
}

std::string trig_data::innerSubst(std::vector<DgHolder>& current, const std::string& expr) {
    // This function will only be called on a string where the first character is right past a %  
    std::string chopped = expr;

    std::string field, args;
    bool finished = false;

    while(!chopped.empty()) {
        // if the first character is a %, we must recurse.
        if(chopped[0] == '%') {
            chopped = evalExpr(chopped);
            continue;
        }
        // After this SHOULD BE a field (no spaces allowed), which ends in a either a dot, open paren, or %.
        // First, let's find the end of the field.
        std::size_t end = 0;
        char found = -1;
        for(std::size_t i = 0; i < chopped.size(); i++) {
            if(chopped[i] == '.' || chopped[i] == '(' || chopped[i] == '%') {
                end = i;
                found = chopped[i];
                break;
            }
        }
        field = chopped.substr(0, end);
        switch(found) {
            case -1:
                // we reached the very end and found no %.
                throw DgScriptException("Unexpected end of string in innerSubst. No % found.");
                break;
            case '.': {
                // we found a dot. Capture the word up to but not including end.
                // be careful to not go past the end of the string. the . might be the end.
                if(end+1 < chopped.size()) {
                    chopped = chopped.substr(end+1);
                } else {
                    throw DgScriptException("Unexpected end of string in innerSubst. Dot with no following field.");
                }
            }
                break;
            case '(': {
                // An opening parentheses. We need to find the matching closing parentheses.
                if(auto endParen = findEndParen(chopped, end); endParen) {
                    args = chopped.substr(end+1, endParen.value()-end-1);
                    args = evalExpr(args);
                    // Be careful to not go past the end of the string. The closing paren might be the end.
                    if(endParen.value()+1 < chopped.size()) {
                        chopped = chopped.substr(endParen.value()+1);
                    } else {
                        chopped = "";
                    }
                } else {
                    // we didn't find the closing parentheses. We're done here.
                    throw DgScriptException("Unexpected end of string in innerSubst. Open paren with no matching close paren.");
                }
                // should it exist, if the following character is not a . or %, we should throw an error.
                if(!chopped.empty()) {
                    switch(chopped[0]) {
                        case '.':
                            // advance the string past the dot.
                            chopped = chopped.substr(1);
                            break;
                        case '%':
                            chopped = chopped.substr(1);
                            finished = true;
                            break;
                        default:
                            throw DgScriptException(fmt::format("Unexpected {} after closing paren in innerSubst.", chopped[0]));
                    }
                }
            }
            break;
            case '%': {
                finished = true;
                // A % character. We're done here.
                // If there is anything after the %, we must return it.
                if(end+1 < chopped.size()) {
                    chopped = chopped.substr(end+1);
                } else {
                    chopped = "";
                }
            }
        }

        // At the tail end of every while loop, we should have a field and args.
        if(!field.empty()) {
            handleSubst(current, field, args);
            field.clear();
            args.clear();
        }
        if(finished) return chopped;
        
    }
    return "";
}

std::string trig_data::varSubst(const std::string& expr) {
    std::string out;

    std::string chopped = expr;
    
    while(chopped.contains('%')) {
        std::vector<DgHolder> current;
        // Copy the string up to the first % into out.
        auto find = chopped.find('%');
        out += chopped.substr(0, find);
        // gotta be careful to make sure we don't go past the end of the string.
        if(find+1 < chopped.size()) {
            chopped = chopped.substr(find+1);
        } else {
            break;
        }
        // We get the remainder back...
        chopped = innerSubst(current, chopped);
        // Extract from current and append to out.
        if(current.empty()) continue;
        auto back = current.back();
        switch(back.index()) {
            case 0: {
                out += std::get<0>(back);
            }
                break;
            case 1: {
                out += std::get<1>(back)->getUID();
            }
                break;
            default:
                break;
        }
    }

    return out + chopped;
}