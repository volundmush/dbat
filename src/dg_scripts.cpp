/**************************************************************************
*  File: dg_scripts.c                                                     *
*  Usage: contains the main script driver interface.                      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <filesystem>
#include <charconv>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <regex>

#include "dbat/dg_scripts.h"
#include "dbat/act.wizard.h"
#include "dbat/dg_event.h"
#include "dbat/send.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/constants.h"
#include "dbat/comm.h"
#include "dbat/players.h"

#define PULSES_PER_MUD_HOUR     (SECS_PER_MUD_HOUR*PASSES_PER_SEC)


/* Local functions not used elsewhere */
void do_stat_trigger(struct char_data *ch, trig_proto_data *trig);

void script_stat(char_data *ch, script_data *sc);

int remove_trigger(script_data *sc, char *name);

bool is_num(const std::string &arg);

void eval_op(char *op, char *lhs, char *rhs, char *result, unit_data *go,
             script_data *sc, trig_data *trig);

char *matching_paren(char *p);

void eval_expr(char *line, char *result, unit_data *go, script_data *sc,
               trig_data *trig, UnitType type);

int eval_lhs_op_rhs(char *expr, char *result, unit_data *go, script_data *sc,
                    trig_data *trig, UnitType type);

void process_wait(unit_data *go, trig_data *trig, UnitType type, char *cmd,
                  struct cmdlist_element *cl);

void process_set(script_data *sc, trig_data *trig, char *cmd);

void process_attach(unit_data *go, script_data *sc, trig_data *trig,
                    UnitType type, char *cmd);

void process_detach(unit_data *go, script_data *sc, trig_data *trig,
                    UnitType type, char *cmd);

int process_return(trig_data *trig, char *cmd);

void process_unset(script_data *sc, trig_data *trig, char *cmd);

void process_remote(script_data *sc, trig_data *trig, char *cmd);

void process_rdelete(script_data *sc, trig_data *trig, char *cmd);

void process_global(script_data *sc, trig_data *trig, char *cmd, long id);

void process_context(script_data *sc, trig_data *trig, char *cmd);

void extract_value(script_data *sc, trig_data *trig, char *cmd);

void dg_letter_value(script_data *sc, trig_data *trig, char *cmd);

struct cmdlist_element *
find_case(struct trig_data *trig, struct cmdlist_element *cl,
    unit_data *go, script_data *sc, UnitType type, char *cond);

struct cmdlist_element *find_done(struct cmdlist_element *cl);

int fgetline(FILE *file, char *p);

ACMD(do_attach);

ACMD(do_detach);

ACMD(do_vdelete);

ACMD(do_tstat);

/* Return pointer to first occurrence of string ct in */
/* cs, or nullptr if not present.  Case insensitive */
char *str_str(char *cs, char *ct) {
    char *s, *t;

    if (!cs || !ct || !*ct)
        return nullptr;

    while (*cs) {
        t = ct;

        while (*cs && (LOWER(*cs) != LOWER(*t)))
            cs++;

        s = cs;

        while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
            t++;
            cs++;
        }

        if (!*t)
            return s;
    }

    return nullptr;
}


int trgvar_in_room(room_vnum vnum) {
    room_rnum rnum = real_room(vnum);
    int i = 0;
    char_data *ch;

    if (rnum == NOWHERE) {
        script_log("people.vnum: world[rnum] does not exist");
        return (-1);
    }

    return get_room(rnum)->getPeople().size();
}

obj_data *get_obj_in_list(char *name, const std::vector<std::weak_ptr<obj_data>>& list) {
    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);;
        if(!uidResult) return nullptr;
        auto obj2 = std::dynamic_pointer_cast<obj_data>(uidResult);
        if(!obj2) return nullptr;
        auto obj = obj2.get();

        for (auto i : filter_raw(list))
            if(i == obj) return obj;
    } else {
        for (auto i : filter_raw(list))
            if (isname(name, i->getName()))
                return i;
    }

    return nullptr;
}

obj_data *get_object_in_equip(char_data *ch, char *name) {
    int j, n = 0, number;
    obj_data *obj;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp = tmpname;
    int32_t id;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        if(!uidResult) return nullptr;
        auto obj2 = std::dynamic_pointer_cast<obj_data>(uidResult);
        if(!obj2) return nullptr;
        auto o = obj2.get();

        for (j = 0; j < NUM_WEARS; j++)
            if ((obj = GET_EQ(ch, j)))
                if (o == obj) return obj;
    } else if (is_number(name)) {
        obj_vnum ovnum = atoi(name);
        for (j = 0; j < NUM_WEARS; j++)
            if ((obj = GET_EQ(ch, j)))
                if (GET_OBJ_VNUM(obj) == ovnum)
                    return (obj);
    } else {
        snprintf(tmpname, sizeof(tmpname), "%s", name);
        if (!(number = get_number(&tmp)))
            return nullptr;

        for (j = 0; (j < NUM_WEARS) && (n <= number); j++)
            if ((obj = GET_EQ(ch, j)))
                if (isname(tmp, obj->getName()))
                    if (++n == number)
                        return (obj);
    }

    return nullptr;
}

static struct eq_pos_list {
    const char *pos;
    int where;
} eq_pos[] = {
        {"hold",      WEAR_WIELD2},
        {"held",      WEAR_WIELD2},
        {"light",     WEAR_WIELD2},
        {"wield",     WEAR_WIELD1},
        {"rfinger",   WEAR_FINGER_R},
        {"lfinger",   WEAR_FINGER_L},
        {"neck1",     WEAR_NECK_1},
        {"neck2",     WEAR_NECK_2},
        {"body",      WEAR_BODY},
        {"head",      WEAR_HEAD},
        {"legs",      WEAR_LEGS},
        {"feet",      WEAR_FEET},
        {"hands",     WEAR_HANDS},
        {"arms",      WEAR_ARMS},
        {"shield",    WEAR_WIELD2},
        {"about",     WEAR_ABOUT},
        {"waist",     WEAR_WAIST},
        {"rwrist",    WEAR_WRIST_R},
        {"lwrist",    WEAR_WRIST_L},
        {"backpack",  WEAR_BACKPACK},
        {"rear",      WEAR_EAR_R},
        {"lear",      WEAR_EAR_L},
        {"shoulders", WEAR_SH},
        {"scouter",   WEAR_EYE},
        {"none", -1}
};

/* Handles 'held', 'light' and 'wield' positions - Welcor
   After idea from Byron Ellacott - bje@apnic.net */
int find_eq_pos_script(char *arg) {
    int i;

    if (is_number(arg) && (i = atoi(arg)) >= 0 && i < NUM_WEARS)
        return i;

    for (i = 0; eq_pos[i].where != -1; i++) {
        if (!strcasecmp(eq_pos[i].pos, arg))
            return eq_pos[i].where;
    }
    return (-1);
}

int can_wear_on_pos(struct obj_data *obj, int pos) {
    switch (pos) {
        case WEAR_WIELD1:
            return CAN_WEAR(obj, ITEM_WEAR_WIELD);
        case WEAR_WIELD2:
            return CAN_WEAR(obj, ITEM_WEAR_TAKE);
        case WEAR_FINGER_R:
        case WEAR_FINGER_L:
            return CAN_WEAR(obj, ITEM_WEAR_FINGER);
        case WEAR_NECK_1:
        case WEAR_NECK_2:
            return CAN_WEAR(obj, ITEM_WEAR_NECK);
        case WEAR_BODY:
            return CAN_WEAR(obj, ITEM_WEAR_BODY);
        case WEAR_HEAD:
            return CAN_WEAR(obj, ITEM_WEAR_HEAD);
        case WEAR_LEGS:
            return CAN_WEAR(obj, ITEM_WEAR_LEGS);
        case WEAR_FEET:
            return CAN_WEAR(obj, ITEM_WEAR_FEET);
        case WEAR_HANDS:
            return CAN_WEAR(obj, ITEM_WEAR_HANDS);
        case WEAR_ARMS:
            return CAN_WEAR(obj, ITEM_WEAR_ARMS);
        case WEAR_ABOUT:
            return CAN_WEAR(obj, ITEM_WEAR_ABOUT);
        case WEAR_WAIST:
            return CAN_WEAR(obj, ITEM_WEAR_WAIST);
        case WEAR_WRIST_R:
        case WEAR_WRIST_L:
            return CAN_WEAR(obj, ITEM_WEAR_WRIST);
        case WEAR_BACKPACK:
            return CAN_WEAR(obj, ITEM_WEAR_PACK);
        case WEAR_EAR_R:
        case WEAR_EAR_L:
            return CAN_WEAR(obj, ITEM_WEAR_EAR);
        case WEAR_SH:
            return CAN_WEAR(obj, ITEM_WEAR_SH);
        case WEAR_EYE:
            return CAN_WEAR(obj, ITEM_WEAR_EYE);
        default:
            return false;
    }
}

/************************************************************
 * generic searches based only on name
 ************************************************************/

/* search the entire world for a char, and return a pointer */
char_data *get_char(char *name) {
    char_data *i;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        i = std::dynamic_pointer_cast<char_data>(uidResult).get();
        if(!i) return nullptr;

        if (i && valid_dg_target(i, DG_ALLOW_GODS))
            return i;
    } else {
        for (const auto& r : getAllCharacters()) {
            i = r.lock().get();
            if(!i) continue;
            if (isname(name, i->getName()) &&
                valid_dg_target(i, DG_ALLOW_GODS))
                return i;
        }
    }

    return nullptr;
}

/*
 * Finds a char in the same room as the object with the name 'name'
 */
char_data *get_char_near_obj(obj_data *obj, char *name) {
    char_data *ch;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        ch = std::dynamic_pointer_cast<char_data>(uidResult).get();
        if(!ch) return nullptr;

        if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
            return ch;
    } else {
        room_rnum num;
        if ((num = obj_room(obj)) != NOWHERE) {
            auto people = get_room(num)->getPeople();
            for (auto ch : filter_raw(people))
                if (isname(name, ch->getName()) &&
                    valid_dg_target(ch, DG_ALLOW_GODS))
                    return ch;
        }
            
    }

    return nullptr;
}


/*
 * returns a pointer to the first character in world by name name,
 * or nullptr if none found.  Starts searching in room room first
 */
char_data *get_char_in_room(room_data *room, char *name) {
    char_data *ch;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        ch = std::dynamic_pointer_cast<char_data>(uidResult).get();
        if(!ch) return nullptr;

        if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
            return ch;
    } else {
        auto people = room->getPeople();
        for (auto ch : filter_raw(people))
            if (isname(name, ch->getName()) &&
                valid_dg_target(ch, DG_ALLOW_GODS))
                return ch;
    }

    return nullptr;
}

/* searches the room with the object for an object with name 'name'*/

