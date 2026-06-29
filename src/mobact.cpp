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
#include "act.other.h"
#include "act.social.h"
#include "character_api.h"
#include "character_scripts.h"
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
    char_script_add(ch, "mob_scavenger");
  if (!MOB_FLAGGED(ch, MOB_SENTINEL))
    char_script_add(ch, "mob_wander");
  if (MOB_FLAGGED(ch, MOB_AGGRESSIVE))
    char_script_add(ch, "mob_aggressive");
  if (MOB_FLAGGED(ch, MOB_HELPER))
    char_script_add(ch, "mob_helper");
  if (IS_HUMANOID(ch) && !MOB_FLAGGED(ch, MOB_DUMMY))
    char_subscribe_add(ch, "mob_memory");
  if (GET_MOB_SPEC(ch) == shop_keeper)
    char_subscribe_add(ch, "mob_shopkeeper");
}

void char_game_deactivate(struct char_data *ch) {
  char_condition_game_deactivate(ch);
  char_unsubscribe_all(ch);
}

void obj_game_activate(struct obj_data *obj) {
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
  time_phase("mob_runtime_update",   [&]{ mob_runtime_update(); });
  time_phase("mob_shopkeeper_update",[&]{ mob_shopkeeper_update(); });
  time_phase("mob_memory_update",    [&]{ mob_memory_update(); });

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
