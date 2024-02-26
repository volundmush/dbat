/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/mobact.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/spells.h"
#include "dbat/shop.h"
#include "dbat/combat.h"
#include "dbat/act.offensive.h"
#include "dbat/act.movement.h"
#include "dbat/act.other.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/spec_procs.h"
#include "dbat/class.h"
#include "dbat/entity.h"

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

/* local functions */
static int player_present(Character *ch) {

    Character *vict, *next_v;
    int found = false;

    if (IN_ROOM(ch) == NOWHERE)
        return 0;

    for (auto vict : ch->getRoom()->getPeople()) {
        if (!IS_NPC(vict)) {
            found = true;
            break;
        }
    }

    return (found);
}

static const std::vector<std::string> scavengerTalk = {
    "$n@W says, '@CFinders keepers, losers weepers.@W'@n",
    "$n@W says, '@CPeople always leaving their garbage JUST LYING AROUND. The nerve....@W'@n",
    "$n@W says, '@CWho would leave this here? Oh well..@W'@n",
    "$n@W says, '@CI always wanted one of these.@W'@n",
    "$n@W looks around quickly to see if anyone is paying attention.@n"
};

void mobile_activity_specials(uint64_t heartPulse, double deltaTime) {
    if(no_specials) return;
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(!MOB_FLAGGED(ch, MOB_SPEC)) continue;
        if (mob_index[GET_MOB_RNUM(ch)].func == nullptr) {
            basic_mud_log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
                GET_NAME(ch), GET_MOB_VNUM(ch));
            ch->clearFlag(FlagType::NPC, MOB_SPEC);
            auto &mp = mob_proto[ch->getVN()];
        } else {
            char actbuf[MAX_INPUT_LENGTH] = "";
            (mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, actbuf);
        }
    }
}

void mobile_activity_scavenger(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(MOB_FLAGGED(ch, MOB_NOSCAVENGER)) continue;
        if(MOB_FLAGGED(ch, MOB_NOKILL)) continue;
        if(!AWAKE(ch)) continue;
        if(!IS_HUMANOID(ch)) continue;
        if(FIGHTING(ch)) continue;
        if(!player_present(ch)) continue;
        if(axion_dice(0) > 118) continue;

        if (auto contents = ch->getRoom()->getInventory(); !contents.empty() && rand_number(1, 100) >= 95) {
            int max = 1;
            Object* best_obj = nullptr;
            for (auto obj : contents)
                if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
                    best_obj = obj;
                    max = GET_OBJ_COST(obj);
                }
            if (best_obj != nullptr && CAN_GET_OBJ(ch, best_obj) && GET_OBJ_TYPE(best_obj) != ITEM_BED &&
                !GET_OBJ_POSTED(best_obj) && !OBJ_FLAGGED(best_obj, ITEM_NOPICKUP)) {
                    auto line = Random::get(scavengerTalk);
                    act(line->c_str(), true, ch, nullptr, nullptr, TO_ROOM);
                    perform_get_from_room(ch, best_obj);
                }
        }

    }
}

void mobile_activity_movement(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(rand_number(1,2) != 2) continue;
        if(!AWAKE(ch)) continue;
        if(MOB_FLAGGED(ch, MOB_SENTINEL)) continue;
        if(IS_AFFECTED(ch, AFF_PARALYZE)) continue;
        if(!block_calc(ch)) continue;
        if(GET_POS(ch) != POS_STANDING) continue;
        if(AFF_FLAGGED(ch, AFF_TAMED)) continue;
        if(ABSORBBY(ch)) continue;

        std::vector<int> availableDirections;
        //Check if door is a viable movement
        auto r = ch->getRoom();
        for(auto &[i, ex] : r->getExits()) {
            if(ex->checkFlag(FlagType::Exit, EX_CLOSED)) continue;
            auto dest = reg.try_get<Destination>(ex->ent);
            if(!dest) continue;
            if(flags::check(dest->target, FlagType::Room, ROOM_NOMOB) || flags::check(dest->target, FlagType::Room, ROOM_DEATH)) continue;
            if(auto room = reg.try_get<Room>(dest->target); room) {
                if(MOB_FLAGGED(ch, MOB_STAY_ZONE) && room->zone != r->zone) continue;
            }
            availableDirections.push_back(i);
        }
        if(!availableDirections.empty()) {
            auto door = Random::get(availableDirections);
            ch->moveInDirection(*door, 1);
        }
    }
}

