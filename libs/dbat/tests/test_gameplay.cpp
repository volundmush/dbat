/*
 * Rules of the game that were wrong at some point, and that it would be easy to get wrong again.
 * Each check runs in a process of its own.
 *
 * test_gameplay         all of them
 * test_gameplay NAME    one of them, here
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/wait.h>
#include <unistd.h>

#include "dbat/act.attack.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/act.other.hpp"
#include "dbat/affect.hpp"
#include "dbat/combat.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/senseis.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/db.hpp"
#include "dbat/genobj.hpp"
#include "dbat/fight.hpp"
#include "dbat/game_world.hpp"
#include "dbat/handler.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/players.hpp"
#include "dbat/relocate.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"

#include "harness.hpp"

static int failures = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    if (!(cond)) {                                               \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      failures++;                                                \
    }                                                            \
  } while (0)

static bool saw(struct char_data *ch, const char *text)
{
  return strstr(th_output(ch), text) != nullptr;
}

/* Plenty of everything, in the same room as anyone else this is done to. */
static struct char_data *fighter(GameWorld *g, const char *name, int admlevel, int race, int sensei)
{
  struct char_data *ch = th_player_as(g, name, admlevel, race, sensei);

  char_set_basepl(ch, 100000000);
  char_set_baseki(ch, 100000000);
  char_set_basest(ch, 100000000);
  restore(g, ch, false);
  char_from_room(g, ch);
  char_to_room(g, ch, 300);
  th_take_output(g, ch);
  return ch;
}

static void check_commands_wait_for_the_song_to_stop(void)
{
  static const struct {
    const char *name;
    void (*command)(GameWorld *, struct char_data *, char *, int, int);
  } commands[] = {
      {"throw", do_throw}, {"tslash", do_tslash}, {"pslash", do_pslash}, {"ddslash", do_ddslash},
      {"kousengan", do_kousengan}, {"heeldrop", do_heeldrop}, {"attack", do_attack}, {"grapple", do_grapple},
      {"zen", do_zen}, {"malice", do_malice}, {"head", do_head}, {"bash", do_bash},
      {"seishou", do_seishou}, {"eraser", do_eraser}, {"tailwhip", do_tailwhip}, {"steal", do_steal},
  };
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Singer", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);

  char_set_powerattack(ch, SONG_SHIELDING);
  for (size_t i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
    char argument[] = "nobody";

    th_take_output(g, ch);
    commands[i].command(g, ch, argument, 0, 0);
    if (!saw(ch, "currently playing a song")) {
      printf("FAIL: %s works during a song: %s\n", commands[i].name, th_output(ch));
      failures++;
    }
  }
  th_shutdown(g);
}

static void check_song_of_shielding_costs_ki(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Singer", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);
  struct obj_data *instrument;
  bool shielded = false;

  /* the fixture has no instrument, and adding one would shift what bots pick from it: make one here */
  struct obj_data *proto = g->obj_proto.emplace(8802);
  copy_object(g, proto, obj_proto_of(g, 350));
  obj_set_proto_script(proto, nullptr); /* copy_object() shares the original's trigger list */
  obj_set_item_number(proto, 8802);
  g->obj_index.emplace(8802)->vnum = 8802;
  instrument                       = read_object(g, 8802);
  obj_to_char(g, instrument, ch);
  char_set_skills_base(ch, SKILL_MYSTICMUSIC, 100);
  char_set_powerattack(ch, SONG_SHIELDING);
  for (int i = 0; i < 100; i++) {
    int64_t before;

    decCurKI(g, ch, getCurKI(g, ch) / 2, 0); /* room for it to wrongly go up */
    before = getCurKI(g, ch);
    handle_songs(g);
    CHECK(getCurKI(g, ch) <= before);
    shielded |= char_get_barrier(ch) > 0;
    restoreKI(ch, true);
  }
  CHECK(shielded); /* or this checked nothing */
  th_shutdown(g);
}

