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

/*
 * Thanks to James Long for his assistance in plugging the memory leak
 * that used to be here.   -- Welcor
 */
/* adds a variable with given name and value to trigger */
void add_var(struct trig_var_data **var_list, char *name, const char *value, long id) {
    struct trig_var_data *vd;

    if (strchr(name, '.')) {
        basic_mud_log("add_var() : Attempt to add illegal var: %s", name);
        return;
    }

    for (vd = *var_list; vd && strcasecmp(vd->name, name); vd = vd->next);

    if (vd && (!vd->context || vd->context == id)) {
        free(vd->value);
        CREATE(vd->value, char, strlen(value) + 1);
    } else {
        CREATE(vd, struct trig_var_data, 1);

        CREATE(vd->name, char, strlen(name) + 1);
        strcpy(vd->name, name);                            /* strcpy: ok*/

        CREATE(vd->value, char, strlen(value) + 1);

        vd->next = *var_list;
        vd->context = id;
        *var_list = vd;
    }
    if(vd->value) free(vd->value);
    vd->value = strdup(value);
}


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

std::optional<std::string> text_processed(char *field, char *subfield, struct trig_var_data *vd) {
    char *p, *p2;
    char tmpvar[MAX_STRING_LENGTH];

    if (iequals(field, "strlen")) {                     /* strlen    */
        char limit[200];
        return fmt::format("{}", strlen(vd->value));
    } else if (iequals(field, "trim")) {                /* trim      */
        /* trim whitespace from ends */
        snprintf(tmpvar, sizeof(tmpvar) - 1, "%s", vd->value); /* -1 to use later*/
        p = tmpvar;
        p2 = tmpvar + strlen(tmpvar) - 1;
        while (*p && isspace(*p)) p++;
        while ((p <= p2) && isspace(*p2)) p2--;
        if (p > p2) { /* nothing left */
            return "";
        }
        *(++p2) = '\0';                                         /* +1 ok (see above) */
        return p;
    } else if (iequals(field, "contains")) {            /* contains  */
        return str_str(vd->value, subfield) ? "1" : "0";
    } else if (iequals(field, "car")) {
        auto sp = split(vd->value, ' ');
        if(!sp.empty()) return sp[0];
        return "";
    } else if (iequals(field, "cdr")) {                 /* cdr       */
        char *cdr = vd->value;
        while (*cdr && !isspace(*cdr)) cdr++; /* skip 1st field */
        while (*cdr && isspace(*cdr)) cdr++;  /* skip to next */
        return cdr;
    } else if (iequals(field, "charat")) {              /* CharAt    */
        size_t len = strlen(vd->value), dgindex = atoi(subfield);
        if (dgindex > len || dgindex < 1)
            return "";
        else
            return fmt::format("{}", vd->value[dgindex - 1]);
    } else if (iequals(field, "mudcommand")) {
        /* find the mud command returned from this text */
/* NOTE: you may need to replace "cmd_info" with "complete_cmd_info", */
/* depending on what patches you've got applied.                      */

/* on older source bases:    extern struct command_info *cmd_info; */
        int length, cmd;
        for (length = strlen(vd->value), cmd = 0;
             *cmd_info[cmd].command != '\n'; cmd++)
            if (!strncmp(cmd_info[cmd].command, vd->value, length))
                break;

        if (*cmd_info[cmd].command == '\n')
            return "";
        else
            return fmt::format("{}", cmd_info[cmd].command);
    }

    return {};
}


