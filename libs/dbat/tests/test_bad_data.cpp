/*
 * What the game does with a script that has a mistake in it, a file that's damaged, and an edit that
 * would lose something. Each check runs in a process of its own, as most of these used to crash or hang.
 *
 * test_bad_data         all of them
 * test_bad_data NAME    one of them, here
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/wait.h>
#include <unistd.h>

#include "dbat/iterate.hpp"
#include "dbat/affect.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/applies.hpp"
#include "dbat/consts/feats.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/db.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genwld.hpp"
#include "dbat/handler.hpp"
#include "dbat/objsave.hpp"
#include "dbat/relocate.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/shop.hpp"
#include "dbat/world_audit.hpp"

#include "harness.hpp"
#include "../src/dg/dg_internal.hpp"

static int failures = 0;

constexpr int CONTAINER_VNUM = 300;

/* so that the game refusing a file, with an exit of 1, can be told from a memory error */
constexpr int MEMORY_ERROR = 99;
const char *__asan_default_options(void);
const char *__asan_default_options(void)
{
  return "exitcode=99";
}

#define CHECK(cond)                                              \
  do {                                                           \
    if (!(cond)) {                                               \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      failures++;                                                \
    }                                                            \
  } while (0)

static void print_problem(void *, const char *problem)
{
  printf("AUDIT: %s\n", problem);
}

#define CHECK_AUDIT(g) CHECK(world_audit((g), print_problem, nullptr) == 0)

static bool saw(struct char_data *ch, const char *text)
{
  return strstr(th_output(ch), text) != nullptr;
}

static void write_file(const char *path, const char *text)
{
  FILE *fl = fopen(path, "w");

  if (!fl) {
    perror(path);
    exit(2);
  }
  fputs(text, fl);
  fclose(fl);
}

static void append_file(const char *path, const char *text)
{
  FILE *fl = fopen(path, "a");

  if (!fl) {
    perror(path);
    exit(2);
  }
  fputs(text, fl);
  fclose(fl);
}

static const char *script_text;

/* Over the $~ at the end of the file. */
static void add_script_to_trigger_file(void)
{
  FILE *fl = fopen("world/trg/3.trg", "r+");

  if (!fl)
    abort();
  fseek(fl, -3, SEEK_END);
  fprintf(fl, "#399\nScript~\n2 c 100\nrunit~\n%s\n~\n$~\n", script_text);
  fclose(fl);
}

/* Boots a game with this script in the room the player is in, and has the player set it off. */
static GameWorld *run_script(const char *script, struct char_data **player)
{
  GameWorld *g;
  struct char_data *ch;

  script_text = script;
  th_before_boot(add_script_to_trigger_file);
  g  = th_boot();
  ch = th_player(g, "Tester", ADMLVL_NONE);
  CHECK(real_trigger(g, 399) != NOTHING);
  add_trigger(g, dg_ensure_script(room_script_slot(&g->rooms[char_get_in_room(ch)])), read_trigger(g, real_trigger(g, 399)), -1);
  th_type(g, ch, "runit");
  th_pulses(g, 20);
  *player = ch;
  return g;
}

/* Where a player's save is, for one who has just been made. */
static void save_path(const char *kind, const char *extension, const char *name, char *path, size_t size)
{
  snprintf(path, size, "%s/P-T/%s.%s", kind, name, extension);
}

/* Loads this player's save again, into a character of its own, the way logging in does. */
static struct char_data *load_again(GameWorld *g, const char *name)
{
  struct char_data *copy;

  copy = new char_data();
  clear_char(copy);
  *char_player_specials_slot(copy) = new player_special_data();
  CHECK(load_char(g, name, copy) >= 0);
  return copy;
}

/* ------ scripts ------ */