static void check_healing_glow_goes_on_the_target(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Healer", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *vict = fighter(g, "Patient", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);
  char argument[] = "patient";

  char_set_skills_base(ch, SKILL_HEALGLOW, 100);
  do_healglow(g, ch, argument, 0, 0);
  CHECK(saw(ch, "blue glow"));
  CHECK(is_affected(vict, AFF_HEALGLOW));
  CHECK(!is_affected(ch, AFF_HEALGLOW));
  th_shutdown(g);
}

static void hurt_all_over(GameWorld *g, struct char_data *ch)
{
  assign_affect(g, ch, AFF_POISON, SKILL_POISON, 5, 0, 0, 0, 0, 0, 0);
  decCurST(g, ch, getCurST(g, ch) / 2, 0);
  decCurHealthPercent(g, ch, .5, 0);
}

/* Until it takes: it can fail, and each attempt costs ki. */
static void heal_until_it_works(GameWorld *g, struct char_data *ch, const char *target)
{
  for (int i = 0; i < 200 && !saw(ch, "orange glow"); i++) {
    char argument[64];

    snprintf(argument, sizeof(argument), "%s", target);
    restoreKI(ch, true);
    do_heal(g, ch, argument, 0, 0);
  }
  if (!saw(ch, "orange glow"))
    printf("%s\n", th_output(ch));
  CHECK(saw(ch, "orange glow"));
}

static void check_heal_helps_the_patient(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Healer", ADMLVL_IMPL, RACE_NAMEK, CLASS_NAIL);
  struct char_data *vict = fighter(g, "Patient", ADMLVL_IMPL, RACE_SAIYAN, CLASS_ROSHI);
  int64_t healer_stamina, patient_stamina;

  char_set_skills_base(ch, SKILL_HEAL, 100);
  hurt_all_over(g, ch);
  hurt_all_over(g, vict);
  assign_affect(g, vict, AFF_BLIND, SKILL_SOLARF, 5, 0, 0, 0, 0, 0, 0);
  char_plr_set(vict, PLR_STAIL, false);
  healer_stamina  = getCurST(g, ch);
  patient_stamina = getCurST(g, vict);
  heal_until_it_works(g, ch, "patient");
  CHECK(!char_aff_flagged(vict, AFF_POISON));
  CHECK(!char_aff_flagged(vict, AFF_BLIND));
  CHECK(char_aff_flagged(ch, AFF_POISON));
  CHECK(getCurST(g, vict) > patient_stamina);
  CHECK(getCurST(g, ch) == healer_stamina);
  CHECK(char_plr_flagged(vict, PLR_STAIL));
  th_shutdown(g);
}

static void check_healing_yourself_cures_poison(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Healer", ADMLVL_IMPL, RACE_NAMEK, CLASS_NAIL);

  char_set_skills_base(ch, SKILL_HEAL, 100);
  hurt_all_over(g, ch);
  heal_until_it_works(g, ch, "healer");
  CHECK(!char_aff_flagged(ch, AFF_POISON));
  th_shutdown(g);
}

static void check_sacred_vial_stops_at_a_full_stomach(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Drinker", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);
  char argument[] = "item86";

  obj_to_char(g, read_object(g, 86), ch);
  char_set_conditions(g, ch, HUNGER, 40);
  char_set_conditions(g, ch, THIRST, 46);
  do_drink(g, ch, argument, 0, 0);
  CHECK(saw(ch, "uncork"));
  CHECK(char_get_conditions(g, ch, THIRST) <= 48);
  th_shutdown(g);
}

static void check_trait_limits_training_at_70(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Trainee", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  char first[] = "str", second[] = "str";

  char_set_bonuses(ch, BONUS_WIMP, 1);
  char_set_real_abils_str(ch, 50);
  do_train(g, ch, first, 0, 0);
  CHECK(!saw(ch, "not able to withstand"));
  th_take_output(g, ch);
  char_set_real_abils_str(ch, 70);
  do_train(g, ch, second, 0, 0);
  CHECK(saw(ch, "beyond 70"));
  th_shutdown(g);
}