obj_data *get_obj_near_obj(obj_data *obj, char *name) {
    obj_data *i = nullptr;
    char_data *ch;
    room_vnum rm;
    int32_t id;

    if (!strcasecmp(name, "self") || !strcasecmp(name, "me"))
        return obj;

    /* is it inside ? */
    if ((i = get_obj_in_list(name, obj->getObjects())))
        return i;

    /* or outside ? */
    if(!obj->location.unit)
        return nullptr;
    
    switch(obj->location.getType()) {
        case UnitType::object: {
            auto o = static_cast<obj_data*>(obj->location.unit);
            if (*name == UID_CHAR) {
                auto uidResult = resolveUID(name);
                auto o2 = std::dynamic_pointer_cast<obj_data>(uidResult).get();
                if(!o2) return nullptr;
                if(o2 == o) return o;
            } else if (isname(name, o->getName()))
                return o;
            break;
        }
        case UnitType::character: {
            auto c = static_cast<char_data*>(obj->location.unit);
            if(obj->location.position.x >= 0.0 && (i = get_object_in_equip(c, name))) {
                // worn?
                return i;
            } else {
                if(i = get_obj_in_list(name, c->getObjects())) {
                    // carried?
                    return i;
                }
            }
        }
        case UnitType::room: {
            rm = obj_room(obj);
            break;
        }
        default:
            return nullptr;
    }

    if ((rm = obj_room(obj)) != NOWHERE) {
        /* check the floor */
        if ((i = get_obj_in_list(name, get_room(rm)->getObjects())))
            return i;

        /* check peoples' inventory */
        auto people = get_room(rm)->getPeople();
        for (auto ch : filter_raw(people))
            if ((i = get_object_in_equip(ch, name)))
                return i;
    }
    return nullptr;
}

/* returns the object in the world with name name, or nullptr if not found */
obj_data *get_obj(char *name) {
    obj_data *obj;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        return std::dynamic_pointer_cast<obj_data>(uidResult).get();
    }
    else {
        auto ao = objectSubscriptions.all("active");
        for (auto obj : filter_raw(ao)) {
            if (isname(name, obj->getName()))
                return obj;
        }
    }

    return nullptr;
}


/* finds room by id or vnum.  returns nullptr if not found */
room_data *get_room(char *name) {
    room_rnum nr;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        return std::dynamic_pointer_cast<room_data>(uidResult).get();
    }
    else if ((nr = real_room(atoi(name))) == NOWHERE)
        return nullptr;
    else
        return get_room(nr);
}


/*
 * returns a pointer to the first character in world by name name,
 * or nullptr if none found.  Starts searching with the person owing the object
 */
char_data *get_char_by_obj(obj_data *obj, char *name) {
    char_data *ch;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        ch = std::dynamic_pointer_cast<char_data>(uidResult).get();
        if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
            return ch;
    } else {
        if (auto carrier = obj->getCarriedBy(); carrier && isname(name, carrier->getName()) && valid_dg_target(carrier, DG_ALLOW_GODS))
            return carrier;

        if (auto wearer = obj->getWornBy(); wearer && isname(name, wearer->getName()) && valid_dg_target(wearer, DG_ALLOW_GODS))
            return wearer;

        auto ac = characterSubscriptions.all("active");
        for (auto ch : filter_raw(ac)) {
            if (isname(name, ch->getName()) &&
                valid_dg_target(ch, DG_ALLOW_GODS))
                return ch;
        }
    }

    return nullptr;
}


/*
 * returns a pointer to the first character in world by name name,
 * or nullptr if none found.  Starts searching in room room first
 */
char_data *get_char_by_room(room_data *room, char *name) {
    char_data *ch;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        ch = std::dynamic_pointer_cast<char_data>(uidResult).get();

        if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
            return ch;
    } else {
        auto people = room->getPeople();
        for (auto ch : filter_raw(people))
            if (isname(name, ch->getName()) &&
                valid_dg_target(ch, DG_ALLOW_GODS))
                return ch;

        auto ac = characterSubscriptions.all("active");
        for (auto ch : filter_raw(ac)) {
            if (isname(name, ch->getName()) &&
                valid_dg_target(ch, DG_ALLOW_GODS))
                return ch;
        }
    }

    return nullptr;
}


/*
 * returns the object in the world with name name, or nullptr if not found
 * search based on obj
 */
obj_data *get_obj_by_obj(obj_data *obj, char *name) {
    obj_data *i = nullptr;
    room_vnum rm;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        return std::dynamic_pointer_cast<obj_data>(uidResult).get();
    }

    if (!strcasecmp(name, "self") || !strcasecmp(name, "me"))
        return obj;

    if (i = get_obj_in_list(name, obj->getObjects()))
        return i;

    if(!obj->location.unit)
        return nullptr;
    
    switch(obj->location.getType()) {
        case UnitType::object: {
            auto o = static_cast<obj_data*>(obj->location.unit);
            if(isname(name, o->getName()))
                return o;
        }
            break;
        case UnitType::character: {
            auto c = static_cast<char_data*>(obj->location.unit);
            if(obj->location.position.x == -1) {
                if(i = get_obj_in_list(name,c->getObjects()); i)
                    return i;
            } else {
                if (i = get_object_in_equip(c, name); i)
                    return i;
            }
        }
            break;
        default:
            break;
    }

    if (((rm = obj_room(obj)) != NOWHERE) &&
        (i = get_obj_in_list(name, get_room(rm)->getObjects())))
        return i;

    return get_obj(name);
}

/* only searches the room */
obj_data *get_obj_in_room(room_data *room, char *name) {
    obj_data *obj;
    int32_t id;

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        auto o = std::dynamic_pointer_cast<obj_data>(uidResult).get();
        if(!o) return nullptr;
        auto con = room->getObjects();
        for (auto obj : filter_raw(con))
            if (o == obj)
                return obj;
    } else {
        auto con = room->getObjects();
        for (auto obj : filter_raw(con))
            if (isname(name, obj->getName()))
                return obj;
    }

    return nullptr;
}

/* returns obj with name - searches room, then world */
obj_data *get_obj_by_room(room_data *room, char *name) {

    if (*name == UID_CHAR) {
        auto uidResult = resolveUID(name);
        return std::dynamic_pointer_cast<obj_data>(uidResult).get();
    }

    auto con = room->getObjects();
    for (auto obj : filter_raw(con))
        if (isname(name, obj->getName()))
            return obj;

    auto ao = objectSubscriptions.all("active");
    for (auto obj : filter_raw(ao)) {
        if (isname(name, obj->getName()))
            return obj;
    }

    return nullptr;
}

