#pragma once
/*
 * Strikes (punch, kick, uppercut and the rest) as data, as ki attacks are in ki_attack.hpp: what every one of them
 * does is perform_strike(), and what each does differently is its strike_attack. They cost stamina rather than
 * charged ki, a parry of one is a counterblow, and a hit can be burned by a fire shield and lead to a multihit.
 */
#include <cstdint>
#include <string_view>

#include "dbat/comm.hpp"
#include "dbat/consts/types.hpp"

/* Where a hit landed, which decides how hard it hurts and what equipment it damages */
enum class strike_spot { body,
                         head,
                         arm,
                         leg };

struct strike_attack;

/* A strike being made, as the hooks see it. */
struct strike_shot {
  GameWorld *g{};
  struct char_data *ch{};
  struct char_data *vict{};
  struct obj_data *obj{};
  const strike_attack *attack{};
  int skill{}; /* init_skill()'s roll */
  int64_t dmg{};
  int64_t stcost{}; /* physical_cost()'s, which a hit or a strike at an object costs; anything else, half */
};

struct strike_attack {
  int skill{};              /* SKILL_ */
  const char *name{};       /* as the zanzoken messages name it */
  int damtype{};            /* damtype()'s number for it */
  int cooldown{};           /* handle_cooldown() */
  int stamina_share{};      /* the user needs a 1/this share of their max power level in stamina */
  int improve_mode{};       /* improve_skill()'s last argument for the user */
  bool legs{};              /* the user's legs are checked (a broken one hurt further), rather than arms and hands */
  int limb_hurt{};          /* hurt()'s limb argument for a hit on an arm or a leg */
  int leg_flag{};           /* hurt()'s first argument for a hit on a leg */
  const char *fireshield{}; /* what of the user a fire shield burns when it hits */
  bool spent_hurts_first{}; /* quirk: a miss of someone with no stamina left hurts them before the user pays */
  bool spent_free{};        /* quirk: and the user then pays nothing */

  act_msgs parry; /* a counterblow */
  act_msgs block;
  act_msgs dodge;
  act_msgs miss;              /* when the victim could have avoided it */
  act_msgs miss_spent;        /* when the victim had no stamina left to avoid it */
  act_msgs hits[5];           /* by where roll_hitloc() says it landed */
  const strike_spot *spots{}; /* what each of those is; null for body, head, body, arm, leg */
  const char *obj_to_char{};
  const char *obj_to_room{};
  bool obj_hide{}; /* act()'s hide_invisible for those two */

  /* each may be null */
  int (*cooldown_for)(strike_shot &shot){};        /* handle_cooldown()'s argument, if not cooldown */
  void (*on_spot)(strike_shot &shot, int where){}; /* after a hit's messages, before its damage */
};

void perform_strike(GameWorld *g, struct char_data *ch, std::string_view argument, const strike_attack &attack);