/* sets str to be the value of var.field */
std::string find_replacement(trig_data *trig, char *var, char *field, char *subfield) {
    int type = trig->data_type;
    struct trig_var_data *vd = nullptr;
    char_data *ch, *c = nullptr, *rndm;
    obj_data *obj, *o = nullptr;
    struct room_data *room, *r = nullptr;
    char *name;
    int num, count, i, j, doors;

    for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
        if (iequals(vd->name, var))
            break;

    unit_data *u = nullptr;
    switch(trig->owner.index()) {
        case 0:
            u = std::get<0>(trig->owner);
            break;
        case 1:
            u = std::get<1>(trig->owner);
            break;
        case 2:
            u = std::get<2>(trig->owner);
            break;
    }

    if (!vd)
        for (vd = u->script->global_vars; vd; vd = vd->next)
            if (iequals(vd->name, var) && (vd->context == 0 || vd->context == u->script->context))
                break;

    if (!*field) {
        if (vd) return vd->value;
        else {
            if (iequals(var, "self")) {
                return u->getUID(false);
            } else if (iequals(var, "global")) {
                /* so "remote varname %global%" will work */
                return world[0].getUID(false);
            } else if (iequals(var, "ctime"))
                return fmt::format("{}", time(nullptr));
            else if (iequals(var, "door"))
                return door[type];
            else if (iequals(var, "force"))
                return force[type];
            else if (iequals(var, "load"))
                return load[type];
            else if (iequals(var, "purge"))
                return purge[type];
            else if (iequals(var, "teleport"))
                return teleport[type];
            else if (iequals(var, "damage"))
                return xdamage[type];
            else if (iequals(var, "send"))
                return send_cmd[type];
            else if (iequals(var, "echo"))
                return echo_cmd[type];
            else if (iequals(var, "echoaround"))
                return echoaround_cmd[type];
            else if (iequals(var, "zoneecho"))
                return zoneecho[type];
            else if (iequals(var, "asound"))
                return asound[type];
            else if (iequals(var, "at"))
                return at[type];
            else if (iequals(var, "transform"))
                return transform[type];
            else if (iequals(var, "recho"))
                return recho[type];
            else
                return "";
        }
        return;
    } else {
        if (vd) {
            name = vd->value;

            switch (type) {
                case MOB_TRIGGER:
                    ch = (char_data *)u;

                    if ((o = get_object_in_equip(ch, name)));
                    else if ((o = get_obj_in_list(name, ch->contents)));
                    else if (IN_ROOM(ch) != NOWHERE && (c = get_char_in_room(ch->getRoom(), name)));
                    else if ((o = get_obj_in_list(name, ch->getRoom()->contents)));
                    else if ((c = get_char(name)));
                    else if ((o = get_obj(name)));
                    else if ((r = get_room(name))) {}

                    break;
                case OBJ_TRIGGER:
                    obj = (obj_data *)u;

                    if ((c = get_char_by_obj(obj, name)));
                    else if ((o = get_obj_by_obj(obj, name)));
                    else if ((r = get_room(name))) {}

                    break;
                case WLD_TRIGGER:
                    room = (room_data *)u;

                    if ((c = get_char_by_room(room, name)));
                    else if ((o = get_obj_by_room(room, name)));
                    else if ((r = get_room(name))) {}

                    break;
            }
        } else {
            if (iequals(var, "self")) {
                c = nullptr;
                r = nullptr;
                o = nullptr;
                switch (type) {
                    case MOB_TRIGGER:
                        c = (char_data *)u;
                        break;     /* the room.  - Welcor        */
                    case OBJ_TRIGGER:
                        o = (obj_data *)u;
                        break;
                    case WLD_TRIGGER:
                        r = (struct room_data *)u;
                        break;
                }
            } else if (iequals(var, "global")) {
                auto thescript = SCRIPT(&world[0]);
                if (!thescript) {
                    script_log("Attempt to find global var. Apparently the void has no script.");
                    return;
                }
                for (vd = thescript->global_vars; vd; vd = vd->next)
                    if (iequals(vd->name, field))
                        break;

                if (vd) return vd->value;

                return;
            } else if (iequals(var, "people")) {
                return fmt::format("{}", ((num = atoi(field)) > 0) ? trgvar_in_room(num) : 0);
            } else if (iequals(var, "time")) {
                if (iequals(field, "hour"))
                    return fmt::format("{}", time_info.hours);
                else if(iequals(field, "minute"))
                    return fmt::format("{}", time_info.minutes);
                else if(iequals(field, "second"))
                    return fmt::format("{}", time_info.seconds);
                else if (iequals(field, "day"))
                    return fmt::format("{}", time_info.day + 1);
                else if (iequals(field, "month"))
                    return fmt::format("{}", time_info.month + 1);
                else if (iequals(field, "year"))
                    return fmt::format("{}", time_info.year);
                else return "";
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
            else if (iequals(var, "findmob")) {
                if (!field || !*field || !subfield || !*subfield) {
                    script_log("findmob.vnum(mvnum) - illegal syntax");
                    return "0";
                } else {
                    room_rnum rrnum = real_room(atof(field));
                    mob_vnum mvnum = atof(subfield);

                    if (rrnum == NOWHERE) {
                        script_log("findmob.vnum(ovnum): No room with vnum %d", atof(field));
                        return "0";
                    } else {
                        for (i = 0, ch = world[rrnum].people; ch; ch = ch->next_in_room)
                            if (GET_MOB_VNUM(ch) == mvnum)
                                i++;
                        return fmt::format("{}", i);
                    }
                }
            }
                /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
            else if (iequals(var, "findobj")) {
                if (!field || !*field || !subfield || !*subfield) {
                    script_log("findobj.vnum(ovnum) - illegal syntax");
                    return "0";
                } else {
                    room_rnum rrnum = real_room(atof(field));

                    if (rrnum == NOWHERE) {
                        script_log("findobj.vnum(ovnum): No room with vnum %d", atof(field));
                        return "0";
                    } else {
                        /* item_in_list looks within containers as well. */
                        return fmt::format("{}", item_in_list(subfield, world[rrnum].contents));
                    }
                }
            } else if (iequals(var, "random")) {
                if (iequals(field, "char")) {
                    rndm = nullptr;
                    count = 0;

                    if (type == MOB_TRIGGER) {
                        ch = (char_data *)u;
                        for (c = ch->getRoom()->people; c; c = c->next_in_room)
                            if ((c != ch) && valid_dg_target(c, DG_ALLOW_GODS) &&
                                CAN_SEE(ch, c)) {
                                if (!rand_number(0, count))
                                    rndm = c;
                                count++;
                            }
                    } else if (type == OBJ_TRIGGER) {
                        for (c = world[obj_room((obj_data *)u)].people; c;
                             c = c->next_in_room)
                            if (valid_dg_target(c, DG_ALLOW_GODS)) {
                                if (!rand_number(0, count))
                                    rndm = c;
                                count++;
                            }
                    } else if (type == WLD_TRIGGER) {
                        for (c = ((struct room_data *)u)->people; c;
                             c = c->next_in_room)
                            if (valid_dg_target(c, DG_ALLOW_GODS)) {

                                if (!rand_number(0, count))
                                    rndm = c;
                                count++;
                            }
                    }
                    return rndm ? fmt::format("{}", ((rndm)->getUID(false).c_str())) : "0";
                } else if (iequals(field, "dir")) {
                    room_rnum in_room = NOWHERE;

                    switch (type) {
                        case WLD_TRIGGER:
                            in_room = real_room(((struct room_data *)u)->vn);
                            break;
                        case OBJ_TRIGGER:
                            in_room = obj_room((struct obj_data *)u);
                            break;
                        case MOB_TRIGGER:
                            in_room = IN_ROOM((struct char_data *)u);
                            break;
                    }
                    if (in_room == NOWHERE) {
                        return "";
                    } else {
                        std::vector<int> available;
                        room = &world[in_room];
                        for (i = 0; i < NUM_OF_DIRS; i++)
                            if (R_EXIT(room, i))
                                available.push_back(i);

                        if (available.empty()) {
                            return "";
                        } else {
                            auto dir = Random::get(available);
                            return dirs[*dir];
                        }
                    }
                } else
                    return fmt::format("{}", ((num = atoi(field)) > 0) ? rand_number(1, num) : 0);

                return;
            }
        }

        if (auto res = text_processed(field, subfield, vd); res) {
            return res.value();
        }

        /* set str to some 'non-text' first */
        if(u) {
            if(auto result = u->dgCallMember(trig, field, subfield ? subfield : ""); result) {
                return result.value();
            }
        }
        return "";
    }
}

std::size_t matching_percent(const std::string& line, std::size_t start) {
    int depth;

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

DgResults scriptStartHolder(trig_data* trig, const std::string& field, const std::string& args) {
    auto type = trig->parent->data_type;

    if(iequals(field, "self")) {
        return trig->sc->owner;
    }
    else if(iequals(field, "time")) {
        return std::function(scriptTimeHolder);
    }
    else if (iequals(field, "global")) {
        /* so "remote varname %global%" will work */
        return world[0].getUID(false);
    } 
    else if (iequals(field, "ctime"))
        return fmt::format("{}", time(nullptr));
    else if (iequals(field, "door"))
        return door[type];
    else if (iequals(field, "force"))
        return force[type];
    else if (iequals(field, "load"))
        return load[type];
    else if (iequals(field, "purge"))
        return purge[type];
    else if (iequals(field, "teleport"))
        return teleport[type];
    else if (iequals(field, "damage"))
        return xdamage[type];
    else if (iequals(field, "send"))
        return send_cmd[type];
    else if (iequals(field, "echo"))
        return echo_cmd[type];
    else if (iequals(field, "echoaround"))
        return echoaround_cmd[type];
    else if (iequals(field, "zoneecho"))
        return zoneecho[type];
    else if (iequals(field, "asound"))
        return asound[type];
    else if (iequals(field, "at"))
        return at[type];
    else if (iequals(field, "transform"))
        return transform[type];
    else if (iequals(field, "recho"))
        return recho[type];


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

std::vector<std::pair<std::string, std::string>> splitFields(const std::string& line) {
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
                // TODO: probably need to do some recursive shit here on args.
            }
        }
        out.emplace_back(field, args);
    }
    

    return out;
}


std::string trig_data::handleSubst(const std::string& expr) {
    auto type = parent->data_type;
    DgHolder current = "";
    int i = 0;

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
                /* so "remote varname %global%" will work */
                current = &world[0];
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

            continue;
        }
        switch(current.index()) {
            case 0: {
                // Strings. strings will invoke the string manipulation funcs.
                auto s = std::get<0>(current);
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


std::string trig_data::varSubst(const std::string& line) {
    std::string out;
    std::string l = line;

    std::size_t start = l.find('%');

    while(start != std::string::npos) {
        auto end = matching_percent(l, start);

        auto sub = l.substr(start+1, line.size()-end);
        out += handleSubst(sub);
        l = l.substr(end+1);

        start = l.find('%');
    }

    out += l;

    return out;
}