static void check_stretchy_limbs_reach_one_level_up(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Stretchy", ADMLVL_NONE, RACE_NAMEK, CLASS_ROSHI);
  struct char_data *vict = fighter(g, "Flyer", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);

  char_set_class_level(ch, 50); /* past the newbie shield */
  char_set_class_level(vict, 50);
  char_set_altitude(vict, 1);
  can_kill(g, ch, vict, nullptr, 0);
  CHECK(saw(ch, "stretch your limbs"));
  th_shutdown(g);
}

/* How many of these deaths ended in a Zenkai: each in a process of its own, as it's a death. */
static int zenkais(GameWorld *g, struct char_data *ch, int tries)
{
  int count = 0;

  for (int i = 0; i < tries; i++) {
    int status = 0;
    pid_t child;

    fflush(stdout);
    if (!(child = fork())) {
      circle_srandom(g, 1000 + i);
      die(g, ch, nullptr);
      _exit(char_plr_flagged(ch, PLR_GOOP) ? 1 : 0);
    }
    waitpid(child, &status, 0);
    count += WIFEXITED(status) && WEXITSTATUS(status) == 1;
  }
  return count;
}

static void check_no_zenkai_for_a_spirit(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Saiyan", ADMLVL_NONE, RACE_SAIYAN, CLASS_ROSHI);

  CHECK(zenkais(g, ch, 40) > 0); /* or this checks nothing */
  char_aff_set(ch, AFF_SPIRIT, true);
  CHECK(zenkais(g, ch, 40) == 0);
  th_shutdown(g);
}

static void check_disguise_is_safe_out_of_combat(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Disguised", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);

  char_plr_set(ch, PLR_DISGUISED, true);
  char_set_charge(ch, 1); /* an aura, and not a fight: this used to be enough to lose it */
  for (int i = 0; i < 200; i++)
    fight_stack(g);
  CHECK(char_plr_flagged(ch, PLR_DISGUISED));
  th_shutdown(g);
}

static void check_staff_log_all_of_snet(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Speaker", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *nearby = fighter(g, "Nearby", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *far = fighter(g, "Faraway", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *mortal = fighter(g, "Mortal", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  struct obj_data *scouter = read_object(g, 350);
  char ordinary[] = "hello everyone", broadcast[] = "* big news", call[] = "#12345 just you";

  obj_wear_set(scouter, ITEM_WEAR_EYE, true);
  equip_char(g, ch, scouter, WEAR_EYE);
  char_from_room(g, far);
  char_to_room(g, far, 301);
  char_from_room(g, mortal);
  char_to_room(g, mortal, 301);
  do_snet(g, ch, ordinary, 0, 0);
  do_snet(g, ch, broadcast, 0, 0);
  do_snet(g, ch, call, 0, 0);
  for (struct char_data *staff = nearby; staff; staff = staff == nearby ? far : nullptr) {
    bool logged[3] = {};

    for (const std::string &line : *char_comm_hist(g, staff, HIST_SNET)) {
      logged[0] |= line.find("ello everyone") != std::string::npos;
      logged[1] |= line.find("Broadcast") != std::string::npos;
      logged[2] |= line.find("Call to") != std::string::npos;
    }
    CHECK(logged[0]);
    CHECK(logged[1]);
    CHECK(logged[2]);
    CHECK(saw(staff, "ello everyone"));
  }
  CHECK(!saw(mortal, "ello everyone")); /* no scouter */
  th_shutdown(g);
}

static void check_rp_bank_is_gone(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Admin", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);

  th_type(g, ch, "rpbank");
  CHECK(!saw(ch, "RPP Bank"));
  th_type(g, ch, "bexchange");
  CHECK(!saw(ch, "Syntax: bexchange"));
  th_type(g, ch, "rbank");
  CHECK(!saw(ch, "Syntax: rbank"));
  th_shutdown(g);
}

static void check_weapon_preference_uses_up_a_small_charge(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Swordsman", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);

  char_set_preference(ch, PREFERENCE_WEAPON);
  char_set_charge(ch, 1);
  damtype(g, ch, -1, 1, 1.0);
  CHECK(char_get_charge(ch) == 0);
  th_shutdown(g);
}

