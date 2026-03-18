/* ************************************************************************
 *   File: handler.c                                     Part of CircleMUD *
 *  Usage: internal funcs: moving and finding chars/objs                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include <bsd/string.h>
#include "dbat/game/handler.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/Structure.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/db.hpp"
#include "dbat/game/interpreter.hpp"
//#include "dbat/game/spells.hpp"
#include "dbat/game/dg_scripts.hpp"
//#include "dbat/game/feats.hpp"
#include "dbat/game/races.hpp"
#include "dbat/game/class.hpp"
#include "dbat/game/fight.hpp"
//#include "dbat/game/races.hpp"
#include "dbat/game/act.informative.hpp"
//#include "dbat/game/act.misc.hpp"
#include "dbat/game/act.movement.hpp"
#include "dbat/game/players.hpp"
//#include "dbat/game/mobact.hpp"
#include "dbat/util/FilterWeak.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/Random.hpp"
#include "dbat/game/Parse.hpp"

#include "dbat/game/const/WearSlot.hpp"
#include "dbat/game/const/MaterialType.hpp"

/* local vars */
static std::unordered_set<Character *> extractions_pending;

/* external vars */

/* local functions */
static int apply_ac(Character *ch, int eq_pos);

static void update_object(Object *obj, int use);

/* external functions */
Object *find_vehicle_by_vnum(int vnum);

void remove_follower(Character *ch);

ACMD(do_return);

void perform_wear(Character *ch, Object *obj, int where);

void perform_remove(Character *ch, int pos);

int find_eq_pos(Character *ch, Object *obj, char *arg);

SPECIAL(shop_keeper);

/* For Getting An Intro Name */
const char *get_i_name(Character *ch, Character *vict)
{
    static char name[50];

    /* Read Introduction File */
    if (!vict || vict == ch)
    {
        return ("");
    }

    if (IS_NPC(ch) || IS_NPC(vict))
    {
        sprintf(name, "%s", RACE(vict));
        return name;
    }

    auto p = ch->player;

    auto found = p->dub_names.find(vict->player->id);
    if (found == p->dub_names.end()) {
        sprintf(name, "%s", RACE(vict));
        return name;
    }


    // print *found to name and return buf pointer.
    sprintf(name, "%s", found->second.c_str());
    return (name);
}

char *fname(const char *namelist)
{
    static char holder[READ_SIZE];
    char *point;

    for (point = holder; isalpha(*namelist); namelist++, point++)
        *point = *namelist;

    *point = '\0';

    return (holder);
}

/* Stock isname().  Leave this here even if you put in a newer  *
 * isname().  Used for OasisOLC.                                */
int is_name(std::string_view str, std::string_view namelist) {
    if (str.empty() || namelist.empty()) return 0;

    auto it  = namelist.begin();
    auto end = namelist.end();

    // helpers as inline lambdas (note the unsigned char cast to avoid UB with std::isalpha)
    auto is_alpha    = [](char c){ return std::isalpha(static_cast<unsigned char>(c)) != 0; };
    auto is_not_alpha= [&](char c){ return !is_alpha(c); };

    while (true) {
        it = std::find_if(it, end, is_alpha);              // start of next alpha token
        if (it == end) break;
        auto jt = std::find_if(it, end, is_not_alpha);     // end of token

        std::string_view token{ &*it, static_cast<size_t>(jt - it) };

        // (the all_of is technically redundant given how we found the token,
        //  but included per your preference)
        if (!token.empty()
            && std::all_of(token.begin(), token.end(), is_alpha)
            && boost::iequals(token, str)) {
            return 1;
        }
        it = jt; // continue scanning
    }
    return 0;
}

/* allow abbreviations */
#define WHITESPACE " \t"

static char *mystrsep(char **stringp, const char *delim)
{
    char *start = *stringp;
    char *p;

    p = (start != NULL) ? strpbrk(start, delim) : NULL;

    if (p == NULL)
    {
        *stringp = NULL;
    }
    else
    {
        *p = '\0';
        *stringp = p + 1;
    }

    return start;
}

bool isname(std::string_view needle, std::span<std::string_view> haystack) {
    for (const auto& kw : haystack) {
        if (std::all_of(kw.begin(), kw.end(), ::isdigit)) continue;
        if (boost::istarts_with(kw, needle)) {
            return true;
        }
    }

    return false;
}

bool isname(std::string_view needle, std::string_view haystack) {

    // if seeking is a number we return false...
    if (std::all_of(needle.begin(), needle.end(), ::isdigit)) {
        return false;
    }

    std::vector<std::string_view> keywords;
    boost::split(keywords, haystack, boost::is_space(), boost::token_compress_on);

    return isname(needle, keywords);
}



