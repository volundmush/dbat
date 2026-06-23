/* ************************************************************************
 *   File: mobact.c                                      Part of CircleMUD *
 *  Usage: Functions for generating intelligent (?) behavior in mobiles    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "mobact.h"

#include "act.item.h"
#include "act.movement.h"
#include "act.other.h"
#include "act.social.h"
#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "class.h"
#include "combat.h"
#include "comm.h"
#include "consts/affflags.h"
#include "consts/applies.h"
#include "consts/itemdata.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/races.h"
#include "consts/sex.h"
#include "db.h"
#include "extract.h"
#include "flags.h"
#include "handler.h"
#include "interpreter.h"
#include "log.h"
#include "object_impl.h"
#include "object_macros.h"
#include "random.h"
#include "room_api.h"
#include "room_utils.h"
#include "shop.h"
#include "shop_impl.h"
#include "spec_procs.h"
#include "spells.h"

#include "object_db.h"
#include "zone_api.h"

#include <cstring>
#include <vector>
#include "iterate.hpp"

/* local functions */
int player_present(struct char_data *ch);

bool aggressive_mob_on_a_leash(struct char_data *slave,
                               struct char_data *master,
                               struct char_data *attack);
void mob_absorb(struct char_data *ch, struct char_data *vict);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

void mob_absorb(struct char_data *ch, struct char_data *vict) {

  if (ABSORBING(ch)) {
    act("@R$n@w releases YOU from $s grip!@n", TRUE, ch, 0, ABSORBING(ch),
        TO_VICT);
    act("@R$n@w releases @R$N@w from $s grip!@n", TRUE, ch, 0, ABSORBING(ch),
        TO_NOTVICT);
    struct char_data *absorbed = ABSORBING(ch);
    char_absorbing_set(ch, NULL);
    char_absorbed_by_set(absorbed, NULL);
    return;
  }

  int zanzo = FALSE, roll = 0, chance = GET_LEVEL(ch) * 0.5,
      chance2 = GET_LEVEL(ch) + 10;

  if (chance2 > 118)
    chance2 = 118;

  if (GET_LEVEL(ch) < 2)
    return;
  else
    roll = rand_number(chance, chance2);

  if (!vict)
    return;

  if (IS_ANDROID(vict))
    return;

  if (char_condition_has(vict, "zanzoken")) {
    if (char_condition_has(ch, "zanzoken")) {
      if (GET_SPEEDI(ch) < GET_SPEEDI(vict)) {
        zanzo = TRUE;
      } else {
        char_condition_remove(ch, "zanzoken", "zanzoken_over");
      }
    } else {
      zanzo = TRUE;
    }
    if (zanzo == TRUE) {
      act("@R$n@c tries to grab @RYOU@c but you @Czanzoken@c out of the way!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@R$n@ctries to grab @R$N@c but $E @Czanzokens@c out of the way!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      char_condition_remove(ch, "zanzoken", "zanzoken_over");
      char_condition_remove(vict, "zanzoken", "zanzoken_over");
      return;
    } else {
      act("@cYou try to @Czanzoken@c out of @R$n's@c reach, but $e is too "
          "fast!@n",
          TRUE, ch, 0, vict, TO_VICT);
      act("@c$N tries to @Czanzoken@c out of @R$n's@c reach, but $e is too "
          "fast!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      char_condition_remove(vict, "zanzoken", "zanzoken_over");
    }
  }

  if (roll < check_def(vict)) {
    act("@R$n@r tries to grab YOU, but you manage to evade $s grasp!@n", TRUE,
        ch, 0, vict, TO_VICT);
    act("@R$n@r tries to grab @R$N@r, but @R$N@r manages to evade!@n", TRUE, ch,
        0, vict, TO_NOTVICT);
    return;
  } else {
    act("@R$n@r grabs onto YOU and starts to absorb your energy!@n", TRUE, ch,
        0, vict, TO_VICT);
    act("@R$n@r grabs onto @R$N@r and starts to absorb your energy!@n", TRUE,
        ch, 0, vict, TO_NOTVICT);
    char_absorbing_set(ch, vict);
    char_absorbed_by_set(vict, ch);
    return;
  }
}

int player_present(struct char_data *ch) {

  auto room = char_room_get(ch);
  if(!room) return 0;

  bool found = FALSE;
  room_people_iterate(room, [&](auto t) {
    if (!IS_NPC(t)) {
      found = TRUE;
      return false;
    }
    return true;
  });

  return (found);
}

void char_game_activate(struct char_data *ch) {
  char_condition_game_activate(ch);
  char_meter_conditions_sync(ch);
  char_limb_healing_sync(ch);
  if (!IS_NPC(ch))
    return;
  char_subscribe_add(ch, "mob_active");
  if (MOB_FLAGGED(ch, MOB_SPEC) && mob_proto_special_get(GET_MOB_VNUM(ch)))
    char_subscribe_add(ch, "mob_spec");
  if (IS_HUMANOID(ch) && !MOB_FLAGGED(ch, MOB_NOSCAVENGER) && !MOB_FLAGGED(ch, MOB_NOKILL))
    char_subscribe_add(ch, "mob_scavenger");
  if (!MOB_FLAGGED(ch, MOB_SENTINEL))
    char_subscribe_add(ch, "mob_wander");
  if (MOB_FLAGGED(ch, MOB_AGGRESSIVE))
    char_subscribe_add(ch, "mob_aggressive");
  if (MOB_FLAGGED(ch, MOB_HELPER))
    char_subscribe_add(ch, "mob_helper");
  if (IS_HUMANOID(ch) && !MOB_FLAGGED(ch, MOB_DUMMY))
    char_subscribe_add(ch, "mob_memory");
  if (GET_MOB_SPEC(ch) == shop_keeper)
    char_subscribe_add(ch, "mob_shopkeeper");
  if (GET_ORIGINAL(ch))
    char_subscribe_add(ch, "mob_multiform");
}

void char_game_deactivate(struct char_data *ch) {
  char_condition_game_deactivate(ch);
  char_unsubscribe_all(ch);
}

void obj_game_activate(struct obj_data *obj) {
  if (GET_OBJ_VNUM(obj) == 82 || GET_OBJ_VNUM(obj) == 83) {
    obj_subscribe_add(obj, "obj_huge_attack");
  }
  if (GET_OBJ_VNUM(obj) == 11 || GET_OBJ_VNUM(obj) == 3034)
    obj_subscribe_add(obj, "obj_broken");
  if (GET_OBJ_TYPE(obj) == ITEM_PLANT)
    obj_subscribe_add(obj, "obj_plant");
  if (IS_CORPSE(obj))
    obj_subscribe_add(obj, "obj_corpse");
  if (OBJ_FLAGGED(obj, ITEM_ICE))
    obj_subscribe_add(obj, "obj_ice");
  if (GET_OBJ_VNUM(obj) == 65)
    obj_subscribe_add(obj, "obj_healing_tank");
  if (OBJ_FLAGGED(obj, ITEM_NORENT))
    obj_subscribe_add(obj, "obj_norent");
  if (GET_OBJ_TYPE(obj) == ITEM_PORTAL || GET_OBJ_VNUM(obj) == 1306)
    obj_subscribe_add(obj, "obj_timed");
}

void obj_game_deactivate(struct obj_data *obj) {
  obj_unsubscribe_all(obj);
}

static void mob_spec_update() {
  char_for_each("mob_spec", [](struct char_data *ch) {
    if (!MOB_FLAGGED(ch, MOB_SPEC) || no_specials)
      return;
    if (auto spec = mob_proto_special_get(GET_MOB_VNUM(ch)); !spec) {
      mud_log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
          GET_NAME(ch), GET_MOB_VNUM(ch));
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
    } else {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (spec)(ch, ch, 0, actbuf);
    }
  });
}

static void mob_scavenger_update() {
  char_for_each("mob_scavenger", [](struct char_data *ch) {
    if (!zone_player_count_get(char_zone_vnum_get(ch))) return;
    if (!AWAKE(ch) || FIGHTING(ch))
      return;
    if (!IS_HUMANOID(ch) || MOB_FLAGGED(ch, MOB_NOSCAVENGER) || MOB_FLAGGED(ch, MOB_NOKILL))
      return;
    if (player_present(ch) && axion_dice(0) <= 118)
      return;
    auto room = char_room_get(ch);
    if (!room_contents_get(room) || rand_number(1, 100) < 95)
      return;

    int max = 1;
    struct obj_data *best_obj = NULL;
    room_contents_iterate(room, [&](auto obj) {
      if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
        best_obj = obj;
        max = GET_OBJ_COST(obj);
      }
      return true;
    });
    if (best_obj && CAN_GET_OBJ(ch, best_obj) &&
        GET_OBJ_TYPE(best_obj) != ITEM_BED && !GET_OBJ_POSTED(best_obj) &&
        !OBJ_FLAGGED(best_obj, ITEM_NOPICKUP)) {
      switch (rand_number(1, 5)) {
      case 1:
        act("$n@W says, '@CFinders keepers, losers weepers.@W'@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n@W says, '@CPeople always leaving their garbage JUST LYING AROUND. The nerve....@W'@n",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n@W says, '@CWho would leave this here? Oh well..@W'@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n@W says, '@CI always wanted one of these.@W'@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 5:
        act("$n@W looks around quickly to see if anyone is paying attention.@n",
            TRUE, ch, 0, 0, TO_ROOM);
        break;
      }
      perform_get_from_room(ch, best_obj);
    }
  });
}

static void mob_wander_update() {
  std::vector<int> available_dirs(12, 0);
  size_t available = 0;
  zone_iterate_active([&](auto zone) {
    zone_mobs_iterate(zone_id_get(zone), [&](struct char_data *ch) {
      if (MOB_FLAGGED(ch, MOB_SENTINEL) || GET_POS(ch) != POS_STANDING)
        return true;
      if (!AWAKE(ch) || FIGHTING(ch))
        return true;
      if (AFF_FLAGGED(ch, AFF_TAMED) || ABSORBBY(ch) || IS_AFFECTED(ch, AFF_PARALYZE))
        return true;
      if (rand_number(1, 3) != 3)
        return true;

      available = 0;

      room_exits_iterate(char_room_get(ch), [&](auto dir, auto exit) {
        if (auto dest = char_can_go_exit(ch, exit); dest &&
            !room_flagged(dest, ROOM_NOMOB) && !room_flagged(dest, ROOM_DEATH) &&
            (!MOB_FLAGGED(ch, MOB_STAY_ZONE) || (room_zone_get(dest) == zone))) {
          available_dirs[available++] = dir;
        }
        return true;
      });
      if (available > 0 && block_calc(ch))
        perform_move(ch, available_dirs[rand_number(0, available - 1)], 1);
      return true;
    });
    return true;
  });
}

static void mob_aggressive_update() {
  char_for_each("mob_aggressive", [](struct char_data *ch) {
    if (!zone_player_count_get(char_zone_vnum_get(ch))) return;
    if (!AWAKE(ch) || !MOB_FLAGGED(ch, MOB_AGGRESSIVE) || IS_AFFECTED(ch, AFF_PARALYZE))
      return;
    int spot_roll = rand_number(1, GET_LEVEL(ch) + 10);
    int found = FALSE;
    room_people_iterate(char_room_get(ch), [&](auto vict) {
      if (vict == ch || FIGHTING(ch))
        return true;
      if (!CAN_SEE(ch, vict) || IS_NPC(vict))
        return true;
      if (PRF_FLAGGED(vict, PRF_NOHASSLE))
        return true;
      if (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && GET_ALIGNMENT(vict) < 50)
        return true;
      if (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && GET_ALIGNMENT(vict) > -50)
        return true;
      if (GET_LEVEL(vict) < 5)
        return true;
      if (AFF_FLAGGED(vict, AFF_HIDE) && GET_SKILL(vict, SKILL_HIDE) > spot_roll)
        return true;
      if (AFF_FLAGGED(vict, AFF_SNEAK) && GET_SKILL(vict, SKILL_MOVE_SILENTLY) > spot_roll)
        return true;
      if (ch->aggtimer < 8) {
        ch->aggtimer += 1;
        return true;
      }
      if (found)
        return true;
      ch->aggtimer = 0;
      char tar[MAX_INPUT_LENGTH];
      sprintf(tar, "%s", GET_NAME(vict));
      if (IS_HUMANOID(ch)) {
        if (!AFF_FLAGGED(vict, AFF_HIDE) && !AFF_FLAGGED(vict, AFF_SNEAK)) {
          act("@w'I am going to get you!' @C$n@w shouts at you!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        } else {
          act("@C$n@w notices YOU.\n@w'I am going to get you!' @C$n@w shouts at you!@n",
              TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@w notices @c$N@w.\n@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
        }
        if (char_condition_has(vict, "flying") && !char_condition_has(ch, "flying") &&
            IS_HUMANOID(ch) && GET_LEVEL(ch) > 10) {
          char_cmd_execute(ch, "fly", "");
          return true;
        }
        if (!char_condition_has(vict, "flying") && char_condition_has(ch, "flying")) {
          char_cmd_execute(ch, "fly", "");
          return true;
        }
        char_cmd_execute(ch, "punch", tar);
      } else {
        if (char_condition_has(vict, "flying") && !char_condition_has(ch, "flying") &&
            IS_HUMANOID(ch) && GET_LEVEL(ch) > 10) {
          char_cmd_execute(ch, "fly", "");
          return true;
        }
        if (!char_condition_has(vict, "flying") && char_condition_has(ch, "flying")) {
          char_cmd_execute(ch, "fly", "");
          return true;
        }
        if (!AFF_FLAGGED(vict, AFF_HIDE) && !AFF_FLAGGED(vict, AFF_SNEAK)) {
          act("@C$n @wgrowls viciously at you!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n @wgrowls viciously at @c$N@w!@n", TRUE, ch, 0, vict, TO_NOTVICT);
        } else {
          act("@C$n@w notices YOU.\n@C$n @wgrowls viciously at you!@n", TRUE, ch, 0, vict, TO_VICT);
          act("@C$n@w notices @c$N@w.\n@C$n @wgrowls viciously at @c$N@w!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
        }
        char_cmd_execute(ch, "bite", tar);
      }
      found = TRUE;
      return true;
    });
  });
}

static void mob_multiform_update() {
  char_for_each("mob_multiform", [](struct char_data *ch) {
    if (!AWAKE(ch))
      return;
    struct char_data *original = GET_ORIGINAL(ch);
    if (!original || rand_number(1, 5) < 4)
      return;
    if (!FIGHTING(original) || FIGHTING(ch))
      return;
    char target[MAX_INPUT_LENGTH];
    sprintf(target, "%s", FIGHTING(original)->name);
    if (rand_number(1, 5) >= 4)
      char_cmd_execute(ch, "kick", target);
    else if (rand_number(1, 5) >= 4)
      char_cmd_execute(ch, "elbow", target);
    else
      char_cmd_execute(ch, "punch", target);
  });
}

static void mob_runtime_update() {
  char_for_each("mob_active", [](struct char_data *ch) {
    if (!zone_player_count_get(char_zone_vnum_get(ch))) return;
    if (ABSORBBY(ch) && rand_number(1, 3) == 3)
      char_cmd_execute(ch, "escape", NULL);
    if (GET_POS(ch) == POS_SLEEPING && rand_number(1, 3) == 3)
      char_cmd_execute(ch, "wake", NULL);
  });
}

static void mob_shopkeeper_update() {
  char_for_each("mob_shopkeeper", [](struct char_data *ch) {
    if (GET_MOB_SPEC(ch) != shop_keeper)
      return;
    time_t diff = time(0) - GET_LPLAY(ch);
    if (diff <= 86400)
      return;
    struct shop_data *shop = NULL;
    GET_LPLAY(ch) = time(0);
    shop_iterate([&](auto s) {
      if (SHOP_KEEPER(s) == GET_MOB_VNUM(ch)) {
        shop = s;
        return false;
      }
      return true;
    });
    char_inventory_iterate(ch, [&](auto sobj) {
      if (sobj && (!shop || !shop_producing(sobj, shop))) {
        char_stat_mod(ch, "money", GET_OBJ_COST(sobj));
        extract_obj(sobj);
      }
      return true;
    });
  });
}

static void mob_memory_update() {
  char_for_each("mob_memory", [](struct char_data *ch) {
    if (!zone_player_count_get(char_zone_vnum_get(ch))) return;
    if (!AWAKE(ch) || !IS_HUMANOID(ch) || !MEMORY(ch))
      return;
    if (MOB_FLAGGED(ch, MOB_DUMMY) || IS_AFFECTED(ch, AFF_PARALYZE) || FIGHTING(ch))
      return;
    int found = FALSE;
    memory_rec *names;
    room_people_iterate(char_room_get(ch), [&](auto vict) {
      if (found)
        return false;
      if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
        return true;
      if (GET_HIT(ch) <= GET_MAX_HIT(ch) / 100)
        return true;
      for (names = MEMORY(ch); names && !found; names = names->next) {
        if (names->id != GET_IDNUM(vict))
          continue;
        found = TRUE;
        act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.",
            FALSE, ch, 0, 0, TO_ROOM);
        char tar[MAX_INPUT_LENGTH];
        sprintf(tar, "%s", GET_NAME(vict));
        char_cmd_execute(ch, "punch", tar);
      }
      return true;
    });
  });
}

static void mob_combat_taunt() {
  char_for_each("combat", [](struct char_data *ch) {
    if (!IS_NPC(ch) || !FIGHTING(ch))
      return;
    if (rand_number(1, 30) >= 25)
      mob_taunt(ch);
  });
}

static void mob_helper_update() {
  char_for_each("mob_helper", [](struct char_data *ch) {
    if (!zone_player_count_get(char_zone_vnum_get(ch))) return;
    if (!AWAKE(ch) || !MOB_FLAGGED(ch, MOB_HELPER))
      return;
    if (AFF_FLAGGED(ch, AFF_BLIND) || AFF_FLAGGED(ch, AFF_CHARM))
      return;
    int found = FALSE;
    room_people_iterate(char_room_get(ch), [&](auto vict) {
      if (found)
        return false;
      if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
        return true;
      if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
        return true;
      if (!IS_HUMANOID(vict))
        return true;
      act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
      char tar[MAX_INPUT_LENGTH];
      sprintf(tar, "%s", GET_NAME(FIGHTING(vict)));
      char_cmd_execute(ch, "punch", tar);
      found = TRUE;
      return true;
    });
  });
}

static void huge_attack_update() {
  obj_for_each("obj_huge_attack", [](struct obj_data *hugeatk) {
    auto user = USER(hugeatk);
    if (!user)
      return;
    auto room = obj_room_get(hugeatk);
    if (!room)
      return;
    char tar[MAX_INPUT_LENGTH];
    sprintf(tar, "%s", GET_NAME(user));

    room_people_iterate(room, [&](auto ch) {
      if (!IS_MOB(ch) || FIGHTING(ch) || MOB_FLAGGED(ch, MOB_NOKILL))
        return true;
      act("@W$n@R leaps at @C$N@R desperately!@n", TRUE, ch, 0, user, TO_ROOM);
      act("@W$n@R leaps at YOU desperately!@n", TRUE, ch, 0, user, TO_VICT);
      if (IS_HUMANOID(ch))
        char_cmd_execute(ch, "punch", tar);
      else
        char_cmd_execute(ch, "bite", tar);
      return true;
    });
  });
}

void mobile_activity(void) {
  struct PhaseTime { const char *name; double ms; };
  constexpr double SLOW_PHASE_MS = 10.0;
  PhaseTime phases[12];
  size_t np = 0;

  auto mono_now = []() -> struct timespec {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
  };
  auto elapsed_ms = [](const struct timespec &t0, const struct timespec &t1) -> double {
    return (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1.0e6;
  };
  const struct timespec t_start = mono_now();

  auto time_phase = [&](const char *label, auto fn) {
    const struct timespec t = mono_now();
    fn();
    phases[np++] = {label, elapsed_ms(t, mono_now())};
  };

  time_phase("mob_spec_update",      [&]{ mob_spec_update(); });
  time_phase("mob_scavenger_update", [&]{ mob_scavenger_update(); });
  time_phase("mob_wander_update",    [&]{ mob_wander_update(); });
  time_phase("mob_aggressive_update",[&]{ mob_aggressive_update(); });
  time_phase("mob_multiform_update", [&]{ mob_multiform_update(); });
  time_phase("mob_runtime_update",   [&]{ mob_runtime_update(); });
  time_phase("mob_shopkeeper_update",[&]{ mob_shopkeeper_update(); });
  time_phase("mob_memory_update",    [&]{ mob_memory_update(); });
  time_phase("mob_combat_taunt",     [&]{ mob_combat_taunt(); });
  time_phase("mob_helper_update",    [&]{ mob_helper_update(); });
  time_phase("huge_attack_update",   [&]{ huge_attack_update(); });

  const double total_ms = elapsed_ms(t_start, mono_now());
  bool any_slow = false;
  for (size_t i = 0; i < np; ++i)
    if (phases[i].ms >= SLOW_PHASE_MS) { any_slow = true; break; }
  if (any_slow) {
    char buf[512];
    int pos = snprintf(buf, sizeof(buf), "SLOW mobile_activity %.0fms:", total_ms);
    for (size_t i = 0; i < np && pos < static_cast<int>(sizeof(buf)) - 32; ++i)
      if (phases[i].ms >= 1.0)
        pos += snprintf(buf + pos, sizeof(buf) - pos, " %s=%.0fms", phases[i].name, phases[i].ms);
    mud_log("%s", buf);
  }
}

/* This handles NPCs taunting opponents or reacting to combat. */
void mob_taunt(struct char_data *ch) {

  int message = 1;

  if (room_flagged(char_room_get(ch),
                   ROOM_SPACE)) { /* In space.... nobody cares. */
    return;
  }

  if (!FIGHTING(ch)) { /* The NPC is not fighting. Error. ABORT! */
    return;
  }

  struct char_data *vict = FIGHTING(ch);

  if (vict == NULL) { /* OH NO */
    return;
  }

  if (!IS_HUMANOID(ch) &&
      !room_is_sunken(char_room_get(
          ch))) { /* They are an animal and they are not in the water. */
    message = rand_number(1, 12);
    switch (message) { /* Display the appropriate message. */
    case 1:
      act("@C$n@W growls viciously at @c$N@W!@n", TRUE, ch, 0, vict,
          TO_NOTVICT);
      act("@C$n@W growls viciously at you!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
    case 2:
      act("@C$n@W snaps $s jaws at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W snaps $s jaws at you!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
    case 3:
      act("@C$n@W is panting heavily from $s struggle with @c$N@W!@n", TRUE, ch,
          0, vict, TO_NOTVICT);
      act("@C$n@W is panting heavily from $s struggle with you!@n", TRUE, ch, 0,
          vict, TO_VICT);
      break;
    case 4:
      act("@C$n@W circles around @c$N@W trying to get a better position!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W circles around you trying to find a weak spot!@n", TRUE, ch,
          0, vict, TO_VICT);
      break;
    case 5:
      act("@C$n@W jumps up slightly in an attempt to threaten @c$N@W!@n", TRUE,
          ch, 0, vict, TO_NOTVICT);
      act("@C$n@W jumps up slightly in an attempt to threaten you!@n", TRUE, ch,
          0, vict, TO_VICT);
      break;
    case 6:
      act("@C$n@W turns sideways while facing @c$N@W in an attempt to appear "
          "larger and more threatening!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W turns sideways while facing you in an attempt to appear "
          "larger and more threatening!@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    case 7:
      act("@C$n@W roars with the full power of its lungs at @c$N@W!@n", TRUE,
          ch, 0, vict, TO_NOTVICT);
      act("@C$n@W roars with the full power of its lungs at you!@n", TRUE, ch,
          0, vict, TO_VICT);
    case 8:
      act("@C$n@W staggers from the strain of fighting.@n", TRUE, ch, 0, vict,
          TO_NOTVICT);
      act("@C$n@W staggers from the strain of fighting.@n", TRUE, ch, 0, vict,
          TO_VICT);
      break;
    case 9:
      act("@C$n@W slumps down for a moment before regaining $s guard against "
          "@c$N@W!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W slumps down for a moment before regaining $s guard against "
          "you!@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    case 10:
      act("@C$n's@W eyes dart around as $e seems to look for safe places to "
          "run.@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n's@W eyes dart around as $e seems to look for safe places to "
          "run.@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    case 11:
      act("@C$n@W jumps past @c$N@W before turning and facing $M again!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W jumps past you before turning and facing you again!@n", TRUE,
          ch, 0, vict, TO_VICT);
      break;
    default:
      act("@C$n@W watches @c$N@W with a threatening gaze while $e looks for a "
          "weakness!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W watches you with a threatening gaze while $e looks for a "
          "weakness!@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    }
  } else if (!IS_HUMANOID(ch)) { /* Animal under water */
    message = rand_number(1, 7);
    switch (message) {
    case 1:
      act("@C$n@W snaps $s jaws at @c$N@W which causes a torrent of bubbles to "
          "float upward!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W snaps $s jaws at you which causes a torrent of bubbles to "
          "float upward!@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    case 2:
      act("@C$n@W thrashes around in the water!@n", TRUE, ch, 0, vict,
          TO_NOTVICT);
      act("@C$n@W thrashes around in the water!@n", TRUE, ch, 0, vict, TO_VICT);
      break;
    case 3:
      act("@C$n@W swims past @c$N@W before turning and facing $M again!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W swims past you before turning and facing you again!@n", TRUE,
          ch, 0, vict, TO_VICT);
      break;
    case 4:
      act("@C$n@W begins to slowly circle @c$N@W while looking for an "
          "opening!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W begins to slowly circle you while looking for an opening!@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    case 5:
      act("@C$n@W swims backward in an attempt to gain a safe distance from "
          "@C$N's@W aggression.@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W swims backward in an attempt to gain a safe distance from "
          "you.@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    case 6:
      act("@C$n@W swims toward the side of @C$N@W in an attempt to flank $M!@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      act("@C$n@W swims toward the side of you in an attempt to flank you!@n",
          TRUE, ch, 0, vict, TO_VICT);
      break;
    default:
      act("@C$n@W swims upward before darting down past @c$N@W!@n", TRUE, ch, 0,
          vict, TO_NOTVICT);
      act("@C$n@W swims upward before darting down past you!@n", TRUE, ch, 0,
          vict, TO_VICT);
      break;
    }
  } else if (!MOB_FLAGGED(ch, MOB_DUMMY)) { /* They are intelligent */
    message = rand_number(1, 10);
    if (!room_is_sunken(char_room_get(ch))) {
      if (char_condition_has(ch, "flying")) { /* They are flying */
        switch (message) {
        case 1:
          act("@C$n@W flies around @c$N@W slowly while looking for an "
              "opening!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W flies around you slowly while looking for an opening!@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 2:
          act("@C$n@W floats slowly while scowling at @c$N@W!@n", TRUE, ch, 0,
              vict, TO_NOTVICT);
          act("@C$n@W floats slowly while scowling at you!@n", TRUE, ch, 0,
              vict, TO_VICT);
          break;
        case 3:
          act("@C$n@W spits at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W spits at you!@n", TRUE, ch, 0, vict, TO_VICT);
          break;
        case 4:
          act("@C$n@W looks at @c$N@W as if $e is weighing $s options.@n", TRUE,
              ch, 0, vict, TO_NOTVICT);
          act("@C$n@W looks at you as if $e is weighing $s options.@n", TRUE,
              ch, 0, vict, TO_VICT);
          break;
        case 5:
          act("@C$n@W scowls at @c$N@W while changing $s position carefully!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W scowls at you while changing $s position carefully!@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 6:
          act("@C$n@W flips backward a short way away from @c$N@W!@n", TRUE, ch,
              0, vict, TO_NOTVICT);
          act("@C$n@W flips backward a short way away from you!@n", TRUE, ch, 0,
              vict, TO_VICT);
          break;
        case 7:
          act("@C$n@W moves slowly to the side of @c$N@W while watching $M "
              "carefully.@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W moves slowly to the side of you while watching you "
              "carefully.@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 8:
          act("@C$n@W flexes $s arms in an attempt to threaten @C$N@W.@n", TRUE,
              ch, 0, vict, TO_NOTVICT);
          act("@C$n@W flexes $s arms threaten in an attempt to threaten "
              "you@W.@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 9:
          act("@C$n@W raises an arm in front of $s body as a defense.@n", TRUE,
              ch, 0, vict, TO_NOTVICT);
          act("@C$n@W raises an arm in front of $s body as a defense.@n", TRUE,
              ch, 0, vict, TO_VICT);
          break;
        default:
          act("@C$n@W feints a punch toward @c$N@W that misses by a mile.@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W feints a punch toward you that misses by a mile.@n", TRUE,
              ch, 0, vict, TO_VICT);
          break;
        }

      } else { /* They are not flying. */
        message = rand_number(1, 13);
        switch (message) {
        case 1:
          act("@C$n@W shuffles around @c$N@W slowly while looking for an "
              "opening!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W shuffles around you slowly while looking for an "
              "opening!@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 2:
          act("@C$n@W scowls @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W scowls at you!@n", TRUE, ch, 0, vict, TO_VICT);
          break;
        case 3:
          if (IS_ANDROID(ch)) {
            act("@C$n@W has sparks come off them that land on @c$N@W!@n@n",
                TRUE, ch, 0, vict, TO_NOTVICT);
            act("@C$n@W has sparks come off them that land on you!@n", TRUE, ch,
                0, vict, TO_VICT);
          } else {
            act("@C$n@W spits at @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
            act("@C$n@W spits at you!@n", TRUE, ch, 0, vict, TO_VICT);
          }
          break;
        case 4:
          act("@C$n@W looks at @c$N@W as if $e is weighing $s options.@n", TRUE,
              ch, 0, vict, TO_NOTVICT);
          act("@C$n@W looks at you as if $e is weighing $s options.@n", TRUE,
              ch, 0, vict, TO_VICT);
          break;
        case 5:
          act("@C$n@W scowls at @c$N@W while changing $s position carefully!@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W scowls at you while changing $s position carefully!@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 6:
          act("@C$n@W flips backward a short way away from @c$N@W!@n", TRUE, ch,
              0, vict, TO_NOTVICT);
          act("@C$n@W flips backward a short way away from you!@n", TRUE, ch, 0,
              vict, TO_VICT);
          break;
        case 7:
          act("@C$n@W moves slowly to the side of @c$N@W while watching $M "
              "carefully.@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W moves slowly to the side of you while watching you "
              "carefully.@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 8:
          act("@C$n@W crouches down cautiously.@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          act("@C$n@W crouches down cautiously.@n", TRUE, ch, 0, vict, TO_VICT);
          break;
        case 9:
          act("@C$n@W moves $s feet slowly to achieve a better balance.@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W moves $s feet slowly to achieve a better balance.@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 10:
          act("@C$n@W leaps to a more defensible spot.@n", TRUE, ch, 0, vict,
              TO_NOTVICT);
          act("@C$n@W leaps to a more defensible spot.@n", TRUE, ch, 0, vict,
              TO_VICT);
          break;
        case 11:
          act("@C$n@W runs a short distance away before skidding to a halt and "
              "resuming $s fighting stance.@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W runs a short distance away before skidding to a halt and "
              "resuming $s fighting stance.@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        case 12:
          act("@C$n@W stands up to $s full height and glares at @C$N@W with "
              "burning eyes.@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W stands up to $s full height and glares at you with "
              "intense burning eyes.@n",
              TRUE, ch, 0, vict, TO_VICT);
          break;
        default:
          act("@C$n@W feints a punch toward @c$N@W that misses by a mile.@n",
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("@C$n@W feints a punch toward you that misses by a mile.@n", TRUE,
              ch, 0, vict, TO_VICT);
          break;
        }
      }
    }
  } /* End humanoid */
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim) {
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present && !MOB_FLAGGED(ch, MOB_SPAR) &&
      !PLR_FLAGGED(victim, PLR_SPAR)) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim) {
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return; /* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}

/* erase ch's memory */
void clearMemory(struct char_data *ch) {
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}

/*
 * An aggressive mobile wants to attack something.  If
 * they're under the influence of mind altering PC, then
 * see if their master can talk them out of it, eye them
 * down, or otherwise intimidate the slave.
 */
bool aggressive_mob_on_a_leash(struct char_data *slave,
                               struct char_data *master,
                               struct char_data *attack) {
  static int snarl_cmd;
  int dieroll;

  if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
    return (FALSE);

  if (!snarl_cmd)
    snarl_cmd = find_command("snarl");

  /* Sit. Down boy! HEEEEeeeel! */
  dieroll = rand_number(1, 20);
  if (dieroll != 1 &&
      (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave))) {
    if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
      char victbuf[MAX_NAME_LENGTH + 1];

      strncpy(victbuf, GET_NAME(attack), sizeof(victbuf)); /* strncpy: OK */
      victbuf[sizeof(victbuf) - 1] = '\0';

      do_action(slave, victbuf, snarl_cmd, 0);
    }

    /* Success! But for how long? Hehe. */
    return (TRUE);
  }

  /* So sorry, now you're a player killer... Tsk tsk. */
  return (FALSE);
}