/* What this much damage comes to, on the same roll each time. */
static double damage_taken(GameWorld *g, struct char_data *ch, struct char_data *vict, int type)
{
  int64_t before;

  restore(g, vict, false);
  before = getCurPL(g, vict);
  circle_srandom(g, 42);
  hurt(g, 0, 0, ch, vict, nullptr, 1000000, type);
  stop_fighting(g, ch);
  stop_fighting(g, vict);
  return (double)(before - getCurPL(g, vict));
}

static bool about(double value, double expected)
{
  return fabs(value - expected) < 0.03;
}

static void check_what_reduces_damage(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Attacker", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *vict = fighter(g, "Target", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  double ki, physical, ratio;

  char_set_class_level(vict, 100);
  ki       = damage_taken(g, ch, vict, 1);
  physical = damage_taken(g, ch, vict, 0);
  CHECK(ki > 0 && physical > 0);

  /* a charged ki user takes less: it's the one being hit who matters, not the one hitting */
  char_set_preference(ch, PREFERENCE_KI);
  char_set_charge(ch, 1000);
  ratio = damage_taken(g, ch, vict, 1) / ki;
  CHECK(about(ratio, 1.0));
  char_set_preference(ch, 0);
  char_set_charge(ch, 0);
  char_set_preference(vict, PREFERENCE_KI);
  char_set_charge(vict, 1000);
  ratio = damage_taken(g, ch, vict, 1) / ki;
  CHECK(about(ratio, 0.92));
  char_set_preference(vict, 0);
  char_set_charge(vict, 0);

  char_set_bonuses(vict, BONUS_LEECH, 1);
  ratio = damage_taken(g, ch, vict, 1) / ki;
  CHECK(about(ratio, 0.60));
  ratio = damage_taken(g, ch, vict, 0) / physical;
  CHECK(about(ratio, 1.0));
  char_set_bonuses(vict, BONUS_LEECH, 0);

  char_set_bonuses(vict, BONUS_FIREPROOF, 1);
  ratio = damage_taken(g, ch, vict, 1) / ki;
  CHECK(about(ratio, 0.90));
  ratio = damage_taken(g, ch, vict, 0) / physical;
  CHECK(about(ratio, 1.0));
  th_shutdown(g);
}