/* Insert an affect_type in a Character structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(Character *ch, struct affected_type *af)
{
    auto affected_alloc = new affected_type();

    if (!ch->affected)
    {
        characterSubscriptions.subscribe("affected", ch->shared_from_this());
    }
    *affected_alloc = *af;
    affected_alloc->next = ch->affected;
    ch->affected = affected_alloc;

}

/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(Character *ch, struct affected_type *af)
{
    struct affected_type *cmtemp;

    if (ch->affected == nullptr)
    {
        core_dump();
        return;
    }

    REMOVE_FROM_LIST(af, ch->affected, next, cmtemp);
    delete af;
    if (!ch->affected)
    {
        characterSubscriptions.unsubscribe("affected", ch->shared_from_this());
    }
}

/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(Character *ch, int type)
{
    struct affected_type *hjp, *next;

    for (hjp = ch->affected; hjp; hjp = next)
    {
        next = hjp->next;
        if (hjp->type == type)
            affect_remove(ch, hjp);
    }
}

/* Call affect_remove with every spell of spelltype "skill" */
void affectv_from_char(Character *ch, int type)
{
    struct affected_type *hjp, *next;

    for (hjp = ch->affectedv; hjp; hjp = next)
    {
        next = hjp->next;
        if (hjp->type == type)
            affectv_remove(ch, hjp);
    }
}

/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(Character *ch, int type)
{
    struct affected_type *hjp;

    for (hjp = ch->affected; hjp; hjp = hjp->next)
        if (hjp->type == type)
            return (true);

    return (false);
}

/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affectedv_by_spell(Character *ch, int type)
{
    struct affected_type *hjp;

    for (hjp = ch->affectedv; hjp; hjp = hjp->next)
        if (hjp->type == type)
            return (true);

    return (false);
}

void affect_join(Character *ch, struct affected_type *af,
                 bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
    struct affected_type *hjp, *next;
    bool found = false;

    for (hjp = ch->affected; !found && hjp; hjp = next)
    {
        next = hjp->next;

        if ((hjp->type == af->type) && (hjp->location == af->location))
        {
            if (add_dur)
                af->duration += hjp->duration;
            if (avg_dur)
                af->duration /= 2;

            if (add_mod)
                af->modifier += hjp->modifier;
            if (avg_mod)
                af->modifier /= 2;

            affect_remove(ch, hjp);
            affect_to_char(ch, af);
            found = true;
        }
    }
    if (!found)
        affect_to_char(ch, af);
    characterSubscriptions.subscribe("affected", ch->shared_from_this());
}

/* Return the effect of a piece of armor in position eq_pos */
static int apply_ac(Character *ch, int eq_pos)
{
    if (GET_EQ(ch, eq_pos) == nullptr)
    {
        core_dump();
        return (0);
    }

    if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
        return (0);

    /* The following code is an example of how to make the WEAR_ position of the
     * armor apply MORE AC value based on 'factor' then it's assigned value.
     * IE: An object with an AC value of 5 and a factor of 3 really gives 15 AC not 5.

    int factor;

    switch (eq_pos) {

    case WEAR_BODY:
      factor = 3;
      break;
    case WEAR_HEAD:
    case WEAR_LEGS:
      factor = 1;
      break;
    default:
      factor = 1;
      break;
    }

    return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), VAL_ARMOR_APPLYAC)); */
    return (GET_OBJ_VAL(GET_EQ(ch, eq_pos), VAL_ARMOR_APPLYAC));
}

int invalid_align(Character *ch, Object *obj)
{
    if (obj->not_alignment.get(MoralAlign::evil) && IS_EVIL(ch))
        return true;
    if (obj->not_alignment.get(MoralAlign::good) && IS_GOOD(ch))
        return true;
    if (obj->not_alignment.get(MoralAlign::neutral) && IS_NEUTRAL(ch))
        return true;
    return false;
}

void equip_char(Character *ch, Object *obj, int pos)
{
    if (pos < 0 || pos >= NUM_WEARS)
    {
        core_dump();
        return;
    }

    if (GET_EQ(ch, pos))
    {
        basic_mud_log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
                      obj->getShortDescription());
        return;
    }
    if (auto c = obj->getCarriedBy())
    {
        basic_mud_log("SYSERR: EQUIP: Obj is carried_by when equip.");
        return;
    }
    if (auto r = obj->getRoom())
    {
        basic_mud_log("SYSERR: EQUIP: Obj is in_room when equip.");
        return;
    }

    if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj))
    {
        act("You stop wearing $p as something prevents you.", false, ch, obj, nullptr, TO_CHAR);
        act("$n stops wearing $p as something prevents $m.", false, ch, obj, nullptr, TO_ROOM);
        /* Changed to drop in inventory instead of the ground. */
        ch->addToInventory(obj);
        return;
    }

    ch->addToEquip(obj, pos);
}

