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

int item_in_list(char *item, obj_data *list) {
    obj_data *i;
    int count = 0;

    if (!item || !*item)
        return 0;

    if (*item == UID_CHAR) {
        std::optional<DgUID> result;
        result = resolveUID(item);
        auto uidResult = result;
        if(!uidResult) return 0;
        if(uidResult->index() != 1) return 0;
        auto obj = std::get<1>(*uidResult);

        for (i = list; i; i = i->next_content) {
            if (i == obj)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->contents);
        }
    } else if (is_number(item) > -1) { /* check for vnum */
        obj_vnum ovnum = atof(item);

        for (i = list; i; i = i->next_content) {
            if (GET_OBJ_VNUM(i) == ovnum)
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->contents);
        }
    } else {
        for (i = list; i; i = i->next_content) {
            if (isname(item, i->name))
                count++;
            if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
                count += item_in_list(item, i->contents);
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

    if (item_in_list(item, ch->contents) == 0)
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

std::vector<std::pair<std::string, std::string>> trig_data::splitFields(const std::string& line) {
    std::vector<std::pair<std::string, std::string>> out;
    std::string l = line;
    trim(l);

    std::vector<std::string> dotSeparators;
    // First, we'll chop up l and fill dotSeparators with the chunks, separated by dots.
    while(true) {
        auto dot = findDot(l);
        if(!dot) {
            dotSeparators.emplace_back(l);
            break;
        }
        dotSeparators.emplace_back(l.substr(0, dot.value()));
        l = l.substr(dot.value()+1);
        trim(l);
    }

    // Now we'll go through dotSeparators and split each chunk into a field and an argument.
    // Not every chunk will have an argument, so we'll have to handle that.
    // Arguments follow the field, enclosed by parentheses. Which might be nested.
    for(auto& chunk : dotSeparators) {
        std::string field;
        std::string args;
        auto paren = chunk.find('(');
        if(paren == std::string::npos) {
            field = chunk;
        } else {
            field = chunk.substr(0, paren);
            auto endParen = findEndParen(chunk, paren);
            if(endParen) {
                args = chunk.substr(paren+1, endParen.value()-paren-1);
                trim(args);
                // Recurse the arguments in case there's anything in there that needs to be split.
                args = evalExpr(args);
            }
        }
        out.emplace_back(field, args);
    }
    

    return out;
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
    for (auto ch = world[rrnum].people; ch; ch = ch->next_in_room)
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
    return fmt::format("{}", item_in_list((char*)args.c_str(), world[rrnum].contents));
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
    auto uidCheck = resolveUID(text);
    if(uidCheck) {
        auto uid = *uidCheck;
        switch(uid.index()) {
            case 0: {
                return std::get<0>(uid);
            }
            case 1: {
                return std::get<1>(uid);
            }
            case 2: {
                return std::get<2>(uid);
            }
        }
    }
    return text;
}


std::string trig_data::handleSubst(const std::string& expr) {
    auto type = parent->attach_type;
    DgHolder current = "";
    int i = 0;
    int num = 0;
    
    for(const auto &[field, args] : splitFields(expr)) {
        if(i++ == 0) {
            // it's the first run.
            if(iequals(field, "self")) {
                current = sc->owner;
            }
            else if(iequals(field, "time")) {
                current = std::function(scriptTimeHolder);
            }
            else if (iequals(field, "global")) {
                current = std::function(scriptGlobal);
            }
            else if(iequals(field, "people")) {
                current = fmt::format("{}", ((num = atoi(args.c_str())) > 0) ? trgvar_in_room(num) : 0);
            }
            else if(iequals(field, "findmob")) {
                current = std::function(scriptFindMob);
            }
            else if(iequals(field, "findobj")) {
                current = std::function(scriptFindObj);
            }
            else if(iequals(field, "random")) {
                current = std::function(scriptRandom);
            }
            else if (iequals(field, "ctime"))
                current = fmt::format("{}", time(nullptr));
            else if (iequals(field, "door"))
                current = door[type];
            else if (iequals(field, "force"))
                current = force[type];
            else if (iequals(field, "load"))
                current = load[type];
            else if (iequals(field, "purge"))
                current = purge[type];
            else if (iequals(field, "teleport"))
                current = teleport[type];
            else if (iequals(field, "damage"))
                current = xdamage[type];
            else if (iequals(field, "send"))
                current = send_cmd[type];
            else if (iequals(field, "echo"))
                current = echo_cmd[type];
            else if (iequals(field, "echoaround"))
                current = echoaround_cmd[type];
            else if (iequals(field, "zoneecho"))
                current = zoneecho[type];
            else if (iequals(field, "asound"))
                current = asound[type];
            else if (iequals(field, "at"))
                current = at[type];
            else if (iequals(field, "transform"))
                current = transform[type];
            else if (iequals(field, "recho"))
                current = recho[type];
            else if(hasVar(field)) {
                auto res = getVar(field);
                if(res.index() == 0) {
                    current = std::get<0>(res);
                } else {
                    current = std::get<1>(res);
                }
            }
            continue;
        }
        switch(current.index()) {
            case 0: {
                // Strings. strings will invoke the string manipulation funcs.
                auto s = std::get<0>(current);
                current = scriptTextProcess(this, s, field, args);
                }
                break;
            case 1: {
                // a unit_data*.
                auto u = std::get<1>(current);
                // here we'll call the dgCallMember method and set the result into current...
                auto res = u->dgCallMember(this, field, args);
                if(res.index() == 0) {
                        current = std::get<0>(res);
                    } else {
                        current = std::get<1>(res);
                    }
                }
                break;
            case 2: {
                    // a function.
                    auto f = std::get<2>(current);
                    // here we'll call the function and set the result into current...
                    auto res = f(this, field, args);
                    if(res.index() == 0) {
                        current = std::get<0>(res);
                    } else {
                        current = std::get<1>(res);
                    }
                }
                break;
        }
    }

    switch(current.index()) {
        case 0: {
            return std::get<0>(current);
        }
        case 1: {
            return std::get<1>(current)->getUID(false);
        }
        case 2: {
            return "";
        }
    
    }
}


std::string trig_data::varSubst(const std::string& expr) {
    std::string out;
    std::string l = expr;
    std::size_t start = 0;

    while(start < l.length()) {
        std::size_t open = l.find('%', start);
        
        if (open == std::string::npos) {
            // No more opening % found, append the rest of the string to out.
            out += l.substr(start);
            break;
        } else {
            // Append the text before the opening % to out.
            out += l.substr(start, open - start);
        }
        
        std::size_t close = matching_percent(l, open);
        
        // If there's no matching closing %, we have a malformed input. Handle as needed.
        if (close == std::string::npos) {
            throw DgScriptException("No matching closing '%' found for the opening at position " + std::to_string(open));
        }
        
        // Extract the placeholder text and perform substitution.
        std::string placeholder = l.substr(open + 1, close - open - 1);
        out += handleSubst(placeholder);
        
        // Update the start position to the character after the closing %.
        start = close + 1;
    }

    return out;
}