static void check_multihit_repeats_the_last_attack(void)
{
  GameWorld *g = th_boot();
  /* an admin, since mortals can't pick out someone they haven't been introduced to */
  struct char_data *ch = fighter(g, "Attacker", ADMLVL_IMPL, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *vict = fighter(g, "Target", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  bool repeated = false;

  char_set_class_level(ch, 50); /* past the newbie shield */
  char_set_class_level(vict, 50);
  char_set_skills_base(ch, SKILL_ELBOW, 100);
  for (int i = 0; i < 2000 && !repeated; i++) {
    th_take_output(g, ch);
    char_plr_set(ch, PLR_MULTIHIT, false);
    char_set_throws(ch, 0);
    char_set_wait(ch, 0);
    char_set_lastattack(ch, 2);
    handle_multihit(g, ch, vict);
    repeated = saw(ch, "lightning flash");
  }
  CHECK(repeated);
  CHECK(saw(ch, "elbow")); /* and not the punch or kick it used to fall back on */
  th_shutdown(g);
}

static void check_mob_experience_stops_at_zero(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Killer", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *mob = read_mobile(g, 302);
  int64_t before = char_get_exp(ch);

  char_to_room(g, mob, char_get_in_room(ch));
  char_set_class_level(ch, 50);
  /* the least a kill pays is 1, even for a mob with nothing to give */
  char_set_exp(mob, 0);
  solo_gain(g, ch, mob);
  CHECK(char_get_exp(mob) == 0);
  CHECK(char_get_exp(ch) > before);
  char_set_exp(mob, 100);
  gain_exp(g, mob, -1000);
  CHECK(char_get_exp(mob) == 0);
  th_shutdown(g);
}

static const char *mob_level;

static void change_mob_levels(void)
{
  char command[128];

  snprintf(command, sizeof(command), "sed -i 's/^1 0 10 0d0+0/%s 0 10 0d0+0/' world/mob/3.mob", mob_level);
  if (system(command) != 0)
    abort();
}

static void check_mob_experience_at_impossible_levels(void)
{
  static const char *const levels[] = {"20000", "-5"};

  for (size_t i = 0; i < sizeof(levels) / sizeof(*levels); i++) {
    GameWorld *g;
    struct char_data *mob;

    mob_level = levels[i];
    th_before_boot(change_mob_levels);
    g   = th_boot();
    mob = read_mobile(g, 302);
    CHECK(char_get_level(mob) == atoi(levels[i])); /* or this checks nothing */
    CHECK(char_get_exp(mob) >= 0);
    CHECK(char_get_exp(mob) <= 20000000);
    char_to_room(g, mob, 300);
    th_shutdown(g);
  }
}

static void check_experience_past_32_bits_survives_a_save(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = fighter(g, "Veteran", ADMLVL_NONE, RACE_HUMAN, CLASS_ROSHI);
  struct char_data *copy;

  char_set_exp(ch, 3000000000LL);
  save_char(g, ch);
  copy = new char_data();
  clear_char(copy);
  char_make_player_specials(copy);
  CHECK(load_char(g, "Veteran", copy) >= 0);
  CHECK(char_get_exp(copy) == 3000000000LL);
  free_char(g, copy);
  th_shutdown(g);
}

static const struct {
  const char *name;
  void (*check)(void);
} checks[] = {
    {"commands_wait_for_the_song_to_stop", check_commands_wait_for_the_song_to_stop},
    {"song_of_shielding_costs_ki", check_song_of_shielding_costs_ki},
    {"healing_glow_goes_on_the_target", check_healing_glow_goes_on_the_target},
    {"heal_helps_the_patient", check_heal_helps_the_patient},
    {"healing_yourself_cures_poison", check_healing_yourself_cures_poison},
    {"sacred_vial_stops_at_a_full_stomach", check_sacred_vial_stops_at_a_full_stomach},
    {"trait_limits_training_at_70", check_trait_limits_training_at_70},
    {"stretchy_limbs_reach_one_level_up", check_stretchy_limbs_reach_one_level_up},
    {"no_zenkai_for_a_spirit", check_no_zenkai_for_a_spirit},
    {"disguise_is_safe_out_of_combat", check_disguise_is_safe_out_of_combat},
    {"staff_log_all_of_snet", check_staff_log_all_of_snet},
    {"rp_bank_is_gone", check_rp_bank_is_gone},
    {"weapon_preference_uses_up_a_small_charge", check_weapon_preference_uses_up_a_small_charge},
    {"what_reduces_damage", check_what_reduces_damage},
    {"multihit_repeats_the_last_attack", check_multihit_repeats_the_last_attack},
    {"mob_experience_stops_at_zero", check_mob_experience_stops_at_zero},
    {"mob_experience_at_impossible_levels", check_mob_experience_at_impossible_levels},
    {"experience_past_32_bits_survives_a_save", check_experience_past_32_bits_survives_a_save},
};

int main(int argc, char **argv)
{
  int failed = 0;

  for (size_t i = 0; i < sizeof(checks) / sizeof(*checks); i++) {
    int status = 0;
    pid_t child;

    if (argc > 1 && strcmp(argv[1], checks[i].name))
      continue;
    fflush(stdout);
    if (!(child = fork())) {
      alarm(120);
      checks[i].check();
      th_exit(failures != 0);
    }
    waitpid(child, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status)) {
      printf("%s failed (%s %d)\n", checks[i].name, WIFEXITED(status) ? "status" : "signal", WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status));
      failed++;
    }
  }
  if (failed)
    printf("%d check(s) failed\n", failed);
  else
    printf("all checks passed\n");
  return failed != 0;
}