static void check_while_without_done(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("while 0\nwhile 1\nwhile 1\n%echo% x", &ch);

  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_break_without_done(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("break\nwhile 1\n ", &ch);

  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_switch_around_an_unfinished_while(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("switch 1\nwhile 1\nwhile 1\n%echo% x", &ch);

  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_a_finished_while_still_runs(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("set n 0\nwhile %n% < 3\n  eval n %n% + 1\n  %echo% round %n%\ndone\n%echo% after", &ch);

  CHECK(saw(ch, "Round 3"));
  CHECK(saw(ch, "After"));
  th_shutdown(g);
}

static void check_long_chain_of_operators(void)
{
  struct char_data *ch;
  /* what overflowed the stack, at about 190KB a level */
  GameWorld *g = run_script("eval x 1-------------------------------------------------------------1\n%echo% survived", &ch);

  CHECK(saw(ch, "Survived"));
  th_shutdown(g);
}

static void check_chain_as_long_as_real_scripts_have(void)
{
  char script[4096] = "if 1 == 2";
  struct char_data *ch;
  GameWorld *g;

  /* the longest in the game's real triggers has 19 of these */
  for (int i = 0; i < 60; i++)
    strcat(script, " || 1 == 2");
  strcat(script, " || 1 == 1\n  %echo% matched\nend");
  g = run_script(script, &ch);
  CHECK(saw(ch, "Matched"));
  th_shutdown(g);
}

static void check_expression_values(void)
{
  static const struct {
    const char *expression, *value;
  } cases[] = {
      {"1 + 2", "3"},
      {"2 * 3 + 1", "7"},
      {"(1 + 2) * 3", "9"},
      {"10 - 2 - 3", "11"},
      {"1 == 1 && 2 > 1", "1"},
      {"1 == 2 || 2 == 3", "0"},
      {"abc == abc", "1"},
      {"\"a + b\" == \"a + b\"", "1"},
      {"!0", "1"},
      {"((((5))))", "5"},
  };
  GameWorld *g = th_boot();

  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); i++) {
    char line[256], result[MAX_INPUT_LENGTH] = "";

    snprintf(line, sizeof(line), "%s", cases[i].expression);
    eval_expr(g, line, result, nullptr, nullptr, nullptr, WLD_TRIGGER);
    if (strcmp(result, cases[i].value)) {
      printf("FAIL: %s gave '%s', not '%s'\n", cases[i].expression, result, cases[i].value);
      failures++;
    }
  }
  CHECK(g->dg_eval_depth == 0);
  th_shutdown(g);
}

/* ------ affects ------ */

static void check_specific_goes_with_its_location(void)
{
  CHECK(affect_checked_specific(APPLY_SKILL, SKILL_HAYASA) == SKILL_HAYASA);
  CHECK(affect_checked_specific(APPLY_SKILL, SKILL_TABLE_SIZE) == 0);
  CHECK(affect_checked_specific(APPLY_SKILL, -1) == 0);
  CHECK(affect_checked_specific(APPLY_FEAT, NUM_FEATS_DEFINED - 1) == NUM_FEATS_DEFINED - 1);
  CHECK(affect_checked_specific(APPLY_FEAT, NUM_FEATS_DEFINED) == 0);
  CHECK(affect_checked_specific(APPLY_STR, 1234) == 0);
}

static void check_affects_on_two_skills_stay_apart(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct affected_type first = {.type = SPELL_SLEEP, .duration = 5, .modifier = 2, .location = APPLY_SKILL, .specific = SKILL_HAYASA};
  struct affected_type second = {.type = SPELL_SLEEP, .duration = 5, .modifier = 3, .location = APPLY_SKILL, .specific = SKILL_FOCUS};
  int hayasa = char_get_skills_mod(ch, SKILL_HAYASA), focus = char_get_skills_mod(ch, SKILL_FOCUS);

  affect_join(g, ch, &first, false, false, false, false);
  affect_join(g, ch, &second, false, false, false, false);
  CHECK(char_get_skills_mod(ch, SKILL_HAYASA) == hayasa + 2);
  CHECK(char_get_skills_mod(ch, SKILL_FOCUS) == focus + 3);
  affect_from_char(g, ch, SPELL_SLEEP);
  CHECK(char_get_skills_mod(ch, SKILL_HAYASA) == hayasa);
  CHECK(char_get_skills_mod(ch, SKILL_FOCUS) == focus);
  th_shutdown(g);
}

static void check_saved_affects_are_cleaned_up(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE), *copy;
  char path[256];

  save_char(g, ch);
  save_path("plrfiles", "plr", "tester", path, sizeof(path));
  /* the same list, later in the file, adds to it */
  append_file(path, "Affs:\n"
                    "10 5 1 1 0 77777\n"            /* garbage in specific, as older saves have */
                    "10 5 1 41 0 999999\n"          /* a skill that doesn't exist */
                    "10 5 1 1 2000000 0\n"          /* a flag that doesn't exist */
                    "999999 5 1 1 0 0\n"            /* a spell that doesn't exist */
                    "10 5 2000000000 1 0 0\n"       /* a modifier that would overflow */
                    "0 0 0 0 0 0\n");
  copy = load_again(g, "tester");

  int count = 0;

  for (struct affected_type *af = char_get_affected(copy); af; af = af->next) {
    CHECK(af->specific == 0);
    count++;
  }
  CHECK(count == 2);
  free_char(g, copy);
  th_shutdown(g);
}