Object *unequip_char(Character *ch, int pos)
{
    int j;
    Object *obj;

    if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == nullptr)
    {
        core_dump();
        return (nullptr);
    }

    obj = GET_EQ(ch, pos);

    ch->removeFromEquip(pos);

    return (obj);
}

int get_number(char **name)
{
    int i;
    char *ppos;
    char number[MAX_INPUT_LENGTH];

    *number = '\0';

    if ((ppos = strchr((char *)*name, '.')))
    {
        *ppos++ = '\0';
        strlcpy(number, *name, sizeof(number));
        strcpy((char *)*name, ppos); /* strcpy: OK (always smaller) */

        for (i = 0; *(number + i); i++)
            if (!isdigit(*(number + i)))
                return (0);

        return (atoi(number));
    }
    return (1);
}

/* search the entire world for an object number, and return a pointer  */
Object *get_obj_num(obj_rnum nr)
{
    return objectSubscriptions.first(fmt::format("vnum_{}", nr));
}

/* search a room for a char, and return a pointer if found..  */
Character *get_char_room(char *name, int *number, room_rnum room)
{
    Character *i;
    int num;

    if (!number)
    {
        number = &num;
        num = get_number(&name);
    }

    if (*number == 0)
        return nullptr;

    auto people = get_room(room)->getPeople().snapshot_weak();
    for (auto i : dbat::util::filter_raw(people))
    {
        if (isname(name, i->getName()))
            if (--(*number) == 0)
                return (i);
    }

    return nullptr;
}

/* search all over the world for a char num, and return a pointer if found */
Character *get_char_num(mob_rnum nr)
{
    return characterSubscriptions.first(fmt::format("vnum_{}", nr));
}

/* Extract an object from the world */
void extract_obj(Object *obj)
{
    Object *temp;
    Character *ch;
    obj->clearLocation();

    /* Get rid of the contents of the object, as well. */
    if (auto trash = GET_FELLOW_WALL(obj); trash && GET_OBJ_VNUM(obj) == 79)
    {
        GET_FELLOW_WALL(obj) = nullptr;
        GET_FELLOW_WALL(trash) = nullptr;
        extract_obj(trash);
    }

    if (auto obj2 = GET_OBJ_POSTED(obj); obj2)
    {
        GET_OBJ_POSTED(obj2) = nullptr;
        GET_OBJ_POSTTYPE(obj2) = 0;
        GET_OBJ_POSTED(obj) = nullptr;
    }

    if (TARGET(obj))
    {
        TARGET(obj) = nullptr;
    }
    if (USER(obj))
    {
        USER(obj) = nullptr;
    }

    auto con = obj->getInventory();
    for (auto o : dbat::util::filter_raw(con))
        extract_obj(o);

    obj->deactivate();

    auto id = obj->id;

    Object::registry.erase(id);
}

static void update_object(Object *obj, int use)
{
    if (!obj)
        return;
    /* dont update objects with a timer trigger */
    if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && (GET_OBJ_TIMER(obj) > 0))
        obj->modBaseStat("timer", -use);
    auto con = obj->getInventory();
    for (auto o : dbat::util::filter_raw(con))
    {
        update_object(o, use);
    }
}

