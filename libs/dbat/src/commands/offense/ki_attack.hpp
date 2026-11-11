#pragma once
/*
 * Ki attacks (galik gun, kamehameha and the rest) as data: what every one of them does is perform_ki_attack(), and
 * what each does differently is its ki_attack. A hook covers what only a few do.
 */
#include <cstdint>
#include <string_view>

#include "dbat/comm.hpp"
#include "dbat/consts/types.hpp"

/* How an attack ended, for the hooks that do something after one ends a certain way. */
enum class ki_outcome { zanzoken,
                        absorbed,
                        blocked,
                        dodged,
                        missed,
                        spent, /* missed someone with no stamina left */
                        hit,
                        object };

/* Where a hit landed, which decides how hard it hurts and what equipment it damages */
enum class ki_spot { body,
                     head,
                     arm,
                     leg };

/* What an attack does if an android absorbs it. */
enum class ki_absorb {
  none,      /* androids can't absorb it */
  costs_one, /* the attacker pays pcost(1) */
  costs_all, /* the attacker pays as for any other ending */
};

struct ki_attack;

/* An attack being made, as the hooks see it. */
struct ki_shot {
  GameWorld *g{};
  struct char_data *ch{};
  struct char_data *vict{};
  struct obj_data *obj{};
  const ki_attack *attack{};
  double attperc{};
  double minimum{};
  int skill{}; /* init_skill()'s roll */
  int64_t dmg{};
  int64_t pl_before{}; /* the victim's power level before a hit landed */
  int mastery{};       /* for a hook that rolls for a master's bonus before zanzoken: 0 for none */
  int landings{1};     /* how many times a hit lands, for a hook that grants more */
  int tally{};         /* for a through hook: how much of it got through */
};

struct ki_attack {
  int skill{};                /* SKILL_ */
  const char *name{};         /* as the zanzoken messages name it */
  const char *parry_name{};   /* as parry_ki() names it, if not name */
  int damtype{};              /* damtype()'s number for it */
  double attperc{};           /* the share of the charge it uses */
  double minimum{};           /* the least share it can be made with */
  int cooldown{};             /* handle_cooldown() */
  int kill_mode{1};           /* can_kill()'s mode for a victim */
  bool not_while_singing{};   /* refused while playing a song */
  bool needs_arms{true};      /* limb_ok() first */
  bool sword{};               /* the user's arms as tech_handle_arms() checks them, rather than limb_ok(), and a sword in hand */
  bool sword_checked_early{}; /* the sword is checked before the charge is, rather than after */
  bool perfection{};          /* perfection 1 adds 5% to attperc, 2 adds 5 to accuracy, 3 takes 5% off the minimum and the cost */
  ki_absorb absorb{};         /* what an android absorbing it does */
  bool block_improves{};      /* blocking it improves the victim's block skill */
  int dodge_dmg_cap{95};      /* dodging it damages the room by dodge_dmg while the room's damage is at most this */
  int dodge_dmg{5};
  int limb_hurt{190};        /* hurt()'s limb argument for a hit on an arm or a leg */
  bool powerhit_crit{};      /* a critical hit does 3x damage, 5x with BONUS_POWERHIT, rather than calc_critical() */
  int head_mult{};           /* a critical hit does this many times the damage, if not as powerhit_crit says */
  int arm_crit{1};           /* calc_critical()'s argument for a hit on an arm */
  bool blockable{true};      /* the victim can block it */
  int dodge_damtype{};       /* dodge_ki()'s number for it, if not damtype */
  int dodge_kind{};          /* dodge_ki()'s first argument when it is dodged: 0 straight, 1 homing, 2 guided */
  int steers{};              /* a miss is followed on into the room too: dodge_ki() with this, 1 homing or 2 guided */
  bool guided{};             /* it also follows a zanzoken that way, a parry of it hurts the victim for nothing (rather than
                            damaging the room), and a miss of someone with no stamina left hurts them for nothing twice */
  bool homing{};             /* it follows a zanzoken as dodge_ki() with 1 after the victim pays (the attacker paying last),
                            and a miss of someone with no stamina left hurts them for nothing before the attacker pays
                            and again after (quirk) */
  bool parry_returns{true};  /* a parry sends it on (parry_ki()) */
  bool parry_scatters{true}; /* a parry that isn't guided damages the room as a dodge does */
  bool targets_first{};      /* the target is found before the charge is checked, rather than after */
  bool soft_crit{true};      /* BONUS_SOFT can make a hit on the body critical */
  int leg_flag{1};           /* hurt()'s first argument for a hit on a leg */
  bool dodge_scatters{true}; /* dodging it sends it into the room (dodge_ki(), room damage) and improves dodge */
  int obj_damtype{};         /* damtype()'s number for it at an object, if not damtype */
  bool multihit{};           /* handle_multihit() after a hit */
  bool needs_free_hand{};    /* not with both hands holding weapons */
  /* told to the room when it is dodged, if not null */
  const char *dodge_explosion{"@wA bright explosion erupts from the impact!\r\n"};
  /* more than one: the attacker pays on the last, or when one leaves the victim down; each after the first is a fifth
   * less likely to land, and only the first checks the victim is up */
  int shots{1};

  act_msgs parry; /* if it can be parried: parry_ki() sends it on */
  act_msgs block;
  act_msgs dodge;
  act_msgs miss;          /* when the victim could have avoided it */
  act_msgs miss_spent;    /* when the victim had no stamina left to avoid it */
  act_msgs hits[6];       /* by where it landed, 1-5 from roll_hitloc() (or 1-6 from locate) */
  const ki_spot *spots{}; /* what each of those is; null for roll_hitloc()'s body, head, body, arm, leg */
  const char *obj_to_char{};
  const char *obj_to_room{};
  bool obj_hide{}; /* act()'s hide_invisible for those two */

  /* each may be null */
  int (*cooldown_for)(ki_shot &shot){};                  /* handle_cooldown()'s argument, if not cooldown */
  double (*cost)(const ki_shot &shot){};                 /* the share of the charge the attacker pays, if not attperc */
  void (*steady)(ki_shot &shot, int &prob){};            /* the last word on its accuracy, just before zanzoken */
  bool (*through)(ki_shot &shot, int prob, int perc){};  /* whether it got past the victim, if not prob >= perc - 20 */
  void (*bonus_damage)(ki_shot &shot){};                 /* after the damage of a hit is rolled */
  int (*locate)(ki_shot &shot){};                        /* where a hit lands, instead of roll_hitloc() */
  int (*aim)(ki_shot &shot, int hitspot){};              /* where a hit lands, given roll_hitloc()\'s choice */
  void (*before_landing)(ki_shot &shot){};               /* before a hit lands, after the attack got through */
  bool (*strike)(ki_shot &shot, int where){};            /* after a hit's messages: true if it dealt the hit itself */
  void (*after_hit)(ki_shot &shot){};                    /* after a hit lands, before the attacker pays */
  void (*after_paying)(ki_shot &shot, ki_outcome how){}; /* right after the attacker pays, however it ended */
  bool (*on_object)(ki_shot &shot){};              /* aimed at an object: true if it dealt with it */
  void (*finish)(ki_shot &shot, ki_outcome how){}; /* last, however it ended at the victim but absorbed */
};

void perform_ki_attack(GameWorld *g, struct char_data *ch, std::string_view argument, const ki_attack &attack);

/* For hooks: the three messages of m about shot's victim */
void ki_act(const ki_shot &shot, const act_msgs &m);