static void check_infravision_comes_off_an_android(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player_as(g, "Tester", ADMLVL_NONE, RACE_ANDROID, 0);
  struct affected_type af = {.type = SPELL_SLEEP, .duration = 5, .modifier = 7, .location = APPLY_ACCURACY, .bitvector = AFF_INFRAVISION};
  int accuracy = char_get_accuracy(ch);

  affect_to_char(g, ch, &af);
  CHECK(char_get_accuracy(ch) == accuracy + 7);
  affect_from_char(g, ch, SPELL_SLEEP);
  CHECK(char_get_accuracy(ch) == accuracy);
  th_shutdown(g);
}

/* ------ player files ------ */

static void check_cut_off_skill_list(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE), *copy;
  char path[256];

  save_char(g, ch);
  save_path("plrfiles", "plr", "tester", path, sizeof(path));
  append_file(path, "Skil:\n5 1 0\n"); /* and no 0 to end it */
  copy = load_again(g, "tester");
  free_char(g, copy);
  th_shutdown(g);
}

static void check_impossible_indexes_in_a_save(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE), *copy;
  char path[256];

  save_char(g, ch);
  save_path("plrfiles", "plr", "tester", path, sizeof(path));
  append_file(path, "Colr: 999999 x\nColr: -5 x\nTcos: 9999 1\nTcos: -1 1\n");
  copy = load_again(g, "tester");
  free_char(g, copy);
  th_shutdown(g);
}

static void check_birth_at_the_end_of_time(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE), *copy;
  char path[256];

  save_char(g, ch);
  save_path("plrfiles", "plr", "tester", path, sizeof(path));
  append_file(path, "Brth: 9223372036854775807\nMxAg: 0\n");
  copy = load_again(g, "tester");
  free_char(g, copy);
  th_shutdown(g);
}

static void check_remote_display_survives_a_load(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE), *copy;

  char_set_rdisplay(ch, "stands here, waving");
  save_char(g, ch);
  copy = load_again(g, "tester");
  CHECK(char_get_rdisplay(copy) && !strcmp(char_get_rdisplay(copy), "stands here, waving"));
  free_char(g, copy);
  th_shutdown(g);
}

static void check_no_object_file_is_no_problem(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);

  Crash_delete_file("tester");
  th_take_output(g, ch);
  Crash_load(g, ch);
  CHECK(!saw(ch, "problem loading"));
  th_shutdown(g);
}