void update_char_objects(Character *ch)
{
    int i, j;

    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i))
        {
            if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS) > 0 &&
                GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_TIME) <= 0)
            {
                j = MOD_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS, -1);
                SET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_TIME, 3);
                if (j == 1)
                {
                    ch->sendText("Your light begins to flicker and fade.\r\n");
                    act("$n's light begins to flicker and fade.", false, ch, nullptr, nullptr, TO_ROOM);
                }
                else if (j == 0)
                {
                    ch->sendText("Your light sputters out and dies.\r\n");
                    act("$n's light sputters out and dies.", false, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            else if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_HOURS) > 0)
            {
                MOD_OBJ_VAL(GET_EQ(ch, i), VAL_LIGHT_TIME, -1);
            }
            update_object(GET_EQ(ch, i), 2);
        }
    auto con = ch->getInventory();
    for (auto o : dbat::util::filter_raw(con))
        update_object(o, 1);
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(Character *ch)
{
    Character *k, *temp;
    Object *chair;
    struct descriptor_data *d;
    Object *obj;
    int i;

    if (!ch->location)
    {
        basic_mud_log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
                      GET_NAME(ch), __FILE__);
        shutdown_game(1);
    }

    /*
     * We're booting the character of someone who has switched so first we
     * need to stuff them back into their own body.  This will set ch->desc
     * we're checking below this loop to the proper value.
     */
    if (!IS_NPC(ch) && !ch->desc)
    {
        for (d = descriptor_list; d; d = d->next)
            if (d->original == ch)
            {
                do_return(d->character, nullptr, 0, 0);
                break;
            }
    }

    if (ch->desc)
    {
        if (ch->desc->original)
            do_return(ch, nullptr, 0, 0);
    }
    /* On with the character's assets... */

    if (ch->followers || ch->master)
        die_follower(ch);

    if (auto original = GET_ORIGINAL(ch); original)
    {
        auto shared = ch->shared_from_this();
        original->clones.remove(shared);
    }

    ch->mergeClones();

    purge_homing(ch);

    if (auto ml = MINDLINK(ch))
    {
        MINDLINK(ml) = nullptr;
        MINDLINK(ch) = nullptr;
    }

    if (auto grap = GRAPPLING(ch))
    {
        act("@WYou stop grappling with @C$N@W!@n", true, ch, nullptr, grap, TO_CHAR);
        act("@C$n@W stops grappling with @c$N@W!@n", true, ch, nullptr, grap, TO_ROOM);
        grap->setBaseStat<int>("grapple_type", -1);
        GRAPPLED(grap) = nullptr;
        GRAPPLING(ch) = nullptr;
        ch->setBaseStat<int>("grapple_type", -1);
    }
    if (auto grap = GRAPPLED(ch))
    {
        act("@WYou stop being grappled with by @C$N@W!@n", true, ch, nullptr, grap, TO_CHAR);
        act("@C$n@W stops being grappled with by @c$N@W!@n", true, ch, nullptr, grap, TO_ROOM);
        grap->setBaseStat<int>("grapple_type", -1);
        GRAPPLING(grap) = nullptr;
        GRAPPLED(ch) = nullptr;
        ch->setBaseStat<int>("grapple_type", -1);
    }

    if (CARRYING(ch))
    {
        carry_drop(ch, 3);
    }

    if (CARRIED_BY(ch))
    {
        carry_drop(CARRIED_BY(ch), 3);
    }

    if (ch->poisonby)
    {
        auto shared = ch->shared_from_this();
        ch->poisonby->poisoned.remove(shared);
        ch->poisonby = nullptr;
    }

    ch->poisoned.for_each([&](Character* c) {
        c->poisonby = nullptr;
    });
    ch->poisoned.clear();

    if (auto drg = DRAGGING(ch))
    {
        act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, drg, TO_CHAR);
        act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, drg, TO_ROOM);
        DRAGGED(drg) = nullptr;
        DRAGGING(ch) = nullptr;
    }

    if (auto dr = DRAGGED(ch))
    {
        act("@WYou stop being dragged by @C$N@W!@n", true, ch, nullptr, dr, TO_CHAR);
        act("@C$n@W stops being dragged by @c$N@W!@n", true, ch, nullptr, dr, TO_ROOM);
        DRAGGING(dr) = nullptr;
        DRAGGED(ch) = nullptr;
    }

    if (auto def = GET_DEFENDER(ch))
    {
        GET_DEFENDING(def) = nullptr;
        GET_DEFENDER(ch) = nullptr;
    }
    if (auto def = GET_DEFENDING(ch))
    {
        GET_DEFENDER(def) = nullptr;
        GET_DEFENDING(ch) = nullptr;
    }

    if (auto blk = BLOCKED(ch))
    {
        BLOCKS(blk) = nullptr;
        BLOCKED(ch) = nullptr;
    }
    if (auto bl = BLOCKS(ch))
    {
        BLOCKED(bl) = nullptr;
        BLOCKS(ch) = nullptr;
    }
    if (auto ab = ABSORBING(ch))
    {
        ABSORBBY(ab) = nullptr;
        ABSORBING(ch) = nullptr;
    }
    if (auto aby = ABSORBBY(ch))
    {
        ABSORBING(aby) = nullptr;
        ABSORBBY(ch) = nullptr;
    }

    /* transfer objects to room, if any */
    if (IS_NPC(ch))
    {
        auto con = ch->getInventory();
        for (auto obj : dbat::util::filter_shared(con))
        {
            obj->clearLocation();
            obj->moveToLocation(ch);
        }

        /* transfer equipment to room, if any */
        for (auto &[slot, o] : ch->getEquipment())
        {
            unequip_char(ch, slot);
            o->moveToLocation(ch);
        }
    }

    if (FIGHTING(ch))
        stop_fighting(ch);
    auto subs = characterSubscriptions.all("combatSystem");
    for (auto k : dbat::util::filter_raw(subs))
    {
        if (FIGHTING(k) == ch)
            stop_fighting(k);
    }



    /* If there's a descriptor, they're in the menu now. */
    if (ch->desc)
    {
        ch->desc->connected = CON_QUITGAME;
    }

    ch->deactivate();
    if (IS_NPC(ch))
    {
        ch->leaveLocation();
        auto id = ch->id;
        Character::registry.erase(id);
    }
}