// The serious small count of such attacks means that we should instead iterate over the list of ki attacks that exist in rooms.
void mobile_activity_kirespond(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(!AWAKE(ch)) continue;
        if(MOB_FLAGGED(ch, MOB_NOKILL)) continue;
        if(FIGHTING(ch)) continue;

        for (auto hugeatk : ch->getRoom()->getInventory()) {

            if (GET_OBJ_VNUM(hugeatk) == 82 || GET_OBJ_VNUM(hugeatk) == 83) {
                if (USER(hugeatk) != nullptr) {
                    act("@W$n@R leaps at @C$N@R desperately!@n", true, ch, nullptr, USER(hugeatk), TO_ROOM);
                    act("@W$n@R leaps at YOU desperately!@n", true, ch, nullptr, USER(hugeatk), TO_VICT);
                    char tar[MAX_INPUT_LENGTH];
                    sprintf(tar, "%s", GET_NAME(USER(hugeatk)));
                    if (IS_HUMANOID(ch)) {
                        do_punch(ch, tar, 0, 0);
                    } else {
                        do_bite(ch, tar, 0, 0);
                    }
                }
            }
        }

    }
}

void mobile_activity_aggressive(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(!AWAKE(ch)) continue;
        if(!MOB_FLAGGED(ch, MOB_AGGRESSIVE)) continue;
        if(IS_AFFECTED(ch, AFF_PARALYZE)) continue;

        int spot_roll = rand_number(1, GET_LEVEL(ch) + 10);
        bool found = false;
        for (auto vict : ch->getRoom()->getPeople()) {
            if (vict == ch)
                continue;
            else if (FIGHTING(ch))
                continue;
            else if (!CAN_SEE(ch, vict))
                continue;
            else if (IS_NPC(vict))
                continue;
            else if (PRF_FLAGGED(vict, PRF_NOHASSLE))
                continue;
            else if (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && GET_ALIGNMENT(vict) < 50)
                continue;
            else if (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && GET_ALIGNMENT(vict) > -50)
                continue;
            else if (GET_LEVEL(vict) < 5)
                continue;
            else if (AFF_FLAGGED(vict, AFF_HIDE) && GET_SKILL(vict, SKILL_HIDE) > spot_roll)
                continue;
            else if (AFF_FLAGGED(vict, AFF_SNEAK) && GET_SKILL(vict, SKILL_MOVE_SILENTLY) > spot_roll)
                continue;
            else if (ch->aggtimer < 8)
                ch->aggtimer += 1;
            else {
                ch->aggtimer = 0;
                char tar[MAX_INPUT_LENGTH];

                sprintf(tar, "%s", GET_NAME(vict));
                if (IS_HUMANOID(ch)) {
                    if (!AFF_FLAGGED(vict, AFF_HIDE) && !AFF_FLAGGED(vict, AFF_SNEAK)) {
                        act("@w'I am going to get you!' @C$n@w shouts at you!@n", true, ch, nullptr, vict, TO_VICT);
                        act("@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", true, ch, nullptr, vict,
                            TO_NOTVICT);
                    } else {
                        act("@C$n@w notices YOU.\n@w'I am going to get you!' @C$n@w shouts at you!@n", true, ch,
                            nullptr, vict, TO_VICT);
                        act("@C$n@w notices @c$N@w.\n@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", true,
                            ch, nullptr, vict, TO_NOTVICT);
                    }
                    if (AFF_FLAGGED(vict, AFF_FLYING) && !AFF_FLAGGED(ch, AFF_FLYING) && IS_HUMANOID(ch) &&
                        GET_LEVEL(ch) > 10) {
                        do_fly(ch, nullptr, 0, 0);
                        continue;
                    }
                    if (!AFF_FLAGGED(vict, AFF_FLYING) && AFF_FLAGGED(ch, AFF_FLYING)) {
                        do_fly(ch, nullptr, 0, 0);
                        continue;
                    }
                    do_punch(ch, tar, 0, 0);
                }
                if (!IS_HUMANOID(ch)) {
                    if (AFF_FLAGGED(vict, AFF_FLYING) && !AFF_FLAGGED(ch, AFF_FLYING) && IS_HUMANOID(ch) &&
                        GET_LEVEL(ch) > 10) {
                        do_fly(ch, nullptr, 0, 0);
                        continue;
                    }
                    if (!AFF_FLAGGED(vict, AFF_FLYING) && AFF_FLAGGED(ch, AFF_FLYING)) {
                        do_fly(ch, nullptr, 0, 0);
                        continue;
                    }
                    if (!AFF_FLAGGED(vict, AFF_HIDE) && !AFF_FLAGGED(vict, AFF_SNEAK)) {
                        act("@C$n @wgrowls viciously at you!@n", true, ch, nullptr, vict, TO_VICT);
                        act("@C$n @wgrowls viciously at @c$N@w!@n", true, ch, nullptr, vict, TO_NOTVICT);
                    } else {
                        act("@C$n@w notices YOU.\n@C$n @wgrowls viciously at you!@n", true, ch, nullptr, vict,
                            TO_VICT);
                        act("@C$n@w notices @c$N@w.\n@C$n @wgrowls viciously at @c$N@w!@n", true, ch, nullptr, vict,
                            TO_NOTVICT);
                    }
                    do_bite(ch, tar, 0, 0);
                }
                /*hit(ch, vict, TYPE_UNDEFINED);*/
                found = true;
            }
        }

    }
}