/* checks every PULSE_SCRIPT for random triggers */
void script_trigger_check(uint64_t heartPulse, double deltaTime) {
    int nr;
    script_data *sc;

    auto crandsubs = characterSubscriptions.all("randomTriggers");
    for (auto ch : filter_raw(crandsubs)) {
        sc = SCRIPT(ch);

        if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
            (!is_empty(ch->location.getZone()->number) ||
                IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
            random_mtrigger(ch);
    }

    auto orandsubs = objectSubscriptions.all("randomTriggers");
    for (auto obj : filter_raw(orandsubs)) {
        sc = SCRIPT(obj);

        if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
            random_otrigger(obj);
    }

    auto rrandsubs = roomSubscriptions.all("randomTriggers");
    for (auto room : filter_raw(rrandsubs)) {
        sc = SCRIPT(room);

        if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
            (!is_empty(room->zone->number) ||
                IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
            random_wtrigger(room);
    }
}

void check_time_triggers() {
    int nr;
    script_data *sc;

    auto ctimesubs = characterSubscriptions.all("timeTriggers");
    for (auto ch : filter_raw(ctimesubs)) {
        sc = SCRIPT(ch);

        if (IS_SET(SCRIPT_TYPES(sc), MTRIG_TIME) &&
            (!is_empty(ch->location.getZone()->number) ||
                IS_SET(SCRIPT_TYPES(sc), MTRIG_GLOBAL)))
            time_mtrigger(ch);
    }

    auto otimesubs = objectSubscriptions.all("timeTriggers");
    for (auto obj : filter_raw(otimesubs)) {
        sc = SCRIPT(obj);

        if (IS_SET(SCRIPT_TYPES(sc), OTRIG_TIME))
            time_otrigger(obj);
    }

    auto rtimesubs = roomSubscriptions.all("timeTriggers");
    for (auto room : filter_raw(rtimesubs)) {
        sc = SCRIPT(room);

        if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIME) &&
            (!is_empty(room->zone->number) ||
                IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
            time_wtrigger(room);
    }
}

void check_interval_triggers(int trigFlag) {
    auto ac = characterSubscriptions.all("active");
    for (auto ch : filter_raw(ac)) {
        auto sc = SCRIPT(ch);

        if (IS_SET(SCRIPT_TYPES(sc), trigFlag) &&
            (!is_empty(ch->location.getZone()->number) ||
                IS_SET(SCRIPT_TYPES(sc), MTRIG_GLOBAL)))
            interval_mtrigger(ch, trigFlag);
    }

    auto ao = objectSubscriptions.all("active");
    for (auto obj : filter_raw(ao)) {
        auto sc = SCRIPT(obj);

        if (IS_SET(SCRIPT_TYPES(sc), trigFlag))
            interval_otrigger(obj, trigFlag);
    }

    for (auto &[vn, r] : world) {
        auto sc = SCRIPT(r);

        if (IS_SET(SCRIPT_TYPES(sc), trigFlag) &&
            (!is_empty(r->zone->number) ||
                IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
            interval_wtrigger(r.get(), trigFlag);
    }
}

void trig_proto_data::setBody(const std::string& body) {
    std::vector<std::string> l;
    boost::split_regex(l, body, boost::regex("\r\n|\r|\n"));
    lines = parse_script(l);
}

std::string trig_proto_data::scriptString() const {
    std::string out;

    int depth = 0;
    int i = 0;
    
    for(const auto &cl : lines) {
        std::string line;
        int predepthmod = 0;
        int postdepthmod = 0;
        switch(std::get<0>(cl)) {
            case ScriptLineType::COMMAND:
                line = std::get<1>(cl);
                break;
            case ScriptLineType::IF:
                line = "if " + std::get<1>(cl);
                postdepthmod = 1;
                break;
            case ScriptLineType::ELSE:
                line = "else";
                predepthmod = -1;
                postdepthmod = 1;
                break;
            case ScriptLineType::ELSEIF:
                line = "elseif " + std::get<1>(cl);
                predepthmod = -1;
                postdepthmod = 1;
                break;
            case ScriptLineType::END:
                line = "end";
                predepthmod = -1;
                break;
            case ScriptLineType::WHILE:
                line = "while " + std::get<1>(cl);
                postdepthmod = 1;
                break;
            case ScriptLineType::SWITCH:
                line = "switch " + std::get<1>(cl);
                postdepthmod = 1;
                break;
            case ScriptLineType::CASE:
                line = "case " + std::get<1>(cl);
                predepthmod = -1;
                postdepthmod = 1;
                break;
            case ScriptLineType::BREAK:
                line = "break";
                break;
            case ScriptLineType::DEFAULT:
                line = "default";
                predepthmod = -1;
                postdepthmod = 1;
                break;
            case ScriptLineType::DONE:
                line = "done";
                predepthmod = -1;
                break;
            case ScriptLineType::COMMENT:
                line = "* " + std::get<1>(cl);
                break;
            default:
                basic_mud_log("SYSERR: Unknown script line type in scriptString: %d", static_cast<int>(std::get<0>(cl)));
                continue;
        }
        if(predepthmod) depth = std::max<int>(0, depth + predepthmod);
        out += std::string(depth * 2, ' ') + line + "\r\n";
        if(postdepthmod) depth = std::max<int>(0, depth + postdepthmod);
        i++;
    }

    return out;
}


void do_stat_trigger(struct char_data *ch, trig_proto_data *trig) {
    char buf[MAX_STRING_LENGTH];
    if (!trig) {
        basic_mud_log("SYSERR: nullptr trigger passed to do_stat_trigger.");
        return;
    }

    std::string out;

    out += "Name: '@y" + trig->name + "@n',  VNum: [@g" + std::to_string(trig->vn) + "@n]\r\n";

    if (trig->attach_type == OBJ_TRIGGER) {
        out += "Trigger Intended Assignment: Objects\r\n";
        sprintbit(trig->trigger_type, otrig_types, buf, sizeof(buf));
    } else if (trig->attach_type == WLD_TRIGGER) {
        out += "Trigger Intended Assignment: Rooms\r\n";
        sprintbit(trig->trigger_type, wtrig_types, buf, sizeof(buf));
    } else {
        out += "Trigger Intended Assignment: Mobiles\r\n";
        sprintbit(trig->trigger_type, trig_types, buf, sizeof(buf));
    }

    out += "Trigger Type: " + std::string(buf) + ", Numeric Arg: " + std::to_string(trig->narg) + ", Arg list: " +
           (!trig->arglist.empty() ? trig->arglist.c_str() : "None") + "\r\n";

    out += "Script Body:\r\n";
    out += trig->scriptString();

    ch->desc->sendText(out);
}


/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name, size_t nlen) {
    char_data *ch;
    obj_data *obj;

    if ((ch = get_char(uid)))
        snprintf(name, nlen, "%s", ch->getName());
    else if ((obj = get_obj(uid)))
        snprintf(name, nlen, "%s", obj->getName());
    else
        snprintf(name, nlen, "uid = %s, (not found)", uid + 1);
}


/* general function to display stats on script sc */
void script_stat(char_data *ch, script_data *sc) {
    struct trig_var_data *tv;
    char name[MAX_INPUT_LENGTH];
    char namebuf[512];
    char buf1[MAX_STRING_LENGTH];

        ch->send_to("Global Variables: %s\r\n", !sc->variables.empty() ? "" : "None");

    for (const auto& [name, value] : sc->variables) {
        if (value[0] == UID_CHAR) {
            auto uidResult = resolveUID(value);
            if(uidResult) {
                std::string n = uidResult->getName();
                                ch->send_to("    %15s:  %s\r\n", name.c_str(), n.c_str());
            } else {
                                ch->send_to("   -BAD UID: %s", value.c_str());
            }
        } else
                        ch->send_to("    %15s:  %s\r\n", name.c_str(), value.c_str());
    }

    auto scripts = sc->getScripts();
    for (const auto& t : filter_shared(scripts)) {
                ch->send_to("\r\n  Trigger: @y%s@n, VNum: [@y%5d@n], RNum: [%5d]\r\n", GET_TRIG_NAME(t), GET_TRIG_VNUM(t), GET_TRIG_RNUM(t));

        if (t->getAttachType() == OBJ_TRIGGER) {
                        ch->sendText("  Trigger Intended Assignment: Objects\r\n");
            sprintbit(GET_TRIG_TYPE(t), otrig_types, buf1, sizeof(buf1));
        } else if (t->getAttachType() == WLD_TRIGGER) {
                        ch->sendText("  Trigger Intended Assignment: Rooms\r\n");
            sprintbit(GET_TRIG_TYPE(t), wtrig_types, buf1, sizeof(buf1));
        } else {
                        ch->sendText("  Trigger Intended Assignment: Mobiles\r\n");
            sprintbit(GET_TRIG_TYPE(t), trig_types, buf1, sizeof(buf1));
        }

                ch->send_to("  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n", buf1, GET_TRIG_NARG(t), ((GET_TRIG_ARG(t) && *GET_TRIG_ARG(t)) ? GET_TRIG_ARG(t) :
                      "None"));

        if (t->waiting != 0.0) {
                        ch->send_to("    Wait: %f seconds, Current line: %d\r\n", t->waiting, t->current_line);
                        ch->send_to("  Variables: %s\r\n", t->variables.empty() ? "None" : "");

            for (const auto &[key, value] : t->variables) {
                if (value.starts_with(UID_CHAR)) {
                    auto uidResult = resolveUID(value);
                    if(uidResult) {
                        std::string n = uidResult->getName();
                                                ch->send_to("    %15s:  %s\r\n", key.c_str(), n.c_str());
                    } else {
                                                ch->send_to("   -BAD UID: %s", value.c_str());
                    }
                } else {
                                        ch->send_to("    %15s:  %s\r\n", key.c_str(), value.c_str());
                }

            }
        }
    }
}


void do_sstat(struct char_data *ch, struct unit_data *ud) {
        ch->sendText("Triggers:\r\n");
    if (!SCRIPT(ud)) {
                ch->sendText("  None.\r\n");
        return;
    }

    script_stat(ch, SCRIPT(ud));
}



/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(script_data *sc, const std::shared_ptr<trig_data> t, int loc) {

    auto tvn = t->getVnum();

    // if the sc (unit_data) isn't already using running_scripts...
    if(!sc->running_scripts.has_value()) {
        sc->running_scripts.emplace(sc->getProtoScript());
    }

    auto &scr = sc->running_scripts.value();

    // Now insert t in the right spot...
    if(loc == -1) {
        scr.emplace_back(tvn);
    } else if(loc == 0) {
        // Insert at the beginning
        scr.insert(scr.begin(), tvn);
    } else if(loc > 0 && loc <= scr.size()) {
        // Insert at the specified location
        scr.insert(scr.begin() + loc - 1, tvn);
    } else {
        // If loc is invalid, just append it
        scr.emplace_back(tvn);
    }

    SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);

    sc->scripts.emplace(tvn, t);
    t->owner = units.at(sc->id).get();
    t->activate();
}


ACMD(do_attach) {
    char_data *victim;
    obj_data *object;
    room_data *room;
    std::shared_ptr<trig_data> trig;
    char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
    char loc_name[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
    int loc, tn, rn, num_arg;
    room_rnum rnum;

    argument = two_arguments(argument, arg, trig_name);
    two_arguments(argument, targ_name, loc_name);

    if (!*arg || !*targ_name || !*trig_name) {
                ch->sendText("Usage: attach { mob | obj | room } { trigger } { name } [ location ]\r\n");
        return;
    }

    num_arg = atoi(targ_name);
    tn = atoi(trig_name);
    loc = (*loc_name) ? atoi(loc_name) : -1;

    if (is_abbrev(arg, "mobile") || is_abbrev(arg, "mtr")) {
        victim = get_char_vis(ch, targ_name, nullptr, FIND_CHAR_WORLD);
        if (!victim) { /* search room for one with this vnum */
            auto people = ch->location.getPeople();
            for (auto t : filter_raw(people)) {
                if (GET_MOB_VNUM(t) == num_arg) {
                    victim = t;
                    break;
                }
            }

            if (!victim) {
                                ch->sendText("That mob does not exist.\r\n");
                return;
            }
        }
        if (!IS_NPC(victim)) {
                        ch->sendText("Players can't have scripts.\r\n");
            return;
        }
        if (!can_edit_zone(ch, ch->location.getZone()->number)) {
                        ch->sendText("You can only attach triggers in your own zone\r\n");
            return;
        }
        /* have a valid mob, now get trigger */
        rn = real_trigger(tn);
        if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
                        ch->sendText("That trigger does not exist.\r\n");
            return;
        }

        add_trigger(victim, trig, loc);

                ch->send_to("Trigger %d (%s) attached to %s [%d].\r\n", tn, GET_TRIG_NAME(trig), GET_SHORT(victim), GET_MOB_VNUM(victim));
    } else if (is_abbrev(arg, "object") || is_abbrev(arg, "otr")) {
        object = get_obj_vis(ch, targ_name, nullptr);
        if(!object) ch->location.findObjectVnum(num_arg);
        if(!object) ch->findObjectVnum(num_arg);
        if(!object) {
                        ch->sendText("That object does not exist.\r\n");
            return;
        }

        if (!can_edit_zone(ch, ch->location.getZone()->number)) {
                        ch->sendText("You can only attach triggers in your own zone\r\n");
            return;
        }
        /* have a valid obj, now get trigger */
        rn = real_trigger(tn);
        if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
                        ch->sendText("That trigger does not exist.\r\n");
            return;
        }

        add_trigger(object, trig, loc);

                ch->send_to("Trigger %d (%s) attached to %s [%d].\r\n", tn, GET_TRIG_NAME(trig), (object->getShortDescription() ?
                      object->getShortDescription() : object->getName()), GET_OBJ_VNUM(object));
    } else if (is_abbrev(arg, "room") || is_abbrev(arg, "wtr")) {
        if (strchr(targ_name, '.'))
            rnum = IN_ROOM(ch);
        else if (isdigit(*targ_name))
            rnum = find_target_room(ch, targ_name);
        else
            rnum = NOWHERE;

        if (rnum == NOWHERE) {
                        ch->sendText("You need to supply a room number or . for current room.\r\n");
            return;
        }

        if (!can_edit_zone(ch, get_room(rnum)->zone->number)) {
                        ch->sendText("You can only attach triggers in your own zone\r\n");
            return;
        }
        /* have a valid room, now get trigger */
        rn = real_trigger(tn);
        if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
                        ch->sendText("That trigger does not exist.\r\n");
            return;
        }

        room = get_room(rnum);

        add_trigger(room, trig, loc);

                ch->send_to("Trigger %d (%s) attached to room %d.\r\n", tn, GET_TRIG_NAME(trig), get_room(rnum)->getVnum());
    } else
                ch->sendText("Please specify 'mob', 'obj', or 'room'.\r\n");
}