/*
 * Q: Why do we do this?
 * A: Because trying to iterate over the character
 *    list with 'ch = ch->next' does bad things if
 *    the current character happens to die. The
 *    trivial workaround of 'vict = next_vict'
 *    doesn't work if the _next_ person in the list
 *    gets killed, for example, by an area spell.
 *
 * Q: Why do we leave them on the character_list?
 * A: Because code doing 'vict = vict->next' would
 *    get really confused otherwise.
 */
void extract_char(Character *ch)
{
    if (!ch->active)
    {
        basic_mud_log("Attempt to extract an inactive character.");
        return;
    }

    extractions_pending.insert(ch);

    if(auto foll = ch->followers)
        foll.for_each([&](auto f) {
            if (IS_NPC(f) && AFF_FLAGGED(f, AFF_CHARM) && (f->location == ch->location || IN_ROOM(ch) == 1))
                {
                    /* transfer objects to char, if any */
                    auto con = f->getInventory();
                    for (auto obj : dbat::util::filter_shared(con))
                    {
                        obj->clearLocation();
                        ch->addToInventory(obj);
                    }

                    /* transfer equipment to char, if any */
                    for (auto &[slot, obj] : f->getEquipment())
                    {
                        auto un = unequip_char(f, slot);
                        ch->addToInventory(un);
                    }

                    extract_char(f);
                }
        });
}

/*
 * I'm not particularly pleased with the MOB/PLR
 * hoops that have to be jumped through but it
 * hardly calls for a completely new variable.
 * Ideally it would be its own list, but that
 * would change the '->next' pointer, potentially
 * confusing some code. Ugh. -gg 3/15/2001
 *
 * NOTE: This doesn't handle recursive extractions.
 */
void extract_pending_chars(uint64_t heartBeat, double deltaTime)
{
    auto pending = extractions_pending;

    for (auto ch : pending)
    {
        extractions_pending.erase(ch);
        extract_char_final(ch);
    }
}

/* ***********************************************************************
 * Here follows high-level versions of some earlier routines, ie functions*
 * which incorporate the actual player-data                               *.
 *********************************************************************** */

