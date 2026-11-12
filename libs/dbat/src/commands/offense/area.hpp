#pragma once
/*
 * Attacks on everyone in the room (bakuhatsuha, hell spear, star nova, kakusanha) as data, as ki attacks are in
 * ki_attack.hpp: what every one of them does is perform_area_attack(), and what each does differently is its
 * area_attack. Each of those caught can vanish from it (a zanzoken, or an icer's speed), escape it, or be hit.
 */
#include <cstdint>
#include <string_view>

#include "dbat/comm.hpp"
#include "dbat/consts/types.hpp"

struct area_attack;

/* An area attack being made, as the hooks see it */
struct area_shot {
  GameWorld *g{};
  struct char_data *ch{};
  const area_attack *attack{};
  double attperc{}; /* the share of the user's max ki it uses */
  double minimum{}; /* the least it can */
  int skill{};      /* init_skill()'s roll */
  int count{};      /* how many were worth aiming at */
  int64_t dmg{};    /* each hit's */
};

enum class area_knockdown { never,
                            always,      /* a standing victim who isn't flying is knocked down */
                            one_in_four };

/* Who an area attack is fair game for: anyone else but a player's spirit, a player under the newbie shield or a mob
   no one may kill */
bool area_fair_game(GameWorld *g, struct char_data *ch, struct char_data *vict);
/* ... nor one grouped with the user who leads them or whom they lead */
bool area_fair_game_ungrouped(GameWorld *g, struct char_data *ch, struct char_data *vict);
/* ... nor one grouped with the user under the same leader either */
bool area_fair_game_no_group(GameWorld *g, struct char_data *ch, struct char_data *vict);

/* vict vanishes from it with a zanzoken, or as a fast icer might, saying so by msgs and paying for it; or doesn't */
bool area_vanishes(GameWorld *g, struct char_data *ch, struct char_data *vict, const act_msgs &msgs);

/* What most say of an explosion that catches everyone */
inline constexpr act_msgs EXPLOSION_VANISH = {"@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n",
                                              "@cYou disappear, avoiding the explosion before reappearing elsewhere!@n",
                                              "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n"};
inline constexpr act_msgs EXPLOSION_ESCAPE = {"@c$N@W manages to escape the explosion!@n", "@WYou manage to escape the explosion!@n",
                                              "@c$N@W manages to escape the explosion!@n", true};
inline constexpr act_msgs EXPLOSION_HIT    = {"@R$N@r is caught by the explosion!@n", "@RYou are caught by the explosion!@n",
                                              "@R$N@r is caught by the explosion!@n", true};

struct area_attack {
  int skill{};                /* SKILL_ */
  int damtype{};              /* damtype()'s number for it */
  double attperc{};           /* the share of the user's max ki it uses, unless they say */
  double minimum{};           /* the least it can */
  bool perfections{};         /* perfection 1 uses 5% more, 2 is 5 more skill, 3 is a 5% smaller minimum and cost */
  bool in_peace{};            /* quirk: it can be made in a peaceful room */
  bool (*worth)(GameWorld *g, struct char_data *ch, struct char_data *vict){};  /* who is worth making it for */
  bool (*caught)(GameWorld *g, struct char_data *ch, struct char_data *vict){}; /* who it then catches */
  int cooldown_first{};       /* handle_cooldown() once there is someone to catch, if any */
  int cooldown_last{};        /* and once it has been made, if any */
  bool cooldown_first_early{}; /* quirk: the first comes before the roll to hit, not after (a reveal from hiding rolls too) */
  int dodge_low{};            /* escaping it is handle_dodge() + rand_number(this, 5) beating the skill */
  bool dodge_improves_first{}; /* quirk: an escape improves dodge before it hurts, rather than after */
  bool vanish_hurts{};        /* a vanish hurts (for nothing), as starting a fight */
  area_knockdown knockdown{};
  int max_hits{};             /* it catches at most this many, if any */

  act_msgs fail;   /* to_char and to_room: the user's skill fails them */
  act_msgs launch; /* to_char and to_room */
  act_msgs vanish; /* with the victim */
  act_msgs escape;
  act_msgs hit;

  /* each may be null */
  void (*before_roll)(area_shot &s){}; /* once there is someone to catch, before the skill is tested */
  void (*damage)(area_shot &s){};      /* after its damage is rolled */
  void (*after)(area_shot &s){};       /* after everyone is caught, before the user pays */
};

void perform_area_attack(GameWorld *g, struct char_data *ch, std::string_view argument, const area_attack &attack);