void mobile_activity_clonehelp(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(!AWAKE(ch)) continue;
        auto original = GET_ORIGINAL(ch);
        if(!original) continue;
        if(rand_number(1, 5) < 4) continue;

        if (FIGHTING(original) && !FIGHTING(ch)) {
            char target[MAX_INPUT_LENGTH];
            auto targ = FIGHTING(original);

            sprintf(target, "%s", targ->getName().c_str());
            if (rand_number(1, 5) >= 4) {
                do_kick(ch, target, 0, 0);
            } else if (rand_number(1, 5) >= 4) {
                do_elbow(ch, target, 0, 0);
            } else {
                do_punch(ch, target, 0, 0);
            }
        }
    }
}

void mobile_activity_absorb_escape(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(!AWAKE(ch)) continue;
        if(!ABSORBBY(ch)) continue;
        if(rand_number(1, 5) < 4) continue;
        do_escape(ch, nullptr, 0, 0);
    }
}

void mobile_activity_wake(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(GET_POS(ch) != POS_SLEEPING) continue;
        if(rand_number(1, 3) != 3) continue;
        do_wake(ch, nullptr, 0, 0);
    }
}

void mobile_activity_taunt(uint64_t heartPulse, double deltaTime) {
    for(auto &&[ent, character, npc] : reg.view<Character, NonPlayerCharacter>(entt::exclude<Deleted>).each()) {
        auto ch = &character;
        if(!AWAKE(ch)) continue;
        if(!FIGHTING(ch)) continue;
        if(rand_number(1, 30) < 25) continue;
        mob_taunt(ch);
    }
}

void mobile_activity(uint64_t heartPulse, double deltaTime) {
    mobile_activity_specials(heartPulse, deltaTime);
    mobile_activity_scavenger(heartPulse, deltaTime);
    mobile_activity_movement(heartPulse, deltaTime);
    mobile_activity_kirespond(heartPulse, deltaTime);
    mobile_activity_aggressive(heartPulse, deltaTime);
    mobile_activity_clonehelp(heartPulse, deltaTime);
    mobile_activity_absorb_escape(heartPulse, deltaTime);
    mobile_activity_wake(heartPulse, deltaTime);
    mobile_activity_taunt(heartPulse, deltaTime);
}