static void check_too_many_applies_on_a_saved_object(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  char path[256];
  char text[4096] = "1\n#350\n0\nXAP\n~\n~\n.~\n~\n2 0 0 0 0 0 0 0\n";

  for (int i = 0; i < 40; i++)
    strcat(text, "A\n1 1 77777\n");
  strcat(text, "$~\n");
  save_path("plrobjs", "new", "tester", path, sizeof(path));
  write_file(path, text);
  Crash_load(g, ch);
  CHECK(char_get_inventory(ch) != nullptr);
  while (char_get_inventory(ch))
    extract_obj(g, char_get_inventory(ch));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* ------ world files ------ */

static void empty_socials(void)
{
  write_file("misc/socials.new", "$\n");
}

static void check_boot_with_no_socials(void)
{
  th_before_boot(empty_socials);
  GameWorld *g = th_boot();

  CHECK(g->top_of_socialt == -1);
  CHECK(system("! grep -q 'Zero bytes' test.log") == 0);
  th_shutdown(g);
}

static void index_without_an_end(void)
{
  write_file("world/shp/index", "nothing-here.shp\n");
}

static void check_index_without_an_end(void)
{
  th_before_boot(index_without_an_end);
  GameWorld *g = th_boot();

  th_shutdown(g);
}

static void guild_with_impossible_numbers(void)
{
  FILE *in = fopen("world/gld/3.gld", "r");
  char line[512], text[65536] = "";
  bool first = true;

  if (!in)
    abort();
  while (fgets(line, sizeof(line), in)) {
    strcat(text, line);
    if (*line == '#' && first) {
      strcat(text, "999999 1\n999999 2\n999999\n");
      first = false;
    }
  }
  fclose(in);
  write_file("world/gld/3.gld", text);
}

static void check_guild_with_impossible_numbers(void)
{
  th_before_boot(guild_with_impossible_numbers);
  GameWorld *g = th_boot();

  th_shutdown(g);
}

/* ------ play ------ */

static void check_carrying_more_than_a_byte_can_count(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);

  int before = char_get_carry_items(ch);

  for (int i = 0; i < 300; i++)
    obj_to_char(g, read_object(g, 350, VIRTUAL), ch);
  CHECK(char_get_carry_items(ch) == before + 300);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_mob_memory_gets_forgotten(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *mob = read_mobile(g, 300, VIRTUAL);
  struct script_memory *mem;

  char_to_room(g, mob, char_get_in_room(ch));
  mem      = new script_memory();
  mem->id  = char_get_id(ch);
  mem->cmd = strdup("say I remember you");
  char_set_memory(mob, mem);
  greet_memory_mtrigger(g, ch);
  CHECK(saw(ch, "I remember you"));
  CHECK(char_get_memory(mob) == nullptr);
  th_shutdown(g);
}

static void check_android_without_a_model(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player_as(g, "Tester", ADMLVL_NONE, RACE_ANDROID, 0);

  th_type(g, ch, "score personal");
  CHECK(saw(ch, "None"));
  th_shutdown(g);
}

static void check_race_name_forms_stay_apart(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player_as(g, "Tester", ADMLVL_NONE, RACE_HUMAN, 0);
  const char *upper = juggleRaceName(ch, true), *lower = juggleRaceName(ch, false);

  CHECK(!strcmp(upper, "Human"));
  CHECK(!strcmp(lower, "human"));
  th_shutdown(g);
}

static void check_zone_range_keeps_what_is_in_it(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_IMPL);
  zone_rnum zone = real_zone(g, 3);
  int bot = g->zones[zone].bot, top = g->zones[zone].top;

  th_type(g, ch, "zedit 300");
  th_type(g, ch, "t");
  th_type(g, ch, "300");
  CHECK(saw(ch, "Range not changed"));
  th_type(g, ch, "b");
  th_type(g, ch, "399");
  CHECK(saw(ch, "Range not changed"));
  th_type(g, ch, "q");
  th_type(g, ch, "y");
  CHECK(g->zones[zone].bot == bot);
  CHECK(g->zones[zone].top == top);
  CHECK(real_room(g, 301) != NOWHERE);
  th_shutdown(g);
}

static void write_long_word_line(const char *path, const char *before, const char *after)
{
  FILE *fl = fopen(path, "w");

  if (!fl)
    abort();
  fputs(before, fl);
  for (int i = 0; i < 70000; i++)
    putc('x', fl);
  fputs(after, fl);
  fclose(fl);
}

static void long_words_in_ban_file(void)
{
  write_long_word_line("etc/badsites", "ban ", " 0 someone\n");
}

static void check_long_words_in_ban_file(void)
{
  th_before_boot(long_words_in_ban_file);
  GameWorld *g = th_boot();

  th_shutdown(g);
}

static void long_word_in_socials(void)
{
  write_long_word_line("misc/socials.new", "~", " x 0 0 0 0\n$\n");
}

static void check_long_word_in_socials(void)
{
  int status = 0;
  pid_t child;

  /* the game refuses this file, and says why; what matters is how it goes about it */
  fflush(stdout);
  if (!(child = fork())) {
    th_before_boot(long_word_in_socials);
    th_boot();
    th_exit(0);
  }
  waitpid(child, &status, 0);
  CHECK(WIFEXITED(status) && WEXITSTATUS(status) != MEMORY_ERROR);
}