/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
int remove_trigger(script_data *sc, char *name) {
    std::shared_ptr<trig_data> j;
    int num = 0, string = false, n;
    char *cname;

    if (!sc)
        return 0;

    if ((cname = strstr(name, ".")) || (!isdigit(*name))) {
        string = true;
        if (cname) {
            *cname = '\0';
            num = atoi(name);
            name = ++cname;
        }
    } else
        num = atoi(name);

    n = 0;
    j = nullptr;
    auto scripts = sc->getScripts();
    for (auto i : filter_shared(scripts)) {
        if (string) {
            if (isname(name, i->proto->name.c_str()))
                if (++n >= num) {
                    j = i;
                    break;
                }
        }

            /* this isn't clean... */
            /* a numeric value will match if it's position OR vnum */
            /* is found. originally the number was position-only */
        else if (++n >= num) {
            j = i;
            break;
        }
        else if (i->getVnum() == num) {
            j = i;
            break;
        }
    }

    if (j) {
        j->deactivate();
        if(sc->running_scripts.has_value()) {
            auto &scr = sc->running_scripts.value();
            if (auto find = std::find(scr.begin(), scr.end(), j->getVnum()); find != scr.end())
                scr.erase(find);
        }
        sc->scripts.erase(j->getVnum());

        /* update the script type bitvector */
        SCRIPT_TYPES(sc) = 0;
        auto update_scripts = sc->getScripts();
        for (auto i : filter_shared(update_scripts))
            SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(i);

        return 1;
    }
    return 0;
}

ACMD(do_detach) {
    char_data *victim = nullptr;
    obj_data *object = nullptr;
    struct room_data *room;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
    char *trigger = nullptr;
    int num_arg;

    argument = two_arguments(argument, arg1, arg2);
    one_argument(argument, arg3);

    if (!*arg1 || !*arg2) {
        ch->sendText("Usage: detach [ mob | object | room ] { target } { trigger |"
                    " 'all' }\r\n");
        return;
    }

    /* vnum of mob/obj, if given */
    num_arg = atoi(arg2);

    if (!strcasecmp(arg1, "room") || !strcasecmp(arg1, "wtr")) {
        room = ch->getRoom();
        if (!can_edit_zone(ch, room->zone->number)) {
            ch->sendText("You can only detach triggers in your own zone\r\n");
            return;
        }
        if (!SCRIPT(room))
                ch->sendText("This room does not have any triggers.\r\n");
        else if (!strcasecmp(arg2, "all")) {
            extract_script(room, WLD_TRIGGER);
            ch->sendText("All triggers removed from room.\r\n");
        } else if (remove_trigger(SCRIPT(room), arg2)) {
            ch->sendText("Trigger removed.\r\n");
            if (room->getScripts().empty()) {
                extract_script(room, WLD_TRIGGER);
            }
        } else
            ch->sendText("That trigger was not found.\r\n");
    } else {
        if (is_abbrev(arg1, "mobile") || !strcasecmp(arg1, "mtr")) {
            victim = get_char_vis(ch, arg2, nullptr, FIND_CHAR_WORLD);
            if (!victim) { /* search room for one with this vnum */
                auto people = ch->location.getPeople();
                for (auto v : filter_raw(people)) {
                    victim = v;
                    if (GET_MOB_VNUM(victim) == num_arg)
                        break;
                }

                if (!victim) {
                                        ch->sendText("No such mobile around.\r\n");
                    return;
                }
            }

            if (arg3 == nullptr || !*arg3)
                                ch->sendText("You must specify a trigger to remove.\r\n");
            else
                trigger = arg3;
        } else if (is_abbrev(arg1, "object") || !strcasecmp(arg1, "otr")) {
            object = get_obj_vis(ch, arg2, nullptr);
            if (!object) object = ch->location.findObjectVnum(num_arg);
            if (!object) object = ch->findObjectVnum(num_arg);
            if (!object) { /* give up */
                                ch->sendText("No such object around.\r\n");
                return;
            }

            if (arg3 == nullptr || !*arg3)
                                ch->sendText("You must specify a trigger to remove.\r\n");
            else
                trigger = arg3;
        } else {
            /* Thanks to Carlos Myers for fixing the line below */
            if ((object = get_obj_in_equip_vis(ch, arg1, nullptr, ch->getEquipment())));
            else if ((object = get_obj_in_list_vis(ch, arg1, nullptr, ch->getObjects())));
            else if ((victim = get_char_room_vis(ch, arg1, nullptr)));
            else if ((object = get_obj_in_list_vis(ch, arg1, nullptr, ch->location.getObjects())));
            else if ((victim = get_char_vis(ch, arg1, nullptr, FIND_CHAR_WORLD)));
            else if ((object = get_obj_vis(ch, arg1, nullptr)));
            else
                                ch->sendText("Nothing around by that name.\r\n");

            trigger = arg2;
        }

        if (victim) {
            if (!IS_NPC(victim))
                                ch->sendText("Players don't have triggers.\r\n");

            else if (!SCRIPT(victim))
                                ch->sendText("That mob doesn't have any triggers.\r\n");
            else if (!can_edit_zone(ch, real_zone_by_thing(GET_MOB_VNUM(victim)))) {
                                ch->sendText("You can only detach triggers in your own zone\r\n");
                return;
            } else if (trigger && !strcasecmp(trigger, "all")) {
                extract_script(victim, MOB_TRIGGER);
                                ch->send_to("All triggers removed from %s.\r\n", GET_SHORT(victim));
            } else if (trigger && remove_trigger(SCRIPT(victim), trigger)) {
                                ch->sendText("Trigger removed.\r\n");
                if (victim->getScripts().empty()) {
                    extract_script(victim, MOB_TRIGGER);
                }
            } else
                                ch->sendText("That trigger was not found.\r\n");
        } else if (object) {
            if (!SCRIPT(object))
                                ch->sendText("That object doesn't have any triggers.\r\n");

            else if (!can_edit_zone(ch, real_zone_by_thing(GET_OBJ_VNUM(object)))) {
                                ch->sendText("You can only detach triggers in your own zone\r\n");
                return;
            } else if (trigger && !strcasecmp(trigger, "all")) {
                extract_script(object, OBJ_TRIGGER);
                                ch->send_to("All triggers removed from %s.\r\n", object->getShortDescription() ? object->getShortDescription() :
                             object->getName());
            } else if (remove_trigger(SCRIPT(object), trigger)) {
                                ch->sendText("Trigger removed.\r\n");
                if (object->getScripts().empty()) {
                    extract_script(object, OBJ_TRIGGER);
                }
            } else
                                ch->sendText("That trigger was not found.\r\n");
        }
    }
}

/*
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void script_vlog(const char *format, va_list args) {
    char output[MAX_STRING_LENGTH];
    struct descriptor_data *i;

    snprintf(output, sizeof(output), "SCRIPT ERR: %s", format);

    // wrap up basic_mud_vlog in a va copy...
    va_list args_copy;
    va_copy(args_copy, args);
    basic_mud_vlog(output, args_copy);
    va_end(args_copy);

    /* the rest is mostly a rip from basic_mud_log() */
    strcpy(output, "[ ");            /* strcpy: OK */
    vsnprintf(output + 2, sizeof(output) - 6, format, args);
    strcat(output, " ]\r\n");        /* strcat: OK */

    for (i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
            continue;
        if (GET_ADMLEVEL(i->character) < ADMLVL_BUILDER)
            continue;
        if (PLR_FLAGGED(i->character, PLR_WRITING))
            continue;
        if (NRM > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
            continue;

                i->character->send_to("@g%s@n", output);
    }
}


void script_log(const char *format, ...) {
    va_list args;

    va_start(args, format);
    script_vlog(format, args);
    va_end(args);
}

bool is_num(const std::string& arg) {
    if (arg.empty()) return false;

    double value;
    auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
    return ec == std::errc() && ptr == arg.data() + arg.size();
}

static void eval_numeric_op(char *op, char *lhs, char *rhs, char *result) {
    auto l = atof(lhs);
    auto r = atof(rhs);

    std::string res;

    /* find the op, and figure out the value */
    if (!strcmp("||", op)) {
        res = fmt::format("{}", (l || r) ? 1 : 0);
    } else if (!strcmp("&&", op)) {
        res = fmt::format("{}", (l && r) ? 1 : 0);
    } else if (!strcmp("==", op)) {
        res = fmt::format("{}", (l == r) ? 1 : 0);
    } else if (!strcmp("!=", op)) {
        res = fmt::format("{}", (l != r) ? 1 : 0);
    } else if (!strcmp("<=", op)) {
        res = fmt::format("{}", (l <= r) ? 1 : 0);
    } else if (!strcmp(">=", op)) {
        res = fmt::format("{}", (l >= r) ? 1 : 0);
    } else if (!strcmp("<", op)) {
        res = fmt::format("{}", (l < r) ? 1 : 0);
    } else if (!strcmp(">", op)) {
        res = fmt::format("{}", (l > r) ? 1 : 0);
    }else if (!strcmp("*", op))
        res = fmt::format("{}", l * r);
    else if (!strcmp("/", op))
        res = fmt::format("{}", (int64_t)l / (int64_t)r);
    else if (!strcmp("+", op))
        res = fmt::format("{}", l + r);
    else if (!strcmp("-", op))
        res = fmt::format("{}", l - r);
    else if (!strcmp("!", op)) {
        res = fmt::format("{}", (r != 0.0) ? 0 : 1);
    }

    if(!res.empty()) strcpy(result, res.c_str());

}

static bool check_truthy(const char *txt) {
    if(!txt || !strlen(txt)) return false;

    // Try to convert to a number and check if it's zero
    std::stringstream str(txt);
    double num;
    str >> num;
    if (!str.fail() && num == 0.0) return false;

    // If not null, not empty, and not zero, it's truthy
    return true;

}


/* evaluates 'lhs op rhs', and copies to result */
void eval_op(char *op, char *lhs, char *rhs, char *result, unit_data *go,
             script_data *sc, trig_data *trig) {
    unsigned char *p;
    int n;

    /* strip off extra spaces at begin and end */
    while (*lhs && isspace(*lhs))
        lhs++;
    while (*rhs && isspace(*rhs))
        rhs++;

    for (p = (unsigned char *) lhs; *p; p++);
    for (--p; isspace(*p) && ((char *) p > lhs); *p-- = '\0');
    for (p = (unsigned char *) rhs; *p; p++);
    for (--p; isspace(*p) && ((char *) p > rhs); *p-- = '\0');


    if(is_num(lhs) && is_num(rhs)) {
        eval_numeric_op(op, lhs, rhs, result);
        return;
    }

    /* find the op, and figure out the value */
    if (!strcmp("||", op)) {
        if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
            strcpy(result, "0");
        else
            strcpy(result, "1");
    } else if (!strcmp("&&", op)) {
        if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
            strcpy(result, "0");
        else
            strcpy(result, "1");
    } else if (!strcmp("==", op)) {
        sprintf(result, "%d", !strcasecmp(lhs, rhs));
    } else if (!strcmp("!=", op)) {
        sprintf(result, "%d", strcasecmp(lhs, rhs));
    } else if (!strcmp("<=", op)) {
        sprintf(result, "%d", strcasecmp(lhs, rhs) <= 0);
    } else if (!strcmp(">=", op)) {
        sprintf(result, "%d", strcasecmp(lhs, rhs) <= 0);
    } else if (!strcmp("<", op)) {
        sprintf(result, "%d", strcasecmp(lhs, rhs) < 0);
    } else if (!strcmp(">", op)) {
        sprintf(result, "%d", strcasecmp(lhs, rhs) > 0);
    } else if (!strcmp("/=", op))
        sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');
    else if (!strcmp("!", op)) {
        sprintf(result, "%d", !check_truthy(rhs));
    } else if(!strcmp("-", op)) {
        sprintf(result, "-%s", rhs);
    }
}