static const std::vector<std::pair<std::string, std::string>> animalLand = {
    {"@C$n@W growls viciously at @c$N@W!@n","@C$n@W growls viciously at you!@n"},
    {"@C$n@W snaps $s jaws at @c$N@W!@n","@C$n@W snaps $s jaws at you!@n"},
    {"@C$n@W is panting heavily from $s struggle with @c$N@W!@n","@C$n@W is panting heavily from $s struggle with you!@n"},
    {"@C$n@W circles around @c$N@W trying to get a better position!@n","@C$n@W circles around you trying to find a weak spot!@n"},
    {"@C$n@W jumps up slightly in an attempt to threaten @c$N@W!@n","@C$n@W jumps up slightly in an attempt to threaten you!@n"},
    {"@C$n@W turns sideways while facing @c$N@W in an attempt to appear larger and more threatening!@n","@C$n@W turns sideways while facing you in an attempt to appear larger and more threatening!@n"},
    {"@C$n@W roars with the full power of its lungs at @c$N@W!@n","@C$n@W roars with the full power of its lungs at you!@n"},
    {"@C$n@W staggers from the strain of fighting.@n","@C$n@W staggers from the strain of fighting.@n"},
    {"@C$n@W slumps down for a moment before regaining $s guard against @c$N@W!@n","@C$n@W slumps down for a moment before regaining $s guard against you!@n"},
    {"@C$n's@W eyes dart around as $e seems to look for safe places to run.@n","@C$n's@W eyes dart around as $e seems to look for safe places to run.@n"},
    {"@C$n@W jumps past @c$N@W before turning and facing $M again!@n","@C$n@W jumps past you before turning and facing you again!@n"},
    {"@C$n@W watches @c$N@W with a threatening gaze while $e looks for a weakness!@n","@C$n@W watches you with a threatening gaze while $e looks for a weakness!@n"}
};

static const std::vector<std::pair<std::string, std::string>> animalWater = {
    {"@C$n@W snaps $s jaws at @c$N@W which causes a torrent of bubbles to float upward!@n","@C$n@W snaps $s jaws at you which causes a torrent of bubbles to float upward!@n"},
    {"@C$n@W thrashes around in the water!@n","@C$n@W thrashes around in the water!@n"},
    {"@C$n@W swims past @c$N@W before turning and facing $M again!@n","@C$n@W swims past you before turning and facing you again!@n"},
    {"@C$n@W begins to slowly circle @c$N@W while looking for an opening!@n","@C$n@W begins to slowly circle you while looking for an opening!@n"},
    {"@C$n@W swims backward in an attempt to gain a safe distance from @C$N's@W aggression.@n","@C$n@W swims backward in an attempt to gain a safe distance from you.@n"},
    {"@C$n@W swims toward the side of @C$N@W in an attempt to flank $M!@n","@C$n@W swims toward the side of you in an attempt to flank you!@n"},
    {"@C$n@W swims upward before darting down past @c$N@W!@n","@C$n@W swims upward before darting down past you!@n"}
};

static const std::vector<std::pair<std::string, std::string>> intelligentFlying = {
    {"@C$n@W flies around @c$N@W slowly while looking for an opening!@n","@C$n@W flies around you slowly while looking for an opening!@n"},
    {"@C$n@W floats slowly while scowling at @c$N@W!@n","@C$n@W floats slowly while scowling at you!@n"},
    {"@C$n@W spits at @c$N@W!@n","@C$n@W spits at you!@n"},
    {"@C$n@W looks at @c$N@W as if $e is weighing $s options.@n","@C$n@W looks at you as if $e is weighing $s options.@n"},
    {"@C$n@W scowls at @c$N@W while changing $s position carefully!@n","@C$n@W scowls at you while changing $s position carefully!@n"},
    {"@C$n@W flips backward a short way away from @c$N@W!@n","@C$n@W flips backward a short way away from you!@n"},
    {"@C$n@W moves slowly to the side of @c$N@W while watching $M carefully.@n","@C$n@W moves slowly to the side of you while watching you carefully.@n"},
    {"@C$n@W flexes $s arms in an attempt to threaten @C$N@W.@n","@C$n@W flexes $s arms threaten in an attempt to threaten you@W.@n"},
    {"@C$n@W raises an arm in front of $s body as a defense.@n","@C$n@W raises an arm in front of $s body as a defense.@n"},
    {"@C$n@W feints a punch toward @c$N@W that misses by a mile.@n","@C$n@W feints a punch toward you that misses by a mile.@n"}
};