static void long_word_in_index(void)
{
  write_long_word_line("world/shp/index", "", "\n$\n");
}

static void check_long_word_in_index(void)
{
  th_before_boot(long_word_in_index);
  GameWorld *g = th_boot();

  th_shutdown(g);
}

static void check_more_affects_than_a_save_holds(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);

  for (int i = 0; i < MAX_AFFECT + 20; i++) {
    struct affected_type af = {.type = SPELL_SLEEP, .duration = 5, .modifier = 1, .location = APPLY_ACCURACY};

    affect_to_char(g, ch, &af);
  }
  save_char(g, ch);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_worn_container_that_cannot_go_back_on(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct obj_data *bag = read_object(g, CONTAINER_VNUM, VIRTUAL);
  struct obj_data *inside = read_object(g, 350, VIRTUAL);

  while (char_get_inventory(ch))
    extract_obj(g, char_get_inventory(ch));
  for (int i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i))
      extract_obj(g, char_get_equipment(ch, i));
  obj_to_obj(g, inside, bag);
  equip_char(g, ch, bag, WEAR_HEAD);
  CHECK(char_get_equipment(ch, WEAR_HEAD) == bag);
  Crash_crashsave(g, ch);
  extract_obj(g, char_get_equipment(ch, WEAR_HEAD));

  /* now something else is on that spot, as can happen when a save has two things there */
  equip_char(g, ch, read_object(g, 350, VIRTUAL), WEAR_HEAD);
  Crash_load(g, ch);

  struct obj_data *back = nullptr;

  for (obj_data *obj : char_inventory(g, ch))
    if (obj_get_vnum(g, obj) == CONTAINER_VNUM)
      back = obj;
  CHECK(back != nullptr);
  CHECK(back && obj_get_contains(back) != nullptr);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_message_about_an_object_with_no_description(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct obj_data *obj = read_object(g, 350, VIRTUAL);
  char *description = obj_get_short_description(obj);

  obj_set_short_description(obj, nullptr);
  act(g, "You look at $p.", false, ch, obj, nullptr, TO_CHAR);
  CHECK(saw(ch, "You look at"));
  obj_set_short_description(obj, description);
  th_shutdown(g);
}

static void move_next_to(GameWorld *g, struct char_data *ch, struct char_data *other)
{
  char_from_room(g, ch);
  char_to_room(g, ch, char_get_in_room(other));
}

static void check_saying_something_to_someone(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *listener = th_player(g, "Friend", ADMLVL_NONE);

  move_next_to(g, listener, ch);
  th_take_output(g, listener);
  th_type(g, ch, "say to Friend hello there");
  CHECK(saw(listener, "says to"));
  CHECK(saw(listener, "hello there"));
  CHECK(!saw(listener, "to Friend hello"));
  th_shutdown(g);
}

static void check_defending_someone_else_lets_go_of_the_first(void)
{
  /* mortals who haven't been introduced can't pick each other out */
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_IMPL);
  struct char_data *first = th_player(g, "First", ADMLVL_IMPL);
  struct char_data *second = th_player(g, "Second", ADMLVL_IMPL);
  struct char_data *rival = th_player(g, "Rival", ADMLVL_IMPL);

  move_next_to(g, first, ch);
  move_next_to(g, second, ch);
  move_next_to(g, rival, ch);
  th_type(g, ch, "defend first");
  CHECK(char_get_defender(first) == ch);
  th_take_output(g, first);
  th_type(g, ch, "defend second");
  CHECK(saw(first, "stops defending you"));
  CHECK(char_get_defender(first) == nullptr);
  CHECK(char_get_defender(second) == ch);
  CHECK(char_get_defending(ch) == second);
  CHECK_AUDIT(g);
  /* one defender at a time: a second takes over from the first, who may not be there to see it */
  char_from_room(g, ch);
  char_to_room(g, ch, real_room(g, 301));
  th_take_output(g, ch);
  th_type(g, rival, "defend second");
  CHECK(saw(ch, "no longer defending"));
  CHECK(char_get_defender(second) == rival);
  CHECK(char_get_defending(ch) == nullptr);
  CHECK_AUDIT(g);
  th_take_output(g, first);
  th_type(g, rival, "defend second"); /* nothing has changed, so nothing to tell */
  CHECK(!saw(first, "stops defending"));
  CHECK(char_get_defender(second) == rival);
  th_shutdown(g);
}