/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
*/
char *matching_quote(char *p) {
    for (p++; *p && (*p != '"'); p++) {
        if (*p == '\\')
            p++;
    }

    if (!*p)
        p--;

    return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
char *matching_paren(char *p) {
    int i;

    for (p++, i = 1; *p && i; p++) {
        if (*p == '(')
            i++;
        else if (*p == ')')
            i--;
        else if (*p == '"')
            p = matching_quote(p);
    }

    return --p;
}


/* evaluates line, and returns answer in result */
void eval_expr(char *line, char *result, unit_data *go, script_data *sc,
               trig_data *trig, UnitType type) {
    char expr[MAX_INPUT_LENGTH], *p;

    while (*line && isspace(*line))
        line++;

    if (*line == '(') {
        p = strcpy(expr, line);
        p = matching_paren(expr);
        *p = '\0';
        eval_expr(expr + 1, result, go, sc, trig, type);
    } else if (eval_lhs_op_rhs(line, result, go, sc, trig, type)) {

    } else
        var_subst(go, sc, trig, type, line, result);
}

/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
int eval_lhs_op_rhs(char *expr, char *result, unit_data *go, script_data *sc,
                    trig_data *trig, UnitType type)
{
    char *p, *tokens[MAX_INPUT_LENGTH];
    char line[MAX_INPUT_LENGTH], lhr[MAX_INPUT_LENGTH], rhr[MAX_INPUT_LENGTH];
    int i, j;

    /*
     * valid operands, in order of priority
     * each must also be defined in eval_op()
     */
    static char *ops[] = {
        "||",
        "&&",
        "==",
        "!=",
        "<=",
        ">=",
        "<",
        ">",
        "/=",
        "-",
        "+",
        "/",
        "*",
        "!",
        "\n"
      };

    p = strcpy(line, expr);

    /*
     * initialize tokens, an array of pointers to locations
     * in line where the ops could possibly occur.
     */

    /* Might be game breaking - Iovan
    *lhr = '\0'; */

    for (j = 0; *p; j++) {
        tokens[j] = p;
        if (*p == '(')
            p = matching_paren(p) + 1;
        else if (*p == '"')
            p = matching_quote(p) + 1;
        else if (isalnum(*p))
            for (p++; *p && (isalnum(*p) || isspace(*p)); p++);
        else
            p++;
    }
    tokens[j] = nullptr;

    for (i = 0; *ops[i] != '\n'; i++)
        for (j = 0; tokens[j]; j++)
            if (!strncasecmp(ops[i], tokens[j], strlen(ops[i]))) {
                *tokens[j] = '\0';
                p = tokens[j] + strlen(ops[i]);

                eval_expr(line, lhr, go, sc, trig, type);
                eval_expr(p, rhr, go, sc, trig, type);
                eval_op(ops[i], lhr, rhr, result, go, sc, trig);

                return 1;
            }

    return 0;
}

/*
 * scans for end of if-block.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */


/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or last line of trigger.
 */


/* processes any 'wait' commands in a trigger */
void process_wait(unit_data *go, trig_data *trig, UnitType type, char *cmd,
                  struct cmdlist_element *cl) {
    char buf[MAX_INPUT_LENGTH], *arg;
    struct wait_event_data *wait_event_obj;
    long when, hr, min, ntime;
    double to_wait = 0.0;
    char c;

    arg = any_one_arg(cmd, buf);
    skip_spaces(&arg);

    if (!*arg) {
        script_log("Trigger: %s, VNum %d. wait w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cl->cmd);
        return;
    }

    if (!strncasecmp(arg, "until ", 6)) {

        /* valid forms of time are 14:30 and 1430 */
        if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
            min += (hr * 60);
        else
            min = (hr % 100) + ((hr / 100) * 60);

        // Convert current MUD time to seconds since midnight
        double current_mud_seconds = time_info.seconds +
                                     (time_info.minutes * SECS_PER_MINUTE) +
                                     (time_info.hours * SECS_PER_HOUR);

        // Convert target MUD time to seconds since midnight
        double target_mud_seconds = min * SECS_PER_MINUTE;

        // Determine waiting time in MUD seconds
        double waiting_mud_seconds = 0.0;
        if (current_mud_seconds >= target_mud_seconds) {
            // If the target time has already passed, wait until the next day
            waiting_mud_seconds = (SECS_PER_DAY - current_mud_seconds) + target_mud_seconds;
        } else {
            // If the target time is in the future, wait until that time
            waiting_mud_seconds = target_mud_seconds - current_mud_seconds;
        }

        // Convert waiting time to real seconds
        trig->setWaiting(waiting_mud_seconds / MUD_TIME_ACCELERATION);

    } else {
        std::string normalized;
        for (size_t i = 0; i < strlen(arg); ++i) {
            if (i > 0 && std::isdigit(arg[i - 1]) && std::isalpha(arg[i])) {
                normalized += ' ';
            }
            normalized += arg[i];
        }
        if (sscanf(normalized.c_str(), "%ld %c", &when, &c) == 2) {
            to_wait = when;
            if (c == 't')
                to_wait *= SECS_PER_HOUR / MUD_TIME_ACCELERATION;
            else if (c == 's')
                to_wait *= 1.0;
        }

        // We need to convert 'when' into a double of seconds-to-wait by dividing by PASSES_PER_SEC.
        trig->setWaiting(to_wait);
    }
}


/* processes a script set command */
void process_set(script_data *sc, trig_data *trig, char *cmd) {
    char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;

    value = two_arguments(cmd, arg, name);

    skip_spaces(&value);

    if (!*name) {
        script_log("Trigger: %s, VNum %d. set w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    trig->setVariable(name, value);

}

/* processes a script eval command */
void process_eval(unit_data *go, script_data *sc, trig_data *trig,
                  UnitType type, char *cmd) {
    char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    char result[MAX_INPUT_LENGTH], *expr;

    expr = one_argument(cmd, arg); /* cut off 'eval' */
    expr = one_argument(expr, name); /* cut off name */

    skip_spaces(&expr);

    if (!*name) {
        script_log("Trigger: %s, VNum %d. eval w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    if(isUID(expr)) {
        trig->setVariable(name, expr);
    } else {
        eval_expr(expr, result, go, sc, trig, type);
        trig->setVariable(name, result);
    }
}


/* script attaching a trigger to something */
void process_attach(unit_data *go, script_data *sc, trig_data *trig,
                    UnitType type, char *cmd) {
    char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
    char result[MAX_INPUT_LENGTH], *id_p;
    std::shared_ptr<trig_data> newtrig;
    char_data *c = nullptr;
    obj_data *o = nullptr;
    room_data *r = nullptr;
    long trignum, id;

    id_p = two_arguments(cmd, arg, trignum_s);
    skip_spaces(&id_p);

    if (!*trignum_s) {
        script_log("Trigger: %s, VNum %d. attach w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    if(isUID(id_p)) {
        snprintf(result, sizeof(result), "%s", id_p);
    } else {
        /* parse and locate the id specified */
        eval_expr(id_p, result, go, sc, trig, type);
    }

    /* parse and locate the id specified */
    auto uidResult = resolveUID(result);
    if(!uidResult) {
        script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    if(auto room = std::dynamic_pointer_cast<room_data>(uidResult); room) {
        r = room.get();
    } else if(auto ch = std::dynamic_pointer_cast<char_data>(uidResult); ch) {
        c = ch.get();
    } else if(auto obj = std::dynamic_pointer_cast<obj_data>(uidResult); obj) {
        o = obj.get();
    }

    /* locate and load the trigger specified */
    trignum = real_trigger(atoi(trignum_s));
    if (trignum == NOTHING || !(newtrig = read_trigger(trignum))) {
        script_log("Trigger: %s, VNum %d. attach invalid trigger: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), trignum_s);
        return;
    }

    if (c) {
        if (!IS_NPC(c)) {
            script_log("Trigger: %s, VNum %d. attach invalid target: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), GET_NAME(c));
            return;
        }
        add_trigger(c, newtrig, -1);
        return;
    }

    if (o) {
        add_trigger(o, newtrig, -1);
        return;
    }

    if (r) {
        add_trigger(r, newtrig, -1);
        return;
    }

}


/* script detaching a trigger from something */
void process_detach(unit_data *go, script_data *sc, trig_data *trig,
                    UnitType type, char *cmd) {
    char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
    char result[MAX_INPUT_LENGTH], *id_p;
    char_data *c = nullptr;
    obj_data *o = nullptr;
    room_data *r = nullptr;
    long id;

    id_p = two_arguments(cmd, arg, trignum_s);
    skip_spaces(&id_p);

    if (!*trignum_s) {
        script_log("Trigger: %s, VNum %d. detach w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    if(isUID(id_p)) {
        snprintf(result, sizeof(result), "%s", id_p);
    } else {
        /* parse and locate the id specified */
        eval_expr(id_p, result, go, sc, trig, type);
    }

    /* parse and locate the id specified */
    auto uidResult = resolveUID(result);
    if(!uidResult) {
        script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    if(auto room = std::dynamic_pointer_cast<room_data>(uidResult); room) {
        r = room.get();
    } else if(auto ch = std::dynamic_pointer_cast<char_data>(uidResult); ch) {
        c = ch.get();
    } else if(auto obj = std::dynamic_pointer_cast<obj_data>(uidResult); obj) {
        o = obj.get();
    }

    if (c) {
        if (!strcmp(trignum_s, "all")) {
            extract_script(c, MOB_TRIGGER);
            return;
        }
        if (remove_trigger(c, trignum_s)) {
            if (c->getScripts().empty()) {
                extract_script(c, MOB_TRIGGER);
            }
        }
        return;
    }

    if (o) {
        if (!strcmp(trignum_s, "all")) {
            extract_script(o, OBJ_TRIGGER);
            return;
        }
        if (remove_trigger(o, trignum_s)) {
            if (o->getScripts().empty()) {
                extract_script(o, OBJ_TRIGGER);
            }
        }
        return;
    }

    if (r) {
        if (!strcmp(trignum_s, "all")) {
            extract_script(r, WLD_TRIGGER);
            return;
        }
        if (remove_trigger(r, trignum_s)) {
            if (r->getScripts().empty()) {
                extract_script(r, WLD_TRIGGER);
            }
        }
        return;
    }

}

struct room_data *dg_room_of_obj(struct obj_data *obj) {
    return obj->getAbsoluteRoom();
}


/*
 * processes a script return command.
 * returns the new value for the script to return.
 */
int process_return(trig_data *trig, char *cmd) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(cmd, arg1, arg2);

    if (!*arg2) {
        script_log("Trigger: %s, VNum %d. return w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);

        return 1;
    }

    return atoi(arg2);
}


/*
 * removes a variable from the global vars of sc,
 * or the local vars of trig if not found in global list.
 */
void process_unset(script_data *sc, trig_data *trig, char *cmd) {
    char arg[MAX_INPUT_LENGTH], *var;

    var = any_one_arg(cmd, arg);

    skip_spaces(&var);

    if (!*var) {
        script_log("Trigger: %s, VNum %d. unset w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    if (auto find = sc->variables.find(var); find != sc->variables.end()) {
        sc->variables.erase(find);
        return;
    }
    trig->eraseVariable(var);
}


/*
 * copy a locally owned variable to the globals of another script
 *     'remote <variable_name> <uid>'
 */
void process_remote(script_data *sc, trig_data *trig, char *cmd) {
    struct trig_var_data *vd;
    script_data *sc_remote = nullptr;
    char *line, *var, *uid_p;
    char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    long uid, context;
    room_data *room;
    char_data *mob;
    obj_data *obj;

    line = any_one_arg(cmd, arg);
    two_arguments(line, buf, buf2);
    var = buf;
    uid_p = buf2;
    skip_spaces(&var);
    skip_spaces(&uid_p);


    if (!*buf || !*buf2) {
        script_log("Trigger: %s, VNum %d. remote: invalid arguments '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    /* find the locally owned variable */
    auto varIt = trig->variables.find(buf);
    if (varIt == trig->variables.end()) {
        script_log("Trigger: %s, VNum %d. local var '%s' not found in remote call",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf);
        return;
    }
    /* find the target script from the uid number */
    auto uidResult = resolveUID(buf2);
    if(!uidResult) {
        script_log("Trigger: %s, VNum %d. remote: illegal uid '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf2);
        return;
    }

    sc_remote = uidResult.get();

    if (sc_remote == nullptr) return; /* no script to assign */
    sc_remote->setVariable(varIt->first, varIt->second); /* copy the variable to the script's variables */
}


/*
 * command-line interface to rdelete
 * named vdelete so people didn't think it was to delete rooms
 */
ACMD(do_vdelete) {
    struct trig_var_data *vd, *vd_prev = nullptr;
    script_data *sc_remote = nullptr;
    char *var, *uid_p;
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    long uid, context;
    room_data *room;
    char_data *mob;
    obj_data *obj;

    argument = two_arguments(argument, buf, buf2);
    var = buf;
    uid_p = buf2;
    skip_spaces(&var);
    skip_spaces(&uid_p);


    if (!*buf || !*buf2) {
                ch->sendText("Usage: vdelete { <variablename> | * | all } <id>\r\n");
        return;
    }

    auto uidResult = resolveUID(buf2);
    if(!uidResult) {
                ch->sendText("vdelete: illegal id specified.\r\n");
        return;
    }
    sc_remote = uidResult.get();

    if(!sc_remote) {
                ch->sendText("vdelete: cannot resolve specified id.\r\n");
        return;
    }

    if (sc_remote->variables.empty()) {
                ch->sendText("That id represents no global variables.(2)\r\n");
        return;
    }

    if (*var == '*' || is_abbrev(var, "all")) {
        sc_remote->variables.clear(); /* clear the script's variables */
                ch->sendText("All variables deleted from that id.\r\n");
        return;
    }

    /* find the global */
    auto find = sc_remote->variables.find(var);
    if (find == sc_remote->variables.end()) {
                ch->sendText("That variable cannot be located.\r\n");
        return;
    }

    sc_remote->variables.erase(find); /* remove from script's variables */
        ch->sendText("Deleted.\r\n");
}

/*
 * Called from do_set - return 0 for failure, 1 for success.
 * ch and vict are verified
 */
int perform_set_dg_var(struct char_data *ch, struct char_data *vict, char *val_arg) {
    char var_name[MAX_INPUT_LENGTH], *var_value;

    var_value = any_one_arg(val_arg, var_name);

    if (var_name == nullptr || !*var_name || var_value == nullptr || !*var_value) {
                ch->sendText("Usage: set <char> <varname> <value>\r\n");
        return 0;
    }

    vict->setVariable(var_name, var_value); /* add the variable to the script's variables */
    return 1;
}

/*
 * delete a variable from the globals of another script
 *     'rdelete <variable_name> <uid>'
 */
void process_rdelete(script_data *sc, trig_data *trig, char *cmd) {
    struct trig_var_data *vd, *vd_prev = nullptr;
    script_data *sc_remote = nullptr;
    char *line, *var, *uid_p;
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    long uid, context;
    room_data *room;
    char_data *mob;
    obj_data *obj;

    line = any_one_arg(cmd, arg);
    two_arguments(line, buf, buf2);
    var = buf;
    uid_p = buf2;
    skip_spaces(&var);
    skip_spaces(&uid_p);


    if (!*buf || !*buf2) {
        script_log("Trigger: %s, VNum %d. rdelete: invalid arguments '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    auto uidResult = resolveUID(buf2);
    if(!uidResult) {
        script_log("Trigger: %s, VNum %d. rdelete: illegal uid '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf2);
        return;
    }
    sc_remote = uidResult.get();

    if (sc_remote == nullptr) return; /* no script to delete a trigger from */
    if(auto find = sc_remote->variables.find(var); find != sc_remote->variables.end()) {
        sc_remote->variables.erase(find); /* remove from script's variables */
        return;
    }
    script_log("Trigger: %s, VNum %d. rdelete: variable '%s' not found in remote call",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), var);
    return;
}


/*
 * makes a local variable into a global variable
 */
void process_global(script_data *sc, trig_data *trig, char *cmd, long id) {
    struct trig_var_data *vd;
    char arg[MAX_INPUT_LENGTH], *var;

    var = any_one_arg(cmd, arg);

    skip_spaces(&var);

    if (!*var) {
        script_log("Trigger: %s, VNum %d. global w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

    auto varIt = trig->variables.find(var);
    if (varIt == trig->variables.end()) {
        script_log("Trigger: %s, VNum %d. local var '%s' not found in global call",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), var);
        return;
    }

    sc->setVariable(varIt->first, varIt->second); /* copy the variable to the script's variables */
}


/* set the current context for a script */
void process_context(script_data *sc, trig_data *trig, char *cmd) {
    char arg[MAX_INPUT_LENGTH], *var;

    var = any_one_arg(cmd, arg);

    skip_spaces(&var);

    if (!*var) {
        script_log("Trigger: %s, VNum %d. context w/o an arg: '%s'",
                   GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
    }

}

void extract_value(script_data *sc, trig_data *trig, char *cmd) {
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    char *buf3;
    char to[128];
    int num;

    buf3 = any_one_arg(cmd, buf);
    half_chop(buf3, buf2, buf);
    strcpy(to, buf2);

    num = atoi(buf);
    if (num < 1) {
        script_log("extract number < 1!");
        return;
    }

    half_chop(buf, buf3, buf2);

    while (num > 0) {
        half_chop(buf2, buf, buf2);
        num--;
    }

    trig->setVariable(to, buf);
}

/*
  Thanks to Jamie Nelson for 4 dimensions for this addition

  Syntax :
    dg_letter <new varname> <letter position> <string to get from>

    ie:
    set string L337-String
    dg_letter var1 4 %string%
    dg_letter var2 11 %string%

    now %var1% == 7 and %var2% == g

    Note that the index starts at 1.

*/

void dg_letter_value(script_data *sc, trig_data *trig, char *cmd) {
    //set the letter/number at position 'num' as the variable.
    char junk[MAX_INPUT_LENGTH];
    char varname[MAX_INPUT_LENGTH];
    char num_s[MAX_INPUT_LENGTH];
    char string[MAX_INPUT_LENGTH];
    int num;

    half_chop(cmd, junk, cmd);   /* "dg_letter" */
    half_chop(cmd, varname, cmd);
    half_chop(cmd, num_s, string);

    num = atoi(num_s);

    script_log("The use of dg_letter is deprecated");
    script_log("- Use 'set <new variable> %%<text/var>.charat(index)%%' instead.");


    if (num < 1) {
        script_log("Trigger #%d : dg_letter number < 1!", GET_TRIG_VNUM(trig));
        return;
    }

    if (num > strlen(string)) {
        script_log("Trigger #%d : dg_letter number > strlen!", GET_TRIG_VNUM(trig));
        return;
    }

    *junk = string[num - 1];
    *(junk + 1) = '\0';
    trig->setVariable(varname, junk);
}

/*  This is the core driver for scripts. */
/*  Arguments:
    void *go_adress
      A pointer to a pointer to the entity running the script.
      The reason for this approcah is that we want to be able to see
      from the calling function, if the entity has been free'd.

    trig_data *trig
      A pointer to the current running trigger.

    int type
      MOB_TRIGGER, OBJ_TRIGGER or WLD_TRIGGER, respectively.

    int mode
      TRIG_NEW     just started from dg_triggers.c
      TRIG_RESTART restarted after a 'wait'
*/


/* returns the real number of the trigger with given virtual number */
trig_rnum real_trigger(trig_vnum vnum) {
    return trig_index.count(vnum) ? vnum : NOTHING;
}

ACMD(do_tstat) {
    int rnum;
    char str[MAX_INPUT_LENGTH];

    half_chop(argument, str, argument);
    if (*str) {
        rnum = real_trigger(atoi(str));
        if (rnum == NOTHING) {
                        ch->sendText("That vnum does not exist.\r\n");
            return;
        }

        do_stat_trigger(ch, &trig_index.at(rnum));
    } else
                ch->sendText("Usage: tstat <vnum>\r\n");
}

/*
* scans for a case/default instance
* returns the line containg the correct case instance, or the last
* line of the trigger if not found.
*/
struct cmdlist_element *
find_case(struct trig_data *trig, struct cmdlist_element *cl,
    unit_data *go, script_data *sc, UnitType type, char *cond) {
    char result[MAX_INPUT_LENGTH];
    struct cmdlist_element *c;
    char *p, *buf;

    eval_expr(cond, result, go, sc, trig, type);

    if (!(cl->next))
        return cl;

    for (c = cl->next; c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++);

        if (!strncasecmp("while ", p, 6) || !strncasecmp("switch", p, 6))
            c = find_done(c);
        else if (!strncasecmp("case ", p, 5)) {
            buf = (char *) malloc(MAX_STRING_LENGTH);
            eval_op("==", result, p + 5, buf, go, sc, trig);
            if (*buf && *buf != '0') {
                free(buf);
                return c;
            }
            free(buf);
        } else if (!strncasecmp("default", p, 7))
            return c;
        else if (!strncasecmp("done", p, 3))
            return c;
    }
    return c;
}

/*
* scans for end of while/switch-blocks.
* returns the line containg 'end', or the last
* line of the trigger if not found.
* Malformed scripts may cause nullptr to be returned.
*/
struct cmdlist_element *find_done(struct cmdlist_element *cl) {
    struct cmdlist_element *c;
    char *p;

    if (!cl || !(cl->next))
        return cl;

    for (c = cl->next; c && c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++);

        if (!strncasecmp("while ", p, 6) || !strncasecmp("switch ", p, 7))
            c = find_done(c);
        else if (!strncasecmp("done", p, 3))
            return c;
    }

    return c;
}


/* read a line in from a file, return the number of chars read */
int fgetline(FILE *file, char *p) {
    int count = 0;

    do {
        *p = fgetc(file);
        if (*p != '\n' && !feof(file)) {
            p++;
            count++;
        }
    } while (*p != '\n' && !feof(file));

    if (*p == '\n') *p = '\0';

    return count;
}

/* find_char() helpers */

// Must be power of 2
constexpr int BUCKET_COUNT = 64;
// to recognize an empty bucket
constexpr int UID_OUT_OF_RANGE = 1000000000;


int check_flags_by_name_ar(bitvector_t *array, int numflags, char *search, const char *namelist[]) {
    int i, item = -1;

    for (i = 0; i < numflags && item < 0; i++)
        if (!strcmp(search, namelist[i]))
            item = i;

    if (item < 0)
        return false;

    if (IS_SET_AR(array, item))
        return item;

    return false;
}

std::shared_ptr<trig_data> trig_data::shared() {
    return shared_from_this();
}

// Note: Trigger instances are meant to be set all active or inactive on a per room/character/item basis,
// not individually.
void trig_data::activate() {
    if(active) {
        return;
    }
    active = true;
    auto sh = shared_from_this();
    std::unordered_set<std::string> services;
    services.insert("active");
    services.insert(fmt::format("vnum_{}", getVnum()));
    if(waiting != 0.0) {
        services.insert("waiting");
    }
    for(const auto& s : services) {
        triggerSubscriptions.subscribe(s, sh);
    }
}

void trig_data::deactivate() {
    if(!active) {
        return;
    }
    active = false;
    auto sh = shared_from_this();
    triggerSubscriptions.unsubscribeFromAll(sh);
}


int trig_data::getVnum() const {
    return proto->vn;
}

UnitType trig_data::getAttachType() const {
    return proto->attach_type;
}

long trig_data::getTriggerType() const {
    return proto->trigger_type;
}


trig_data::trig_data(const trig_proto_data& other) : trig_data() {
    proto = (trig_proto_data*)&other;
}

ScriptLine trig_proto_data::getLine(int line) const {
    if(line < 0 || line >= lines.size()) {
            throw DgScriptError("Line number out of range");
        }
    return lines[line];
}

std::vector<ScriptLine> parse_script(const std::vector<std::string> &orig) {
    std::vector<ScriptLine> result;
    // We're going to iterate through orig and parse each line.
    // Each line will be split into its type, and remaining content.
    // The first token will be checked and used to determine the 
    // appropriate ScriptLineType. It might be the entire string 
    // (note: the lines in orig do not end in newlines),
    // or it might be the first token, respecting ( and space as a token delimiter.
    
    // FOR EXAMPLE, some are written like "if (expr...",
    // others as "if(expr..." and still more as "if expr..." and we must
    // preserve the parentheses if they exist, so treat both space and
    // start parentheses as opening delimiter.

    // For else, end, break, their second part is empty, we can discard anything
    // following them.

    // Lines beginning with * are comments. Preserve following text in the second
    // part of the ScriptLine.

    // strip all leading whitespace from all ScriptLineTypes.
    // strip all trailing whitespace from everything but COMMENT and COMMAND.

    for (const auto &line : orig) {
        ScriptLineType type;
        std::string content;
        std::string trimmedLine = line;
        // Remove leading whitespace
        trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
        
        if (trimmedLine.empty()) {
            continue; // skip empty lines
        }

        // Check the first token
        size_t pos = trimmedLine.find_first_of(" (");
        std::string firstToken = (pos == std::string::npos) ? trimmedLine : trimmedLine.substr(0, pos);

        if (firstToken == "if") {
            type = ScriptLineType::IF;
            content = trimmedLine.substr(pos + 1); // skip the 'if' and space/(
        } else if (firstToken == "elseif") {
            type = ScriptLineType::ELSEIF;
            content = trimmedLine.substr(pos + 1); // skip the 'elseif' and space
        } else if (firstToken == "else") {
            type = ScriptLineType::ELSE;
        } else if (firstToken == "while") {
            type = ScriptLineType::WHILE;
            content = trimmedLine.substr(pos + 1);
        } else if (firstToken == "switch") {
            type = ScriptLineType::SWITCH;
            content = trimmedLine.substr(pos + 1);
        } else if (firstToken == "case") {
            type = ScriptLineType::CASE;
            content = trimmedLine.substr(pos + 1);
        } else if (firstToken == "default") {
            type = ScriptLineType::DEFAULT;
        } else if (firstToken == "end") {
            type = ScriptLineType::END;
        } else if (firstToken == "done") {
            type = ScriptLineType::DONE;
        } else if (firstToken == "break") {
            type = ScriptLineType::BREAK;
        } else if (firstToken[0] == '*') { // comment line
            type = ScriptLineType::COMMENT;
            content = trimmedLine.substr(1); // skip the '*
        } else {
            type = ScriptLineType::COMMAND;
            content = trimmedLine; // the entire line is the command
        }
        result.emplace_back(type, content);
    }
    return result;
}

void trig_data::reset() {
    variables.clear();
    depth_stack.clear();
    current_line = 0;
    state = DgScriptState::READY;
    waiting = 0.0;
}

void trig_data::setState(DgScriptState newState) {
    state = newState;
    switch(state) {
        case DgScriptState::PAUSED:
        case DgScriptState::WAITING:
            triggerSubscriptions.subscribe("waiting", shared_from_this());
            break;
    }
}

void trig_data::setWaiting(double waitTime, DgScriptState newState) {
    if(waitTime < 0.0) {
        throw DgScriptError("Waiting time cannot be negative.");
    }
    waiting = waitTime;
    setState(newState);
}

bool trig_data::isReady() const {
    return state == DgScriptState::READY || state == DgScriptState::ERROR || state == DgScriptState::DONE;
}

void trig_data::error(const std::string &msg) {
    // Log the error message and set the state to ERROR
    script_log("Error in trigger '%s' (VNum %d) at line %d: %s", GET_TRIG_NAME(this), getVnum(), current_line, (char*)msg.c_str());
    // Optionally, you can also throw an exception or handle it as needed.
}

int trig_data::execute() {
    // This is a clever hack to prevent the script from being deleted
    // during execution if, by some chance, it obliterates its host object.

    std::shared_ptr<unit_data> sh;
    switch(owner->type) {
        case UnitType::character:
            sh = ((char_data*)owner)->shared();
            break;
        case UnitType::object:
            sh = ((obj_data*)owner)->shared();
            break;
        case UnitType::room:
            sh = ((room_data*)owner)->shared();
            break;
        default:
            throw DgScriptError("Invalid owner type for script execution.");
    }

    try {
        // Execute the trigger's script
        if(!owner->isActive()) {
            throw DgScriptError("Owner is inactive.");
        }
        if(depth_stack.size() > MAX_SCRIPT_DEPTH) {
            throw DgScriptError("Exceeded maximum script depth.");
        }
        if(state == DgScriptState::ERROR || state == DgScriptState::DONE) {
            reset();
        }
        int executionCount = 0;
        state = DgScriptState::RUNNING;
        int processed_line = 0;
        // The main script driver, we'll keep running lines until we've run too long, run out of lines,
        // the state changes, or the owner becomes inactive.
        while((current_line < proto->lines.size()) && (state == DgScriptState::RUNNING) && owner->isActive()) {
            if(executionCount++ > 50) {
                setWaiting(0.1, DgScriptState::PAUSED);
                break; // Prevent infinite loops, yield control
            }
            auto line = proto->getLine(current_line);
            processed_line = current_line;
            processLine(line);
        }

        if(state == DgScriptState::RUNNING && (processed_line == (proto->lines.size() - 1))) {
            // If we reached the end of the script, we set it to DONE.
            // This is important to ensure that the script is marked as finished.
            setState(DgScriptState::DONE);
        }
        
    } catch (const DgScriptError& e) {
        state = DgScriptState::ERROR;
        error(e.what());
    }

    return toReturn;
}

void trig_data::processLine(const ScriptLine& line) {
    // The way this one works: it processes a single line of the script.
    // Since we don't have proper blocks, this handles all flow control by
    // determining which line to jump to and checking the context in the
    // depth_stack.
    switch(std::get<0>(line)) {

        case ScriptLineType::COMMENT:
            // Comments are ignored, just increment the line counter.
            current_line++;
            break;

        case ScriptLineType::COMMAND:
            // Execute the command.
            processCommand(std::get<1>(line));
            current_line++;
            break;

        case ScriptLineType::IF: {
            
            auto cond = std::get<1>(line);
            // Evaluate the while condition.
            auto expr = evaluateExpression(cond);
            if(evaluateComparison(expr, "1", "==")) {
                // Condition is true, execute the loop body
                depth_stack.emplace_back(ScriptLineType::IF, current_line, true, "");
                current_line++;
            } else {
                // Condition is false, exit the loop
                depth_stack.emplace_back(ScriptLineType::IF, current_line, false, "");
                current_line = locateElseIfElseEnd(current_line + 1);
            }
        }
            break;
        case ScriptLineType::ELSEIF: {
            if(depth_stack.empty() || std::get<0>(depth_stack.back()) != ScriptLineType::IF) {
                throw DgScriptError("Elseif without matching if.");
            }
            auto cond = std::get<1>(line);
            // Evaluate the while condition.
            auto expr = evaluateExpression(cond);
            if(!std::get<2>(depth_stack.back()) && evaluateComparison(expr, "1", "==")) {
                // Condition is true, execute the loop body
                std::get<2>(depth_stack.back()) = true; // mark this elseif as true
                current_line++;
            } else {
                // Condition is false
                current_line = locateElseIfElseEnd(current_line + 1);
            }
        }
            break;

        case ScriptLineType::ELSE:
            if(depth_stack.empty() || std::get<0>(depth_stack.back()) != ScriptLineType::IF) {
                throw DgScriptError("Elseif without matching if.");
            }
            if(std::get<2>(depth_stack.back())) {
                // We already executed an elseif, so we skip this else.
                current_line = locateElseIfElseEnd(current_line + 1);
            } else {
                // We execute the else block.
                std::get<2>(depth_stack.back()) = true; // mark this else as true
                current_line++;
            }
            break;

        case ScriptLineType::WHILE: {
            depth_stack.emplace_back(ScriptLineType::WHILE, current_line, false, "");
            auto cond = std::get<1>(line);
            // Evaluate the while condition.
            auto expr = evaluateExpression(cond);
            if(evaluateComparison(expr, "1", "==")) {
                // Condition is true, execute the loop body
                current_line++;
            } else {
                // Condition is false, exit the loop
                current_line = locateDone(ScriptLineType::WHILE,  current_line + 1);
            }
        }
            break;

        case ScriptLineType::SWITCH: {
            auto cond = std::get<1>(line);
            // Evaluate the switch condition.
            auto expr = evaluateExpression(cond);
            // we store the evaluated expression in the depth stack so we can compare it against cases.
            depth_stack.emplace_back(ScriptLineType::SWITCH, current_line, false, expr);
            current_line = locateCaseDefaultDone(current_line + 1); // Skip to the next case/default/end
        }
            break;

        case ScriptLineType::CASE: {
            if(depth_stack.empty() || std::get<0>(depth_stack.back()) != ScriptLineType::SWITCH) {
                throw DgScriptError("Case without matching switch.");
            }
            auto &dep = depth_stack.back();
            if(std::get<2>(dep)) {
                // This case has already matched, and we didn't get a break, so we fallthrough.
                current_line++;
            } else {
                auto swcond = std::get<3>(dep);
                auto cond = std::get<1>(line);
                auto expr = evaluateExpression(cond);
                if(evaluateComparison(swcond, expr, "==")) {
                    // This case matched so we will enter the case block.
                    std::get<2>(dep) = true; // mark this case as matched
                    current_line++; 
                } else {
                    // This case did not match, so we'll advance to the next case, default, or end.
                    // important to +1 so we don't loop on the same case.
                    current_line = locateCaseDefaultDone(current_line + 1);
                }
            }
        }
            break;

        case ScriptLineType::DEFAULT: {
            if(depth_stack.empty() || std::get<0>(depth_stack.back()) != ScriptLineType::SWITCH) {
                throw DgScriptError("Default without matching switch.");
            }

            auto &dep = depth_stack.back();
            if(std::get<2>(dep)) {
                // This switch has already matched, and we didn't get a break, so we fallthrough.
                current_line++;
            } else {
                // This default was reached by lacking any other match.
                std::get<2>(dep) = true; // mark this switch as matched
                current_line++;
            }
        }
            break;

        case ScriptLineType::END:
            if(depth_stack.empty() || std::get<0>(depth_stack.back()) != ScriptLineType::IF) {
                throw DgScriptError("End without matching if.");
            }
            depth_stack.pop_back();
            current_line++;
            break;

        case ScriptLineType::DONE: {
            if(depth_stack.empty()) {
                throw DgScriptError("Done without matching switch or while.");
            }
            auto last = std::get<0>(depth_stack.back());
            auto retline = std::get<1>(depth_stack.back());
            if(last != ScriptLineType::WHILE && last != ScriptLineType::SWITCH) {
                throw DgScriptError("Done without matching switch or while.");
            }
            switch(last) {
                case ScriptLineType::WHILE:
                    current_line = retline; // Go back to the start of the while loop.
                    break;
                case ScriptLineType::SWITCH:
                    current_line++;
                    break;
            }
            depth_stack.pop_back();
        }
            break;

        case ScriptLineType::BREAK: 
            if(depth_stack.empty() || std::get<0>(depth_stack.back()) != ScriptLineType::SWITCH) {
                throw DgScriptError("Break without matching switch.");
            }
            current_line = locateDone(ScriptLineType::SWITCH, current_line + 1);
            break;
    }

}

int trig_data::locateElseIfElseEnd(int startLine) const {
    auto i = startLine;
    while(i < proto->lines.size()) {
        auto line = proto->getLine(i);
        switch(std::get<0>(line)) {
            case ScriptLineType::IF:
                // If we hit another if, we need to skip to the end of that if block.
                i = locateEnd(i + 1) + 1;
                break;
            case ScriptLineType::SWITCH:
            case ScriptLineType::WHILE:
                // If we hit a switch or while, we need to skip to the end of that
                // block.
                i = locateDone(std::get<0>(line), i + 1) + 1;
                break;
            case ScriptLineType::ELSEIF:
            case ScriptLineType::ELSE:
            case ScriptLineType::END:
                return i;
            default:
                // If we hit anything else, we are not at the end of the if block.
                i++;
                break;
        }
    }
    throw DgScriptError("Else/elseif/end not found in script.");
}

int trig_data::locateCaseDefaultDone(int startLine) const {
    auto i = startLine;
    while(i < proto->lines.size()) {
        auto line = proto->getLine(i);
        switch(std::get<0>(line)) {
            case ScriptLineType::CASE:
            case ScriptLineType::DEFAULT:
            case ScriptLineType::DONE:
                return i; // found the end
            case ScriptLineType::SWITCH:
            case ScriptLineType::WHILE:
                // If we hit a switch or while, we need to skip to the end of that
                // block.
                i = locateDone(std::get<0>(line), i + 1) + 1;
                break;
            case ScriptLineType::IF:
                // If we hit an if, we need to skip to the end of that if block
                i = locateEnd(i + 1) + 1;
                break;
            default:
                // If we hit anything else, we are not at the end of the switch block.
                i++;
                break;
        }
    }
    throw DgScriptError("Case/default/done not found in script.");
}

int trig_data::locateEnd(int startLine) const {
    auto i = startLine;
    while(i < proto->lines.size()) {
        auto line = proto->getLine(i);
        switch(std::get<0>(line)) {
            case ScriptLineType::SWITCH:
            case ScriptLineType::WHILE:
                i = locateDone(std::get<0>(line), i + 1) + 1;
                break;
            case ScriptLineType::IF:
                i = locateEnd(i + 1) + 1;
                break;
            case ScriptLineType::END:
                return i; // found the end
            default:
                // If we hit anything else, we are not at the end of the if block.
                i++;
                break;
        }
    }
    throw DgScriptError("End not found in script.");
}

int trig_data::locateDone(ScriptLineType type, int startLine) const {
    auto i = startLine;
    while(i < proto->lines.size()) {
        auto line = proto->getLine(i);
        switch(std::get<0>(line)) {
            case ScriptLineType::DONE:
                return i; // found the done
            case ScriptLineType::SWITCH:
            case ScriptLineType::WHILE:
                i = locateDone(std::get<0>(line), i + 1) + 1;
                break;
            case ScriptLineType::IF:
                i = locateEnd(i + 1) + 1;
                break;
            default:
                i++;
                break;
        }
    }
    throw DgScriptError("Done not found in script.");
}

std::string trig_data::evaluateExpression(const std::string& expr) {
    char buf[MAX_STRING_LENGTH];

    eval_expr((char*)expr.c_str(), buf, owner, owner, this, proto->attach_type);
    return std::string(buf);
}

bool trig_data::evaluateComparison(const std::string& left, const std::string& right, const std::string& op) {
    char buf[MAX_STRING_LENGTH];

    eval_op((char*)op.c_str(), (char*)left.c_str(), (char*)right.c_str(), buf, owner, owner, this);
    return truthy(std::string(buf));
}

bool trig_data::truthy(const std::string& value) const {
    // A value is truthy if it is not empty and not "0"
    return !value.empty() && value != "0";
}

std::string trig_data::substituteVariables(const std::string& raw_text) {
    char buf[MAX_STRING_LENGTH];

    var_subst(owner, owner, this, proto->attach_type, (char*)raw_text.c_str(), buf);

    return std::string(buf);
}

void trig_data::processCommand(const std::string& raw_text) {
    void obj_command_interpreter(obj_data *obj, char *argument);
    void wld_command_interpreter(struct room_data *room, char *argument);

    // This function processes a single command line.
    // It will handle the command based on its type.
    
    // we already have no leading whitespace, so we can just check if it's empty.
    if(raw_text.empty()) {
        return; // nothing to do
    }

    auto full_command = substituteVariables(raw_text);

    // full_command should be in the form <cmd> <args>
    // Although it might just be <cmd> without any args.
    // commands are always alphanumeric and can contain underscores.

    // attempt to split into command and arguments.
    std::string cmd;
    std::string args;
    size_t space_pos = full_command.find(' ');
    if(space_pos != std::string::npos) {
        cmd = full_command.substr(0, space_pos);
        args = full_command.substr(space_pos + 1);
    } else {
        cmd = full_command; // no arguments, just the command
    }

    // Now we can handle the command based on its type.
    if(boost::iequals(cmd, "eval")) {
        // Handle eval command
        process_eval(owner, owner, this, proto->attach_type, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "nop")) {
        // NOP does nothing, so we can just return.
    } else if(boost::iequals(cmd, "extract")) {
        // Handle extract command
        extract_value(owner, this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "dg_letter")) {
        // Handle dg_letter command
        dg_letter_value(owner, this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "halt")) {
        // Halt the script execution
        setState(DgScriptState::DONE);
    } else if(boost::iequals(cmd, "dg_cast")) {
        do_dg_cast(owner, owner, this, proto->attach_type, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "dg_affect")) {
        do_dg_affect(owner, owner, this, proto->attach_type, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "global")) {
        process_global(owner, this, (char*)full_command.c_str(), 0);
    } else if(boost::iequals(cmd, "context")) {
        process_context(owner, this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "remote")) {
        process_remote(owner, this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "rdelete")) {
        process_rdelete(owner, this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "return")) {
        toReturn = process_return(this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "set")) {
        process_set(owner, this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "unset")) {
        process_unset(owner, this, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "wait")) {
        process_wait(owner, this, proto->attach_type,
                     (char*)full_command.c_str(), nullptr);
    } else if(boost::iequals(cmd, "attach")) {
        process_attach(owner, owner, this, proto->attach_type, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "detach")) {
        process_detach(owner, owner, this, proto->attach_type, (char*)full_command.c_str());
    } else if(boost::iequals(cmd, "version")) {
        mudlog(NRM, ADMLVL_GOD, true, "%s", DG_SCRIPT_VERSION);
    } else {
        // If we reach here, we assume it's a command that should be executed.
        switch(proto->attach_type) {
            case MOB_TRIGGER:
                command_interpreter((char_data *)owner, (char*)full_command.c_str());
                break;
            case OBJ_TRIGGER:
                obj_command_interpreter((obj_data *)owner, (char*)full_command.c_str());
                break;
            case WLD_TRIGGER:
                wld_command_interpreter((room_data *)owner, (char*)full_command.c_str());
                break;
        }
    }
}