static const std::vector<std::pair<std::string, std::string>> intelligentLand = {
    {"@C$n@W shuffles around @c$N@W slowly while looking for an opening!@n","@C$n@W shuffles around you slowly while looking for an opening!@n"},
    {"@C$n@W scowls @c$N@W!@n","@C$n@W scowls at you!@n"},
    {"@C$n@W has sparks come off them that land on @c$N@W!@n@n","@C$n@W has sparks come off them that land on you!@n"},
    {"@C$n@W looks at @c$N@W as if $e is weighing $s options.@n","@C$n@W looks at you as if $e is weighing $s options.@n"},
    {"@C$n@W scowls at @c$N@W while changing $s position carefully!@n","@C$n@W scowls at you while changing $s position carefully!@n"},
    {"@C$n@W flips backward a short way away from @c$N@W!@n","@C$n@W flips backward a short way away from you!@n"},
    {"@C$n@W moves slowly to the side of @c$N@W while watching $M carefully.@n","@C$n@W moves slowly to the side of you while watching you carefully.@n"},
    {"@C$n@W crouches down cautiously.@n","@C$n@W crouches down cautiously.@n"},
    {"@C$n@W moves $s feet slowly to achieve a better balance.@n","@C$n@W moves $s feet slowly to achieve a better balance.@n"},
    {"@C$n@W leaps to a more defensible spot.@n","@C$n@W leaps to a more defensible spot.@n"},
    {"@C$n@W runs a short distance away before skidding to a halt and resuming $s fighting stance.@n","@C$n@W runs a short distance away before skidding to a halt and resuming $s fighting stance.@n"},
    {"@C$n@W stands up to $s full height and glares at @C$N@W with burning eyes.@n","@C$n@W stands up to $s full height and glares at you with intense burning eyes.@n"}
};


/* This handles NPCs taunting opponents or reacting to combat. */
void mob_taunt(Character *ch) {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE)) { /* In space.... nobody cares. */
        return;
    }

    if (!FIGHTING(ch)) { /* The NPC is not fighting. Error. ABORT! */
        return;
    }

    auto vict = FIGHTING(ch);

    if (!vict) { /* OH NO */
        return;
    }

    if(!IS_HUMANOID(ch)) {
        auto messages = Random::get(SUNKEN(IN_ROOM(ch)) ? animalWater : animalLand);
        act(messages->first.c_str(), true, ch, nullptr, vict, TO_NOTVICT);
        act(messages->second.c_str(), true, ch, nullptr, vict, TO_VICT);
    } else if(!MOB_FLAGGED(ch, MOB_DUMMY)) {
        auto messages = Random::get(AFF_FLAGGED(ch, AFF_FLYING) ? intelligentFlying : intelligentLand);
        act(messages->first.c_str(), true, ch, nullptr, vict, TO_NOTVICT);
        act(messages->second.c_str(), true, ch, nullptr, vict, TO_VICT);
    }
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(Character *ch, Character *victim) {
    memory_rec *tmp;
    bool present = false;

    if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
        return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
        if (tmp->id == GET_IDNUM(victim))
            present = true;

    if (!present && !MOB_FLAGGED(ch, MOB_SPAR) && !PLR_FLAGGED(victim, PLR_SPAR)) {
        CREATE(tmp, memory_rec, 1);
        tmp->next = MEMORY(ch);
        tmp->id = GET_IDNUM(victim);
        MEMORY(ch) = tmp;
    }
}


/* make ch forget victim */
void forget(Character *ch, Character *victim) {
    memory_rec *curr, *prev = nullptr;

    if (!(curr = MEMORY(ch)))
        return;

    while (curr && curr->id != GET_IDNUM(victim)) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return;            /* person wasn't there at all. */

    if (curr == MEMORY(ch))
        MEMORY(ch) = curr->next;
    else
        prev->next = curr->next;

    free(curr);
}


/* erase ch's memory */
void clearMemory(Character *ch) {
    memory_rec *curr, *next;

    curr = MEMORY(ch);

    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    MEMORY(ch) = nullptr;
}