static void check_mob_given_armor_with_no_wear_slot(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *mob = read_mobile(g, 300, VIRTUAL);
  struct obj_data *armor = read_object(g, 350, VIRTUAL);

  CHECK(mob != nullptr);
  char_to_room(g, mob, char_get_in_room(ch));
  obj_set_type_flag(armor, ITEM_WORN);
  obj_to_char(g, armor, mob); /* only "take", and nowhere to wear it */
  item_check(g, armor, mob);
  CHECK(obj_get_carried_by(armor) == mob);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_start_room_cannot_be_deleted(void)
{
  GameWorld *g = th_boot();
  room_rnum start = real_room(g, CONFIG_MORTAL_START);

  CHECK(start != NOWHERE);
  CHECK(!delete_room(g, start));
  CHECK(real_room(g, CONFIG_MORTAL_START) == start);
  CHECK(g->r_mortal_start_room == start);
  th_shutdown(g);
}

static void point_config_at_a_missing_room(void)
{
  append_file("etc/config", "mortal_start_room = 99999\n");
}

static void check_start_room_missing_from_the_world(void)
{
  GameWorld *g;

  th_before_boot(point_config_at_a_missing_room);
  g = th_boot(); /* used to refuse to start */
  CHECK(g->r_mortal_start_room == 0);
  CHECK(g->r_immort_start_room != NOWHERE);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void give_the_shop_impossible_item_types(void)
{
  FILE *fl = fopen("world/shp/3.shp", "w");

  if (!fl)
    abort();
  fputs("CircleMUD v3.0 Shop File~\n#300~\n302\n-1\n1.00\n0.50\n"
        "110\nARMOR potions\n999 ignored keywords\n19\n-1\n"
        "%s Sorry, I don't stock that item.~\n%s You don't seem to have that.~\n"
        "%s I don't trade in such items.~\n%s I can't afford that!~\n%s You are too poor!~\n"
        "%s That'll be %d zenni, thanks.~\n%s I'll give you %d zenni for that.~\n"
        "0\n0\n301\n0 0 16384 0\n303\n-1\n0\n28\n0\n0\n$~\n", fl);
  fclose(fl);
}

static void check_shop_with_impossible_item_types(void)
{
  GameWorld *g;

  th_before_boot(give_the_shop_impossible_item_types);
  g = th_boot();
  CHECK(g->top_shop == 0); /* the one shop, still there */
  CHECK(rshop_get_buy_type(g, 0, 0) == ITEM_ARMOR);
  CHECK(rshop_get_buy_word(g, 0, 0) && !strcmp(rshop_get_buy_word(g, 0, 0), "potions"));
  CHECK(rshop_get_buy_type(g, 0, 1) == 19);
  CHECK(rshop_get_buy_word(g, 0, 1) == nullptr);
  CHECK(rshop_get_buy_type(g, 0, 2) == NOTHING);
  th_shutdown(g);
}

static void save_a_shadow_dragon_that_does_not_exist(void)
{
  FILE *fl = fopen(TIME_FILE, "w");

  if (!fl)
    abort();
  for (int line = 0; line < 17; line++) /* what comes before the dragons */
    fputs("1\n", fl);
  fputs("10\n300\n-1\n-1\n-1\n-1\n-1\n-1\n0\n", fl);
  fclose(fl);
}

static void check_shadow_dragon_missing_from_the_world(void)
{
  GameWorld *g;

  th_before_boot(save_a_shadow_dragon_that_does_not_exist);
  g = th_boot();
  CHECK(real_mobile(g, SHADOW_DRAGON1_VNUM) == NOBODY); /* or this checks nothing */
  CHECK(g->SHADOW_DRAGON1 == -1);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_homing_ball_that_someone_is_carrying(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *target = th_player(g, "Target", ADMLVL_NONE);
  struct obj_data *ball = read_object(g, 80, VIRTUAL);
  struct obj_data *other = read_object(g, 350, VIRTUAL);

  char_from_room(g, target);
  char_to_room(g, target, real_room(g, 301));
  obj_to_char(g, other, ch);
  obj_to_char(g, ball, ch);
  obj_set_kicharge(ball, 100);
  obj_set_user(ball, ch);
  obj_set_target(ball, target);
  homing_update(g);
  CHECK(obj_get_carried_by(ball) == ch);
  CHECK(obj_get_in_room(ball) == NOWHERE);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_long_name_in_auction_file(void)
{
  GameWorld *g = th_boot();
  struct obj_data *obj = read_object(g, 350, VIRTUAL);
  char text[400], name[201];

  memset(name, 'x', 200);
  name[200] = '\0';
  snprintf(text, sizeof(text), "1 %s 1 1 1 1 1\n", name);
  write_file(AUCTION_FILE, text);
  auc_load(obj);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static const struct {
  const char *name;
  void (*check)(void);
} checks[] = {
    {"while_without_done", check_while_without_done},
    {"break_without_done", check_break_without_done},
    {"switch_around_an_unfinished_while", check_switch_around_an_unfinished_while},
    {"a_finished_while_still_runs", check_a_finished_while_still_runs},
    {"long_chain_of_operators", check_long_chain_of_operators},
    {"chain_as_long_as_real_scripts_have", check_chain_as_long_as_real_scripts_have},
    {"expression_values", check_expression_values},
    {"specific_goes_with_its_location", check_specific_goes_with_its_location},
    {"affects_on_two_skills_stay_apart", check_affects_on_two_skills_stay_apart},
    {"saved_affects_are_cleaned_up", check_saved_affects_are_cleaned_up},
    {"infravision_comes_off_an_android", check_infravision_comes_off_an_android},
    {"cut_off_skill_list", check_cut_off_skill_list},
    {"impossible_indexes_in_a_save", check_impossible_indexes_in_a_save},
    {"birth_at_the_end_of_time", check_birth_at_the_end_of_time},
    {"remote_display_survives_a_load", check_remote_display_survives_a_load},
    {"no_object_file_is_no_problem", check_no_object_file_is_no_problem},
    {"too_many_applies_on_a_saved_object", check_too_many_applies_on_a_saved_object},
    {"boot_with_no_socials", check_boot_with_no_socials},
    {"index_without_an_end", check_index_without_an_end},
    {"guild_with_impossible_numbers", check_guild_with_impossible_numbers},
    {"carrying_more_than_a_byte_can_count", check_carrying_more_than_a_byte_can_count},
    {"mob_memory_gets_forgotten", check_mob_memory_gets_forgotten},
    {"android_without_a_model", check_android_without_a_model},
    {"race_name_forms_stay_apart", check_race_name_forms_stay_apart},
    {"zone_range_keeps_what_is_in_it", check_zone_range_keeps_what_is_in_it},
    {"worn_container_that_cannot_go_back_on", check_worn_container_that_cannot_go_back_on},
    {"long_words_in_ban_file", check_long_words_in_ban_file},
    {"long_word_in_socials", check_long_word_in_socials},
    {"long_word_in_index", check_long_word_in_index},
    {"more_affects_than_a_save_holds", check_more_affects_than_a_save_holds},
    {"message_about_an_object_with_no_description", check_message_about_an_object_with_no_description},
    {"saying_something_to_someone", check_saying_something_to_someone},
    {"defending_someone_else_lets_go_of_the_first", check_defending_someone_else_lets_go_of_the_first},
    {"mob_given_armor_with_no_wear_slot", check_mob_given_armor_with_no_wear_slot},
    {"start_room_cannot_be_deleted", check_start_room_cannot_be_deleted},
    {"start_room_missing_from_the_world", check_start_room_missing_from_the_world},
    {"shop_with_impossible_item_types", check_shop_with_impossible_item_types},
    {"shadow_dragon_missing_from_the_world", check_shadow_dragon_missing_from_the_world},
    {"homing_ball_that_someone_is_carrying", check_homing_ball_that_someone_is_carrying},
    {"long_name_in_auction_file", check_long_name_in_auction_file},
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
      alarm(60); /* some of these used to hang */
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