Character *get_player_vis(Character *ch, char *name, int *number, int inroom)
{
    Character *i;
    int num;

    if (!number)
    {
        number = &num;
        num = get_number(&name);
    }

    auto ac = characterSubscriptions.all("active");
    for (auto i : dbat::util::filter_raw(ac))
    {
        if (IS_NPC(i))
            continue;
        if (inroom == FIND_CHAR_ROOM && i->location != ch->location)
            continue;
        if (GET_ADMLEVEL(ch) < 1 && GET_ADMLEVEL(i) < 1 && !IS_NPC(ch) && !IS_NPC(i))
        {
            if (!boost::iequals(RACE(i), name) && !strstr(RACE(i), name))
            {
                if (readIntro(ch, i) == 1)
                {
                    if (!boost::iequals(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name))
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }
        }
        if ((GET_ADMLEVEL(ch) >= 1 || GET_ADMLEVEL(i) >= 1 || IS_NPC(ch) || IS_NPC(i)))
        {
            if (!boost::iequals(i->getName(), name) && !strstr(i->getName(), name))
            {
                if (!boost::iequals(RACE(i), name) && !strstr(RACE(i), name))
                {
                    if (!IS_NPC(ch) && !IS_NPC(i) && readIntro(ch, i) == 1)
                    {
                        if (!boost::iequals(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
    if (!ch->canSee(i))
            continue;
        if (--(*number) != 0)
            continue;
        return (i);
    }

    return (nullptr);
}

Character *get_char_room_vis(Character *ch, char *name, int *number)
{
    Character *i;
    int num;

    if (!number)
    {
        number = &num;
        num = get_number(&name);
    }

    /* JE 7/18/94 :-) :-) */
    if (boost::iequals(name, "self") || boost::iequals(name, "me"))
        return (ch);

    /* 0.<name> means PC with name */
    if (*number == 0)
        return (get_player_vis(ch, name, nullptr, FIND_CHAR_ROOM));
    auto people = ch->location.getPeople();
    for (auto i : dbat::util::filter_raw(people))
    {
        if (boost::iequals(name, "last") && LASTHIT(i) != 0 && LASTHIT(i) == GET_IDNUM(ch))
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (isname(name, i->getName()) && (IS_NPC(i) || IS_NPC(ch) || GET_ADMLEVEL(i) > 0 || GET_ADMLEVEL(ch) > 0) &&
                 i != ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (isname(name, i->getName()) && i == ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (!IS_NPC(i) && !IS_NPC(ch) && boost::iequals(get_i_name(ch, i), CAP(name)) && i != ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (!IS_NPC(i) && !IS_NPC(ch) && strstr(get_i_name(ch, i), CAP(name)) && i != ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (!IS_NPC(i) && !(strcmp(RACE(i), CAP(name))) && i != ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (!IS_NPC(i) && strstr(RACE(i), CAP(name)) && i != ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (!IS_NPC(i) && !(strcmp(RACE(i), name)) && i != ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
        else if (!IS_NPC(i) && strstr(RACE(i), name) && i != ch)
        {
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
        }
    }
    return (nullptr);
}

Character *get_char_world_vis(Character *ch, char *name, int *number)
{
    Character *i;
    int num;

    if (!number)
    {
        number = &num;
        num = get_number(&name);
    }

    if ((i = get_char_room_vis(ch, name, number)))
        return (i);

    if (*number == 0)
        return get_player_vis(ch, name, nullptr, 0);

    auto ac = characterSubscriptions.all("active");
    for (auto i : dbat::util::filter_raw(ac))
    {
        if (ch->location == i->location)
            continue;
        if (GET_ADMLEVEL(ch) < 1 && GET_ADMLEVEL(i) < 1 && !IS_NPC(ch) && !IS_NPC(i))
        {
            if (!boost::iequals(RACE(i), name) && !strstr(RACE(i), name))
            {
                if (readIntro(ch, i) == 1)
                {
                    if (!boost::iequals(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name))
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }
        }
        if ((GET_ADMLEVEL(ch) >= 1 || GET_ADMLEVEL(i) >= 1 || IS_NPC(ch) || IS_NPC(i)))
        {
            if (!boost::iequals(i->getName(), name) && !strstr(i->getName(), name))
            {
                if (!boost::iequals(RACE(i), name) && !strstr(RACE(i), name))
                {
                    if (!IS_NPC(ch) && !IS_NPC(i) && readIntro(ch, i) == 1)
                    {
                        if (!boost::iequals(get_i_name(ch, i), name) && !strstr(get_i_name(ch, i), name))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
        if (--(*number) != 0)
            continue;

        return (i);
    }
    return (nullptr);
}

Character *get_char_vis(Character *ch, char *name, int *number, int where)
{
    if (where == FIND_CHAR_ROOM)
        return get_char_room_vis(ch, name, number);
    else if (where == FIND_CHAR_WORLD)
        return get_char_world_vis(ch, name, number);
    else
        return (nullptr);
}

Object *get_obj_in_list_vis(Character *ch, const char *name, int *number, const std::vector<std::weak_ptr<Object>> &list)
{
    int num;

    if (!number)
    {
        number = &num;
        num = get_number((char**)&name);
    }

    if (*number == 0)
        return nullptr;

    for (auto i : dbat::util::filter_raw(list))
    {
        if (isname(name, i->getName()))
            if (ch->canSee(i) || (GET_OBJ_TYPE(i) == ITEM_LIGHT))
                if (--(*number) == 0)
                    return i;
    }

    return nullptr;
}

Structure *get_structure_in_list_vis(Character *ch, std::string_view name, int *number, const std::vector<std::weak_ptr<Structure>> &list)
{
    int num;

    if (!number)
    {
        number = &num;
        num = get_number((char**)&name);
    }

    if (*number == 0)
        return nullptr;

    for (auto i : dbat::util::filter_raw(list))
    {
        if (isname(name, i->getName()))
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return i;
    }

    return nullptr;
}

/* search the entire world for an object, and return a pointer  */
Object *get_obj_vis(Character *ch, char *name, int *number)
{
    Object *i;
    int num;

    if (!number)
    {
        number = &num;
        num = get_number(&name);
    }

    if (*number == 0)
        return (nullptr);

    /* scan items carried */
    if ((i = get_obj_in_list_vis(ch, name, number, ch->getInventory())))
        return (i);

    /* scan room */
    if ((i = get_obj_in_list_vis(ch, name, number, ch->location.getObjects())))
        return (i);

    /* ok.. no luck yet. scan the entire obj list   */
    auto ao = objectSubscriptions.all("active");
    for (auto i : dbat::util::filter_raw(ao))
    {
        if (isname(name, i->getName()))
            if (ch->canSee(i))
                if (--(*number) == 0)
                    return (i);
    }

    return (nullptr);
}

Object *get_obj_in_equip_vis(Character *ch, char *arg, int *number, const std::map<int, Object *> &equipment)
{
    int j, num;

    if (!number)
    {
        number = &num;
        num = get_number(&arg);
    }

    if (*number == 0)
        return (nullptr);

    for (const auto &[slot, obj] : equipment)
    if (obj && ch->canSee(obj) && isname(arg, obj->getName()))
            if (--(*number) == 0)
                return obj;

    return nullptr;
}

int get_obj_pos_in_equip_vis(Character *ch, char *arg, int *number, const std::map<int, Object *> &equipment)
{
    int j, num;

    if (!number)
    {
        number = &num;
        num = get_number(&arg);
    }

    if (*number == 0)
        return -1;

    for (const auto &[slot, obj] : equipment)
    if (obj && ch->canSee(obj) && isname(arg, obj->getName()))
            if (--(*number) == 0)
                return slot;

    return -1;
}

const char *money_desc(int amount)
{
    int cnt;
    struct
    {
        int limit;
        const char *description;
    } money_table[] = {
        {1, "a single zenni"},
        {10, "a tiny pile of zenni"},
        {20, "a handful of zenni"},
        {75, "a little pile of zenni"},
        {150, "a small pile of zenni"},
        {250, "a pile of zenni"},
        {500, "a big pile of zenni"},
        {1000, "a large heap of zenni"},
        {5000, "a huge mound of zenni"},
        {10000, "an enormous mound of zenni"},
        {15000, "a small mountain of zenni"},
        {20000, "a mountain of zenni"},
        {25000, "a huge mountain of zenni"},
        {50000, "an enormous mountain of zenni"},
        {0, nullptr},
    };

    if (amount <= 0)
    {
        basic_mud_log("SYSERR: Try to create negative or 0 money (%d).", amount);
        return (nullptr);
    }

    for (cnt = 0; money_table[cnt].limit; cnt++)
        if (amount <= money_table[cnt].limit)
            return (money_table[cnt].description);

    return ("an absolutely colossal mountain of zenni");
}

Object *create_money(int amount)
{
    char buf[200];
    int y;

    if (amount <= 0)
    {
        basic_mud_log("SYSERR: Try to create negative or 0 money. (%d)", amount);
        return (nullptr);
    }
    auto obj = create_obj();
    auto &ex = obj->extra_descriptions.emplace_back();
    ex.first = "zenni money";

    obj->name = "zenni money";
    if (amount == 1)
    {
        obj->short_description = "a single zenni";
        obj->room_description = "One miserable zenni is lying here";
        ex.second = "It's just one miserable little zenni.";
    }
    else
    {
        obj->short_description = money_desc(amount);
        snprintf(buf, sizeof(buf), "%s is lying here", money_desc(amount));
        obj->room_description = CAP(buf);

        if (amount < 10)
            snprintf(buf, sizeof(buf), "There is %d zenni.", amount);
        else if (amount < 100)
            snprintf(buf, sizeof(buf), "There is about %d zenni.", 10 * (amount / 10));
        else if (amount < 1000)
            snprintf(buf, sizeof(buf), "It looks to be about %d zenni.", 100 * (amount / 100));
        else if (amount < 100000)
            snprintf(buf, sizeof(buf), "You guess there is, maybe, %d zenni.",
                     1000 * ((amount / 1000) + Random::get<int>(0, (amount / 1000))));
        else
            strcpy(buf, "There are is LOT of zenni."); /* strcpy: OK (is < 200) */
        ex.second = buf;
    }

    obj->type_flag = ItemType::money;
    SET_OBJ_VAL(obj, VAL_ALL_MATERIAL, MATERIAL_GOLD);
    SET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH, 100);
    SET_OBJ_VAL(obj, VAL_ALL_HEALTH, 100);
    obj->wear_flags.set(ITEM_WEAR_TAKE, true);
    SET_OBJ_VAL(obj, VAL_MONEY_SIZE, amount);
    obj->setBaseStat("cost", amount);

    return (obj);
}

/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be nullptr if no character was found, otherwise points
 * **tar_obj Will be nullptr if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(const char *arg, bitvector_t bitvector, Character *ch,
                 Character **tar_ch, Object **tar_obj)
{
    int i, found, number;
    char name_val[MAX_INPUT_LENGTH];
    char *name = name_val;

    *tar_ch = nullptr;
    *tar_obj = nullptr;

    one_argument(arg, name);

    if (!*name)
        return (0);
    if (!(number = get_number(&name)))
        return (0);

    if (IS_SET(bitvector, FIND_CHAR_ROOM))
    { /* Find person in room */
        if ((*tar_ch = get_char_room_vis(ch, name, &number)))
            return (FIND_CHAR_ROOM);
    }

    if (IS_SET(bitvector, FIND_CHAR_WORLD))
    {
        if ((*tar_ch = get_char_world_vis(ch, name, &number)))
            return (FIND_CHAR_WORLD);
    }

    if (IS_SET(bitvector, FIND_OBJ_EQUIP))
    {
        for (found = false, i = 0; i < NUM_WEARS && !found; i++)
            if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->getName()) && --number == 0)
            {
                *tar_obj = GET_EQ(ch, i);
                found = true;
            }
        if (found)
            return (FIND_OBJ_EQUIP);
    }

    if (IS_SET(bitvector, FIND_OBJ_INV))
    {
        if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, ch->getInventory())))
            return (FIND_OBJ_INV);
    }

    if (IS_SET(bitvector, FIND_OBJ_ROOM))
    {
        if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, ch->location.getObjects())))
            return (FIND_OBJ_ROOM);
    }

    if (IS_SET(bitvector, FIND_OBJ_WORLD))
    {
        if ((*tar_obj = get_obj_vis(ch, name, &number)))
            return (FIND_OBJ_WORLD);
    }

    return (0);
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
    if (!strcmp(arg, "all"))
        return (FIND_ALL);
    else if (!strncmp(arg, "all.", 4))
    {
        strcpy(arg, arg + 4); /* strcpy: OK (always less) */
        return (FIND_ALLDOT);
    }
    else
        return (FIND_INDIV);
}

void affectv_to_char(Character *ch, struct affected_type *af)
{
    struct affected_type *affected_alloc;

    CREATE(affected_alloc, struct affected_type, 1);

    if (!ch->affectedv)
    {
        ch->next_affectv = affectv_list;
        affectv_list = ch;
    }
    *affected_alloc = *af;
    affected_alloc->next = ch->affectedv;
    ch->affectedv = affected_alloc;

}

void affectv_remove(Character *ch, struct affected_type *af)
{
    struct affected_type *cmtemp;

    if (ch->affectedv == nullptr)
    {
        core_dump();
        return;
    }


    REMOVE_FROM_LIST(af, ch->affectedv, next, cmtemp);
    free(af);
    if (!ch->affectedv)
    {
        characterSubscriptions.unsubscribe("affectedv", ch);
    }
}

void affectv_join(Character *ch, struct affected_type *af,
                  bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
    struct affected_type *hjp, *next;
    bool found = false;

    for (hjp = ch->affectedv; !found && hjp; hjp = next)
    {
        next = hjp->next;

        if ((hjp->type == af->type) && (hjp->location == af->location))
        {
            if (add_dur)
                af->duration += hjp->duration;
            if (avg_dur)
                af->duration /= 2;

            if (add_mod)
                af->modifier += hjp->modifier;
            if (avg_mod)
                af->modifier /= 2;

            affectv_remove(ch, hjp);
            affectv_to_char(ch, af);
            found = true;
        }
    }
    if (!found)
        affectv_to_char(ch, af);
    characterSubscriptions.subscribe("affectedv", ch);
}

int is_better(Object *object, Object *object2)
{
    int value1 = 0, value2 = 0;

    switch (GET_OBJ_TYPE(object))
    {
    case ITEM_ARMOR:
        value1 = GET_OBJ_VAL(object, VAL_ARMOR_APPLYAC);
        value2 = GET_OBJ_VAL(object2, VAL_ARMOR_APPLYAC);
        break;
    case ITEM_WEAPON:
        value1 = (1 + GET_OBJ_VAL(object, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(object, VAL_WEAPON_DAMDICE);
        value2 = (1 + GET_OBJ_VAL(object2, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(object2, VAL_WEAPON_DAMDICE);
        break;
    default:
        break;
    }

    if (value1 > value2)
        return 1;
    else
        return 0;
}

/* check and see if this item is better */
void item_check(Object *object, Character *ch)
{
    int where = 0;

    if (IS_HUMANOID(ch) && !(GET_MOB_SPEC(ch) == shop_keeper))
    {
        if (invalid_align(ch, object) || invalid_class(ch, object))
            return;

        switch (GET_OBJ_TYPE(object))
        {
        case ITEM_WEAPON:
            if (!GET_EQ(ch, WEAR_WIELD1))
            {
                perform_wear(ch, object, WEAR_WIELD1);
            }
            else
            {
                if (is_better(object, GET_EQ(ch, WEAR_WIELD1)))
                {
                    perform_remove(ch, WEAR_WIELD1);
                    perform_wear(ch, object, WEAR_WIELD1);
                }
            }
            break;
        case ITEM_ARMOR:
        case ITEM_WORN:
            where = find_eq_pos(ch, object, nullptr);
            if (!GET_EQ(ch, where))
            {
                perform_wear(ch, object, where);
            }
            else
            {
                if (is_better(object, GET_EQ(ch, where)))
                {
                    perform_remove(ch, where);
                    perform_wear(ch, object, where);
                }
            }

            break;
        default:
            break;
        }
    }
}

std::pair<int, std::string_view> splitSearchNumber(std::string_view txt)
{
    auto s = boost::trim_copy(txt);

    const auto dot = s.find('.');
    if (dot == std::string_view::npos) {
        // No ordinal prefix: default to 1st match
        return {1, s};
    }

    auto num_sv  = boost::trim_copy(s.substr(0, dot));

    auto after   = boost::trim_copy(s.substr(dot + 1));

    if (num_sv.empty())
        return {0, after};

    auto numres = parseNumber<int>(num_sv, "search number", 1);
    if(!numres) {
        return {0, after};
    }
    return {numres.value(), after};
}
