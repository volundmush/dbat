/*
 * What the game does with a script that has a mistake in it, a file that's damaged, and an edit that
 * would lose something. Each check runs in a process of its own, as most of these used to crash or hang.
 *
 * test_bad_data         all of them
 * test_bad_data NAME    one of them, here
 */
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

#include "dbat/iterate.hpp"
#include "dbat/affect.hpp"
#include "dbat/args.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/applies.hpp"
#include "dbat/consts/feats.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/db.hpp"
#include "dbat/act.item.hpp"
#include "dbat/players.hpp"
#include "dbat/act.other.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/genobj.hpp"
#include "dbat/genzon.hpp"
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/lsan_interface.h>
#endif
#include "dbat/game_world.hpp"
#include "dbat/genwld.hpp"
#include "dbat/handler.hpp"
#include "dbat/help.hpp"
#include "dbat/boards.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/shop.hpp"
#include "dbat/shop_utils.hpp"
#include "dbat/guild.hpp"
#include "dbat/guild_utils.hpp"
#include "dbat/rooms.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/mail.hpp"
#include "dbat/house.hpp"
#include "dbat/object_records.hpp"
#include "dbat/fileop.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/objsave.hpp"
#include "dbat/relocate.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/shop.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/local_limits.hpp"

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

#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) {                                                         \
      file_print(stdout, "FAIL {}:{:d}: {}\n", __FILE__, __LINE__, #cond); \
      failures++;                                                          \
    }                                                                      \
  } while (0)

static void print_problem(void *, const char *problem)
{
  file_print(stdout, "AUDIT: {}\n", problem);
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
  file_print(fl, "#399\nScript~\n2 c 100\nrunit~\n{}\n~\n$~\n", script_text);
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
  CHECK(find_trigger(g, 399) != nullptr);
  add_trigger(g, dg_ensure_script(room_script_slot(room_of(g, char_get_in_room(ch)))), read_trigger(g, find_trigger(g, 399)), -1);
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
  char_make_player_specials(copy);
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
    std::string result = eval_expr(g, cases[i].expression, owner_ptr{static_cast<room_data *>(nullptr)}, nullptr, nullptr);

    if (result != cases[i].value) {
      file_print(stdout, "FAIL: {} gave '{}', not '{}'\n", cases[i].expression, result, cases[i].value);
      failures++;
    }
  }
  CHECK(g->dg_eval_depth == 0);
  th_shutdown(g);
}

/* ------ affects ------ */

/* Arithmetic at the ends of the range wraps around, and never takes the game down. */
static void check_arithmetic_at_the_limits(void)
{
  static const struct {
    const char *expression, *value;
  } cases[] = {
      {"9223372036854775807 + 1",                   "-9223372036854775808"},
      {"9223372036854775807 * 2",                   "-2"                  },
      {"99999999999999999999 + 6",                  "-9223372036854775803"}, /* atoll() makes a number too large the largest one */
      {"(0 - 9223372036854775807) - 2",             "9223372036854775807" }, /* subtraction groups from the right */
      {"7 / 0",                                     "0"                   },
      {"((0 - 9223372036854775807) - 1) / (0 - 1)", "-9223372036854775808"}, /* the processor traps on this */
  };
  GameWorld *g = th_boot();

  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); i++) {
    std::string result = eval_expr(g, cases[i].expression, owner_ptr{static_cast<room_data *>(nullptr)}, nullptr, nullptr);

    if (result != cases[i].value) {
      file_print(stdout, "FAIL: {} gave '{}', not '{}'\n", cases[i].expression, result, cases[i].value);
      failures++;
    }
  }
  th_shutdown(g);
}

/* A script variable can't grow past what a script line holds. */
static void check_script_variable_has_a_limit(void)
{
  GameWorld *g = th_boot(); /* for the log */
  dg::var_store vars;
  std::string long_value(MAX_INPUT_LENGTH + 4000, 'x');

  add_var(vars, "big", long_value, 0);
  CHECK(vars.find("big") && vars.find("big")->value.size() == (size_t)MAX_INPUT_LENGTH - 1);
  add_var(vars, "big", "small", 0); /* and setting it again replaces it */
  CHECK(vars.find("BIG") && vars.find("big")->value == "small" && std::next(vars.begin()) == vars.end());
  th_shutdown(g);
}

/*
 * A chained field gives the same on a one-letter variable as on a long one. ("id" is a number, which a further
 * field gets nothing from; a short name used to be written over, leaving no field, and so gave the whole number.)
 */
static void check_chained_field_on_a_short_variable(void)
{
  GameWorld *g          = th_boot();
  struct char_data *mob = read_mobile(g, 300);
  trig_data trig{};

  char_to_room(g, mob, 300);
  add_char_uid_var(&trig, "x", mob, 0);
  add_char_uid_var(&trig, "actor", mob, 0);
  std::string want = var_subst(g, mob, nullptr, &trig, "%actor.id.strlen%");
  CHECK(var_subst(g, mob, nullptr, &trig, "%x.id.strlen%") == want);
  th_shutdown(g);
}

/* A variable left open at the end of a line runs to the end, and no further. */
static void check_variable_left_open_at_the_end(void)
{
  GameWorld *g          = th_boot();
  struct char_data *mob = read_mobile(g, 300);
  trig_data trig{};

  char_to_room(g, mob, 300);
  add_char_uid_var(&trig, "actor", mob, 0);
  CHECK(var_subst(g, mob, nullptr, &trig, "hello %actor.name") ==
        std::string("hello ") + char_get_name(mob));
  CHECK(var_subst(g, mob, nullptr, &trig, "%") == "");
  th_shutdown(g);
}

/* A wait too long to count is as good as forever, rather than overflowing into no wait at all. */
static void check_wait_too_long_to_count(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("wait 9223372036854775807 s\n%echo% The wait is over.", &ch);

  CHECK(!saw(ch, "The wait is over."));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* A wait until no time at all is an error in the script, not a time made up. */
static void check_wait_until_no_time(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("wait until noon\n%echo% The wait is over.", &ch);

  CHECK(!saw(ch, "The wait is over."));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* A message ending in the escape character ends there. */
static void check_message_ending_in_a_backslash(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("%send% %actor% Hello there\\", &ch);

  CHECK(saw(ch, "Hello there\r\n") || saw(ch, "Hello there\n"));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* The text fields (strlen and the rest) are a variable's: self has none of its own. */
static void check_text_field_of_self(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("%echo% [%self.strlen%] [%self.vnum%]", &ch);

  CHECK(saw(ch, "[] ["));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

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

  for (const struct affected_type &af : char_affects(copy)) {
    CHECK(af.specific == 0);
    count++;
  }
  CHECK(count == 2);
  free_char(g, copy);
  th_shutdown(g);
}

/* A player file that ends partway through its description: the old reader shut the game down. */
static void check_player_file_cut_short_in_its_description(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE), *copy;
  char path[256];

  save_char(g, ch);
  save_path("plrfiles", "plr", "tester", path, sizeof(path));
  append_file(path, "Desc:\nA tall fighter,\nwith no end to");

  copy = new char_data();
  clear_char(copy);
  char_make_player_specials(copy);
  CHECK(load_char(g, "tester", copy) == -1); /* refused, and the game goes on */
  free_char(g, copy);
  th_shutdown(g);
}

/* ------ F8: rent, houses and mail ------ */

static bool boot_is_refused(void (*prepare)(void), const char *reason);

static std::string read_whole(const char *path)
{
  FILE *fl = fopen(path, "rb");
  std::string text;
  char buf[4096];
  size_t n;

  if (!fl)
    return text;
  while ((n = fread(buf, 1, sizeof(buf), fl)) > 0)
    text.append(buf, n);
  fclose(fl);
  return text;
}

static void put_le(std::string &bytes, size_t at, int64_t v, size_t n)
{
  for (size_t i = 0; i < n; i++)
    bytes[at + i] = static_cast<char>((static_cast<uint64_t>(v) >> (8 * i)) & 0xff);
}

/* The old binary mail file: a letter in a header block and one data block, and a deleted block between them */
static void old_binary_mail(void)
{
  std::string bytes(3 * 256, '\0');
  put_le(bytes, 0, -1, 8);    /* a header block */
  put_le(bytes, 8, 512, 8);   /* its text goes on at block 2 */
  put_le(bytes, 16, 7, 8);    /* from */
  put_le(bytes, 24, 9, 8);    /* to */
  put_le(bytes, 32, 1234, 8); /* when */
  std::string start(215, 'a');
  bytes.replace(40, start.size(), start);
  put_le(bytes, 256, -3, 8); /* a deleted block */
  put_le(bytes, 512, -2, 8); /* the last block of the letter */
  bytes.replace(520, 5, "tail!");
  FILE *fl = fopen(MAIL_FILE, "wb");
  if (!fl)
    abort();
  fwrite(bytes.data(), 1, bytes.size(), fl);
  fclose(fl);
}

static void check_old_binary_mail_is_converted(void)
{
  th_before_boot(old_binary_mail);
  GameWorld *g = th_boot();

  CHECK(g->mail.size() == 1);
  CHECK(g->mail[0].to == 9 && g->mail[0].from == 7 && g->mail[0].time == 1234);
  CHECK(g->mail[0].text == std::string(215, 'a') + "tail!");
  CHECK(read_whole(MAIL_FILE).starts_with("# dbat plrmail 2\n"));
  CHECK(read_whole(MAIL_FILE ".bak").size() == 3 * 256); /* the old file, kept */

  /* read again, it is the same, and saving it again changes nothing */
  std::string text = read_whole(MAIL_FILE);
  g->mail.clear();
  CHECK(scan_file(g));
  CHECK(g->mail.size() == 1 && g->mail[0].text == std::string(215, 'a') + "tail!");
  CHECK(save_mail_file(g));
  CHECK(read_whole(MAIL_FILE) == text);

  /* a letter with "~" and line ends in it comes back as it was */
  char message[] = "first line~\r\nsecond ~ line\r\n";
  store_mail(g, 9, 7, message);
  g->mail.clear();
  CHECK(scan_file(g));
  CHECK(g->mail.size() == 2 && g->mail[1].text == message);
  th_shutdown(g);
}

/* A letter in the old mail file whose header links to a deleted block, as one in the live data does: receiving it
   read past the file and looped on a garbage link. It keeps what could be read. */
static void old_binary_mail_with_a_broken_link(void)
{
  std::string bytes(256, '\0');
  put_le(bytes, 0, -1, 8);  /* a header block */
  put_le(bytes, 8, -3, 8);  /* linking to "deleted" */
  put_le(bytes, 16, 7, 8);
  put_le(bytes, 24, 9, 8);
  bytes.replace(40, 6, "start.");
  FILE *fl = fopen(MAIL_FILE, "wb");
  if (!fl)
    abort();
  fwrite(bytes.data(), 1, bytes.size(), fl);
  fclose(fl);
}

static void check_old_binary_mail_with_a_broken_link(void)
{
  th_before_boot(old_binary_mail_with_a_broken_link);
  GameWorld *g = th_boot();

  CHECK(g->mail.size() == 1 && g->mail[0].text == "start.");
  CHECK(has_mail(g, 9));
  th_shutdown(g);
}

/* An old binary house record, read field by field */
static void old_binary_houses(void)
{
  std::string bytes(200, '\0');
  put_le(bytes, 0, 300, 4);   /* vnum */
  put_le(bytes, 4, 301, 4);   /* atrium */
  put_le(bytes, 8, -1, 2);    /* exit */
  put_le(bytes, 16, 1000, 8); /* built */
  put_le(bytes, 24, 3, 4);    /* mode */
  put_le(bytes, 32, 42, 8);   /* owner */
  put_le(bytes, 40, 99, 4);   /* guests: far more than there are room for */
  put_le(bytes, 48, 5, 8);    /* the first guest */
  put_le(bytes, 128, 2000, 8);
  put_le(bytes, 136, 6, 8); /* bits */
  put_le(bytes, 144, 77, 8); /* built by */
  put_le(bytes, 192, 8, 8);  /* spare7 */
  FILE *fl = fopen(HCONTROL_FILE, "wb");
  if (!fl)
    abort();
  fwrite(bytes.data(), 1, bytes.size(), fl);
  fclose(fl);
}

static void check_old_binary_houses_are_read(void)
{
  th_before_boot(old_binary_houses);
  GameWorld *g = th_boot(); /* the owner doesn't exist, so the boot drops the house, but the file reads */
  /* the boot saved the file as text, with no houses, keeping the old one */
  CHECK(read_whole(HCONTROL_FILE) == "# dbat hcontrol 2\n");
  CHECK(read_whole(HCONTROL_FILE ".bak").size() == 200);

  old_binary_houses();
  io::loaded<house_control_file> file = read_house_control();
  CHECK(file && file->converted && file->houses.size() == 1);
  const house_control_rec &h = file->houses[0];
  CHECK(h.vnum == 300 && h.atrium == 301 && h.exit_num == -1 && h.built_on == 1000 && h.mode == 3);
  CHECK(h.owner == 42 && h.num_of_guests == MAX_GUESTS && h.guests[0] == 5 && h.last_payment == 2000);
  CHECK(house_flags_number(h.bitvector) == 6 && h.builtby == 77 && h.spare7 == 8);

  /* written as text and read again, it is the same */
  g->house_control[0] = h;
  g->num_of_houses    = 1;
  House_save_control(g);
  io::loaded<house_control_file> again = read_house_control();
  CHECK(again && !again->converted && again->houses.size() == 1);
  CHECK(again && memcmp(&again->houses[0], &h, sizeof(h)) == 0);
  g->num_of_houses = 0;
  th_shutdown(g);
}

static const char *house_file_text;
static void write_house_file(void)
{
  write_file(HCONTROL_FILE, house_file_text);
}

static void check_hcontrol_that_cant_be_read_refuses_the_boot(void)
{
  static const struct {
    const char *text, *reason;
  } cases[] = {
      {"not a house file at all",  "SYSERR: etc/hcontrol:1: expected '# dbat hcontrol 2' to begin the file"},
      {"# dbat hcontrol 1\n",      "SYSERR: etc/hcontrol:1: expected '# dbat hcontrol 2' to begin the file"},
      {"# dbat hcontrol 2\n#300\nColour: red\n", "SYSERR: etc/hcontrol:3: house #300: 'Colour: red' isn't one of its fields"},
      {"# dbat hcontrol 2\nOwner: 1\n", "SYSERR: etc/hcontrol:2: expected '#<vnum>' to begin a house, got 'Owner: 1'"},
      {"# dbat hcontrol 2\n#300\nGuests: 1 2 3 4 5 6 7 8 9 10 11\n", "SYSERR: etc/hcontrol:3: house #300: can't read its Guests"},
  };
  for (const auto &c : cases) {
    house_file_text = c.text;
    if (!boot_is_refused(write_house_file, c.reason)) {
      file_print(stdout, "not refused as expected: '{}', for '{}'\n", c.text, c.reason);
      CHECK(false);
    }
  }

  house_file_text = "# dbat hcontrol 2\n"; /* no houses is fine */
  th_before_boot(write_house_file);
  GameWorld *g = th_boot();
  CHECK(g->num_of_houses == 0);
  th_shutdown(g);
}

/* A house file's object records, read into a room: a spellbook with more pages than it holds, and an object with
   a wear position, which the old reader read before its rows of containers for */
static void check_house_records_past_their_limits(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  room_vnum room       = char_get_in_room(ch);
  std::string text     = "#350\n0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\nXAP\n~\n~\n.~\n~\n2 0 0 0 0 0 0 0\n";

  for (int i = 0; i < SPELLBOOK_SIZE + 5; i++)
    text += "S\n10 3\n";
  text += "#350\n3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\nXAP\n~\n~\n.~\n~\n2 0 0 0 0 0 0 0\n$~\n";

  io::text_reader in("house/test.house", text);
  io::line_cursor lines(in);
  lines.next();
  bag_rows rows(g, nullptr, room);
  int count = 0;
  io::loaded<void> read = read_object_records(g, lines, record_kind::house, [&](obj_data *obj, int locate) {
    obj_to_room(g, obj, room);
    rows.place(obj, locate);
    count++;
  });
  CHECK(read && count == 2);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* A rent file cut short in an object's strings: the old reader shut the game down */
static void check_rent_file_cut_short_in_a_string(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  char path[256];

  save_path("plrobjs", "new", "tester", path, sizeof(path));
  write_file(path, "1\n#350\n0\nXAP\nkeywords~\na short descr");
  CHECK(Crash_load(g, ch) == 0); /* the file is refused, and the game goes on */
  th_shutdown(g);
}

/* A sense memory of more than 500 ids: the old writer read them into a list of 500, and ran past its end */
static void check_long_sense_memory(void)
{
  GameWorld *g                    = th_boot();
  struct char_data *ch            = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *vict          = th_player(g, "Victim", ADMLVL_NONE);
  std::optional<std::string> path = player_path(SENSE_FILE, "Tester");
  std::string ids;

  for (int i = 1; i <= 600; i++)
    ids += str_format("{:d}\n", 100000 + i);
  io::ensure_directory(std::filesystem::path(*path).parent_path());
  write_file(path->c_str(), ids.c_str());
  sense_memory_write(g, ch, vict);
  CHECK(read_whole(path->c_str()) == ids + str_format("{:d}\n", char_get_id(vict)));
  CHECK(read_sense_memory(g, ch, vict) == 1);
  th_shutdown(g);
}

/* An account listed twice at the bottom of the toplist's last section, moving up: the old list of places was read
   one past its end */
static void check_toplist_name_listed_twice_at_the_bottom(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  std::string user(char_get_user(ch));
  const char *rpp[5]   = {"Aaa", user.c_str(), "Ccc", "Bbb", user.c_str()}; /* the last section, places 20 to 24 */

  CHECK(!user.empty() && char_get_desc(ch) != nullptr);
  for (int x = 0; x < 25; x++) {
    g->topname[x]  = x < 20 ? "Nobody" : rpp[x - 20];
    g->toppoint[x] = x < 20 ? 2000000000000LL : 100 - x;
  }
  g->TOPLOADED = true;
  char_set_trp(ch, 150);
  topWrite(g, ch);
  CHECK(str_eq_nocase(g->topname[20], user) && g->toppoint[20] == 150);
  CHECK(g->topname[21] == "Aaa" && g->topname[24].empty() && g->toppoint[24] == 0);
  th_shutdown(g);
}

static void check_infravision_comes_off_an_android(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player_as(g, "Tester", ADMLVL_NONE, RACE_ANDROID, 0);
  struct affected_type af = {.type = SPELL_SLEEP, .duration = 5, .modifier = 7, .location = APPLY_ACCURACY, .bitvector = aff_flag::infravision};
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
  CHECK(char_get_rdisplay(copy) == "stands here, waving");
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
  CHECK(!char_get_inventory(ch).empty());
  while (char_get_inventory(ch))
    extract_obj(g, first_obj(g, char_get_inventory(ch)));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_saved_object_with_a_negative_unique_id(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  char path[256];

  /* -1 means none yet, but no real unique id is negative, and a pool can't be found from one that is */
  save_path("plrobjs", "new", "tester", path, sizeof(path));
  write_file(path, "1\n#350\n0\nXAP\n~\n~\n.~\n~\n2 0 0 0 0 0 0 0\nU\n-38995189\n$~\n");
  Crash_load(g, ch);
  struct obj_data *obj = first_obj(g, char_get_inventory(ch));
  CHECK(obj && obj_get_unique_id(obj) >= 0);
  while (char_get_inventory(ch))
    extract_obj(g, first_obj(g, char_get_inventory(ch)));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* ------ parsing ------ */

/* The old char * argument parsers, as they were before args.hpp replaced them: copied here to compare with. */
static void old_skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}

/* one of the fill words, which one_argument() passed over; a word starting with '!' never was one */
static bool old_fill_word(const char *word)
{
  static const char *const fill[] = {"in", "into", "from", "with", "the", "on", "at", "to"};

  if (*word == '!')
    return false;
  for (const char *f : fill)
    if (!strcmp(word, f))
      return true;
  return false;
}

static char *old_one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    old_skip_spaces(&argument);
    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = tolower(*argument);
      argument++;
    }
    *first_arg = '\0';
  } while (old_fill_word(begin));
  return argument;
}

static char *old_one_word(char *argument, char *first_arg)
{
  old_skip_spaces(&argument);
  if (*argument == '\"') {
    argument++;
    while (*argument && *argument != '\"') {
      *(first_arg++) = tolower(*argument);
      argument++;
    }
    if (*argument)
      argument++;
  } else {
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = tolower(*argument);
      argument++;
    }
  }
  *first_arg = '\0';
  return argument;
}

static char *old_any_one_arg(char *argument, char *first_arg)
{
  old_skip_spaces(&argument);
  while (*argument && !isspace(*argument)) {
    *(first_arg++) = tolower(*argument);
    argument++;
  }
  *first_arg = '\0';
  return argument;
}

static char *old_two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return old_one_argument(old_one_argument(argument, first_arg), second_arg);
}

static void old_half_chop(char *string, char *arg1, char *arg2)
{
  char *temp = old_any_one_arg(string, arg1);

  old_skip_spaces(&temp);
  if (arg2 != temp)
    memmove(arg2, temp, strlen(temp) + 1);
}

/* The auction list names a lot's owner with a capital, but leaves the player table's lowercase name alone: it used
   to capitalise the name in the table itself */
static void check_auction_list_leaves_player_names_alone(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  CHECK(g->top_of_p_table >= 0);
  const player_index_element &owner = g->player_table[0];
  std::string before                = owner.name;

  equip_char(g, ch, read_object(g, 70), WEAR_EYE); /* any scouter will do */
  struct obj_data *lot = read_object(g, 70);
  obj_to_room(g, lot, 80); /* the auction house */
  obj_set_aucter(lot, owner.id);
  obj_set_curBidder(lot, -1);
  obj_set_bid(lot, 100);
  obj_set_aucTime(lot, game_now(g));
  do_bid(g, ch, "list", 0, 0);

  CHECK(std::string_view(owner.name) == before);
  std::string capital = before;
  CHECK(std::string_view(th_output(ch)).contains(CAP(capital)));
  th_shutdown(g);
}

/* io::scan_range() reads "<low>-<high>" as sscanf("%d-%d") does: the count, and each number it read */
static void check_scan_range(void)
{
  for (const char *text : {"", " ", "5", "5-", "5-9", " 5 - 9", "5 -9", "5- 9", "-5--9", "+5-+9", "-", "5x9", "x", "5-x",
                           "99999999999-1", "1-99999999999", "7-8-9", "\t3-\n4"}) {
    int low = 111, high = 222, old_low = 111, old_high = 222;
    int read     = io::scan_range(text, low, high);
    int expected = sscanf(text, "%d-%d", &old_low, &old_high);
    CHECK(read == expected);
    CHECK(low == old_low);
    CHECK(high == old_high);
  }
}

/* flag_set's names and words, for every family's table, against the readers it replaced and the raw words */

/* The old readers (from flags.cpp, which the game no longer has), exactly as they were, for flag_set to be held to */
static bool old_bit_set(std::span<const bitvector_t> words, std::size_t flag) { return (words[flag / 32] & (1u << (flag % 32))) != 0; }

/* the names in a finished list, the space after the last taken off; an empty list is "None " */
static std::string old_flag_list(std::string out)
{
  if (out.empty())
    return "None ";
  out.pop_back();
  return out;
}

/* each set bit's name, a bit past the end of names UNDEFINED */
static std::string old_sprintbit(bitvector_t bits, std::span<const std::string_view> names)
{
  std::string out;
  for (std::size_t nr = 0; bits; bits >>= 1, nr++)
    if (bits & 1) {
      out += nr < names.size() ? names[nr] : "UNDEFINED";
      out += ' ';
    }
  return old_flag_list(std::move(out));
}

/* the set flags' names, read up to the end of names, an empty name left out */
static std::string old_sprintbitarray(std::span<const bitvector_t> words, std::span<const std::string_view> names)
{
  std::string out;
  for (std::size_t flag = 0; flag < words.size() * 32 && flag < names.size(); flag++)
    if (old_bit_set(words, flag) && !names[flag].empty()) {
      out += names[flag];
      out += ' ';
    }
  return old_flag_list(std::move(out));
}

/* the place of the name, exactly as the list has it, read up to an empty name; -1 if none */
static int old_flag_by_name(std::span<const std::string_view> list, std::string_view name)
{
  for (std::size_t i = 0; i < list.size() && !list[i].empty(); i++)
    if (list[i] == name)
      return static_cast<int>(i);
  return -1;
}

/* A flag_set is laid out as the four words it replaced, so the structures holding one keep their size */
static_assert(sizeof(flag_set<zone_flag>) == sizeof(bitvector_t[FLAG_WORDS]) && alignof(flag_set<zone_flag>) == alignof(bitvector_t));

/* A list of flags, and adding and taking away sets, checked by the compiler */
static_assert([] {
  constexpr flag_set<zone_flag> some{zone_flag::closed, zone_flag::quest};
  flag_set<zone_flag> set{zone_flag::quest, zone_flag::dballs};
  set |= some;
  bool added = set.test(zone_flag::closed) && set.test(zone_flag::quest) && set.test(zone_flag::dballs) && !set.test(zone_flag::noimmort);
  set -= some;
  return added && set == flag_set<zone_flag>{zone_flag::dballs} && some.word(0) == 0b101;
}());

template <class E> static int flag_set_mismatches(const name_table &table)
{
  int mismatches   = 0;
  uint64_t state   = 88172645463325252ULL;
  auto next_word   = [&]() { state ^= state << 13; state ^= state >> 7; state ^= state << 17; return static_cast<bitvector_t>(state); };
  for (int round = 0; round < 2000; round++) {
    std::array<bitvector_t, FLAG_WORDS> words{};
    for (bitvector_t &w : words)
      w = round % 3 == 0 ? 0 : round % 3 == 1 ? next_word() & next_word() : next_word(); /* none, sparse, dense */
    flag_set<E> set = flag_set<E>::from_words(words);
    mismatches += flag_names(set) != old_sprintbitarray(words, table);
    mismatches += set.words() != words;
    for (std::size_t i = 0; i < flag_count<E>; i++)
      mismatches += set.test(static_cast<E>(i)) != old_bit_set(words, i);
    for (E flag : set.flags())
      mismatches += !old_bit_set(words, static_cast<std::size_t>(flag));
  }
  return mismatches;
}

/* Mob flags are an NPC's and player flags a player's: neither kind reads or takes the other's, save mob_flag::isnpc on a
   player, which lands on the player flag of its old shared bit */
static void check_act_split(void)
{
  auto *ch = new char_data();
  char_mob_set(ch, mob_flag::sentinel, true);
  char_plr_set(ch, player_flag::thief, true); /* an NPC has no player flags */
  CHECK(char_mob_flagged(ch, mob_flag::sentinel));
  CHECK(!char_plr_flagged(ch, player_flag::thief));
  CHECK(!char_plr_toggle(ch, player_flag::thief));
  char_make_player_specials(ch);
  CHECK(!char_mob_flagged(ch, mob_flag::sentinel)); /* a player reads no mob flags */
  char_mob_set(ch, mob_flag::aware, true);
  CHECK(char_get_mob_flags(ch).test(mob_flag::sentinel) && !char_get_mob_flags(ch).test(mob_flag::aware));
  char_plr_set(ch, player_flag::thief, true);
  CHECK(char_plr_flagged(ch, player_flag::thief));
  CHECK(!char_plr_toggle(ch, player_flag::thief));
  char_mob_set(ch, mob_flag::isnpc, true);
  CHECK(char_plr_flagged(ch, player_flag::dontset));
  CHECK(!char_mob_toggle(ch, mob_flag::isnpc) && !char_plr_flagged(ch, player_flag::dontset));
  char_drop_player_specials(ch);
  delete ch;
}

/* flag_mask_names() against the old sprintbit(), for a single-word family: word 0 only, UNDEFINED past the names */
template <class E> static int flag_mask_mismatches(const name_table &table)
{
  int mismatches = 0;
  uint64_t state = 88172645463325252ULL;
  for (int round = 0; round < 2000; round++) {
    state ^= state << 13; state ^= state >> 7; state ^= state << 17;
    bitvector_t word = round % 3 == 0 ? 0 : round % 3 == 1 ? static_cast<bitvector_t>(state) & 0x3f : static_cast<bitvector_t>(state);
    flag_set<E> set;
    set.set_word(0, word);
    mismatches += flag_mask_names(set) != old_sprintbit(word, table);
  }
  return mismatches;
}

/* Flags or'ed together are a set of them, never a flag */
static_assert((exit_flag::isdoor | exit_flag::closed | exit_flag::locked).word(0) == 0b111 && (container_flag::closed | container_flag::locked).word(0) == 0b1100);
static_assert(!std::is_convertible_v<decltype(exit_flag::isdoor | exit_flag::closed), exit_flag>);
/* act()'s whom and how: an option goes with someone to tell, never alone */
static_assert((act_to::actor | act_option::even_asleep).to == act_to::actor && (act_to::actor | act_option::even_asleep).even_asleep && !(act_to::actor | act_option::even_asleep).sneak_resist);
static_assert((act_to::room | act_option::sneak_resist).sneak_resist && act_target(act_to::vict).to == act_to::vict && !act_target(act_to::vict).even_asleep);
static_assert(!std::is_convertible_v<act_option, act_target>);

static void check_flag_sets(void)
{
  CHECK(flag_set_mismatches<room_flag>(room_bits) == 0);
  CHECK(flag_set_mismatches<zone_flag>(zone_bits) == 0);
  CHECK(flag_set_mismatches<aff_flag>(affected_bits) == 0);
  CHECK(flag_set_mismatches<player_flag>(player_bits) == 0);
  CHECK(flag_set_mismatches<mob_flag>(action_bits) == 0);
  CHECK(flag_set_mismatches<pref_flag>(preference_bits) == 0);
  CHECK(flag_set_mismatches<extra_flag>(extra_bits) == 0);
  CHECK(flag_set_mismatches<wear_flag>(wear_bits) == 0);
  CHECK(flag_set_mismatches<trade_flag>(trade_letters) == 0);
  CHECK(flag_mask_mismatches<exit_flag>(exit_bits) == 0);
  CHECK(flag_mask_mismatches<container_flag>(container_bits) == 0);
  CHECK(flag_mask_mismatches<shop_flag>(shop_bits) == 0);
  CHECK(flag_mask_mismatches<house_flag>(house_bits) == 0);
  CHECK(flag_mask_mismatches<mob_trig>(trig_types) == 0);
  CHECK(flag_mask_mismatches<obj_trig>(otrig_types) == 0);
  CHECK(flag_mask_mismatches<wld_trig>(wtrig_types) == 0);
  /* a trigger's types word is named by the table of what it is for, any other number as a mob's */
  for (bitvector_t word : {0u, 0x80001u, 0xFFFFFFFFu, 0x40000u}) {
    CHECK(trig_type_names(OBJ_TRIGGER, word) == old_sprintbit(word, otrig_types));
    CHECK(trig_type_names(WLD_TRIGGER, word) == old_sprintbit(word, wtrig_types));
    CHECK(trig_type_names(MOB_TRIGGER, word) == old_sprintbit(word, trig_types));
    CHECK(trig_type_names(7, word) == old_sprintbit(word, trig_types));
  }
  /* a house's flags are a long in the files: every value comes back, negative or past 32 bits */
  for (long n : {0L, 6L, -1L, 0x7fffffffL, 0x80000000L, 0x123456789abL, -0x123456789abL})
    CHECK(house_flags_number(house_flags_from_number(n)) == n);

  /* by number and by name, as the old get_flag_by_name() found them */
  CHECK(flag_from_index<room_flag>(-1) == std::nullopt);
  CHECK(flag_from_index<room_flag>(flag_count<room_flag>) == std::nullopt);
  CHECK(flag_from_index<room_flag>(3) == static_cast<room_flag>(3));
  for (std::size_t i = 0; i < room_bits.size(); i++) {
    std::optional<room_flag> named = flag_by_name<room_flag>(room_bits[i]);
    int old                        = old_flag_by_name(room_bits, room_bits[i]);
    CHECK(named ? static_cast<int>(*named) == old : old == -1);
  }
  CHECK(flag_by_name<room_flag>("NO_SUCH_FLAG") == std::nullopt);
}

/* asctime_text() writes a time as asctime() did, less its newline: the day padded with a space, the rest with zeros */
static void check_asctime_text(void)
{
  for (time_t when : std::initializer_list<time_t>{0, 86399, 951782400, 1700000000, 1798761599, 4102444800}) {
    std::string expected = asctime(localtime(&when));
    expected.pop_back();
    CHECK(asctime_text(when) == expected);
  }
}

/* add_commas() groups the digits in threes, even for the longest numbers, which overflowed its buffer */
static void check_add_commas(void)
{
  CHECK(std::string_view(add_commas(0)) == "0");
  CHECK(std::string_view(add_commas(999)) == "999");
  CHECK(std::string_view(add_commas(1000)) == "1,000");
  CHECK(std::string_view(add_commas(-1000)) == "-1,000");
  CHECK(std::string_view(add_commas(-100)) == "-100");
  CHECK(std::string_view(add_commas(123456789)) == "123,456,789");
  CHECK(std::string_view(add_commas(INT64_MAX)) == "9,223,372,036,854,775,807");
  CHECK(std::string_view(add_commas(INT64_MIN)) == "-9,223,372,036,854,775,808");
}

/* The words args.hpp takes off a line are the ones the old char * functions did, and so is what is left. */
static void check_args_match_the_old_parsers(void)
{
  static const char *const edges[] = {
      "",
      " ",
      "\t",
      "sword",
      "  Sword  shield ",
      "the sword",
      "the in at sword",
      "the",
      "the ",
      "into the bag",
      "\"two words\" more",
      "\"unclosed words",
      "\"\"",
      "\" \"",
      "\t\tTAB\tthen",
      "\xc8\xe9 accents",
      "a\nb",
      "on",
      "To The At",
      "2.sword",
      "all.coin from bag",
      "x \"y z\" w",
  };
  static const char *const tokens[] = {" ", "  ", "\t", "\"", "the", "In", "at", "x", "Sword", "\xc8", "\n", "2.", "on", "a"};
  std::vector<std::string> inputs(edges, edges + sizeof(edges) / sizeof(*edges));
  unsigned seed  = 12345;
  int mismatches = 0;

  for (int i = 0; i < 3000; i++) {
    std::string text;
    for (int n = (seed = seed * 1103515245 + 12345) % 9; n > 0; n--)
      text += tokens[(seed = seed * 1103515245 + 12345) / 65536 % (sizeof(tokens) / sizeof(*tokens))];
    inputs.push_back(text);
  }

  for (const std::string &input : inputs) {
    char buf[MAX_INPUT_LENGTH], w1[MAX_INPUT_LENGTH], w2[MAX_INPUT_LENGTH];
    std::string_view args;
    const char *rest;

    snprintf(buf, sizeof(buf), "%s", input.c_str());
    rest = old_one_argument(buf, w1);
    args = input;
    mismatches += take_arg(args) != w1 || args != rest;

    snprintf(buf, sizeof(buf), "%s", input.c_str());
    rest = old_any_one_arg(buf, w1);
    args = input;
    mismatches += take_word(args) != w1 || args != rest;

    snprintf(buf, sizeof(buf), "%s", input.c_str());
    rest = old_one_word(buf, w1);
    args = input;
    mismatches += take_quoted(args) != w1 || args != rest;

    snprintf(buf, sizeof(buf), "%s", input.c_str());
    rest = old_two_arguments(buf, w1, w2);
    args = input;
    mismatches += take_arg(args) != w1 || take_arg(args) != w2 || args != rest;

    snprintf(buf, sizeof(buf), "%s", input.c_str());
    old_half_chop(buf, w1, w2);
    args = input;
    mismatches += take_word(args) != w1 || skip_spaces(args) != w2;
  }
  CHECK(mismatches == 0);
}

/* is_abbrev() as it was on C strings: copied here to compare with. */
static bool old_is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return false;
  for (; *arg1 && *arg2; arg1++, arg2++)
    if (tolower(*arg1) != tolower(*arg2))
      return false;
  return !*arg1;
}

/* The name matching the lookups do, as it was before it took std::string_views: copied here to compare with. */
static bool old_is_name(const char *str, const char *namelist)
{
  const char *curname, *curstr;

  if (!*str || !*namelist || !str || !namelist)
    return false;

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if (!*curstr && !isalpha(*curname))
        return true;
      if (!*curname)
        return false;
      if (!*curstr || *curname == ' ')
        break;
      if (tolower(*curstr) != tolower(*curname))
        break;
    }
    for (; isalpha(*curname); curname++);
    if (!*curname)
      return false;
    curname++;
  }
}

static bool old_isname(const char *str, const char *namelist)
{
  char *newlist, *curtok;
  static char newlistbuf[MAX_STRING_LENGTH];

  if (!str || !*str || !namelist || !*namelist)
    return false;
  if (!strcasecmp(str, namelist))
    return true;

  strlcpy(newlistbuf, namelist, sizeof(newlistbuf));
  newlist = newlistbuf;
  for (curtok = strsep(&newlist, " \t"); curtok; curtok = strsep(&newlist, " \t")) {
    if (curtok && is_abbrev(str, curtok)) {
      if (isdigit(*str) && (atoi(str) != atoi(curtok)))
        return false;
      return true;
    }
  }
  return false;
}

/* is_number(), CAP() and search_block() as they were on C strings: copied here to compare with */
static bool old_is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return false;
  return true;
}

static char *old_CAP(char *txt)
{
  static constexpr std::string_view colour = "nbBcCgGmMrRyYwWkK0234567oeul";
  int i;

  for (i = 0; txt[i] != '\0' && txt[i] == '@' && txt[i + 1] && colour.contains(txt[i + 1]); i += 2);
  txt[i] = toupper(txt[i]);
  return txt;
}

static int old_search_block(char *arg, const char *const *list, int exact)
{
  int i, l;

  if (*arg == '!')
    return -1;
  for (l = 0; arg[l]; l++)
    arg[l] = tolower(arg[l]);
  if (exact) {
    for (i = 0; *list[i] != '\n'; i++)
      if (!strcmp(arg, list[i]))
        return i;
  } else {
    if (!l)
      l = 1;
    for (i = 0; *list[i] != '\n'; i++)
      if (!strncmp(arg, list[i], l))
        return i;
  }
  return -1;
}

static int old_get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';
  if ((ppos = strchr(*name, '.')) != nullptr) {
    *ppos++ = '\0';
    strlcpy(number, *name, sizeof(number));
    memmove(*name, ppos, strlen(ppos) + 1);
    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
        return (0);
    return (atoi(number));
  }
  return (1);
}

static find_dots old_find_all_dots(char *arg)
{
  if (!strcmp(arg, "all"))
    return (find_dots::all);
  else if (!strncmp(arg, "all.", 4)) {
    memmove(arg, arg + 4, strlen(arg + 4) + 1);
    return (find_dots::alldot);
  } else
    return (find_dots::indiv);
}

/* isname(), is_name(), get_number() and the rest find what they did as char * functions, and leave the
 * caller's name as they did. */
static void check_name_matching_matches_the_old_code(void)
{
  static const char *const tokens[] = {"sword", "Sword", "sw", "2.", "0.", "10", "100", "1000", " ", "\t", "@r",
                                       "@", "\xc8", "a", "-", "+", "0", "99999999999999999999", ".", "all", "all.",
                                       "!", "\n", "x.y", "ME", "-12"};
  static const char *const words[]  = {"finger", "neck", "body", "head", "\n"};
  static constexpr std::string_view word_views[] = {"finger", "neck", "body", "head"}; /* the same, without the "\n" */
  unsigned seed                     = 4242;
  int mismatches                    = 0;
  auto next                         = [&] { return (seed = seed * 1103515245 + 12345) / 65536; };
  auto text                         = [&] {
    std::string t;
    for (int n = next() % 5; n > 0; n--)
      t += tokens[next() % (sizeof(tokens) / sizeof(*tokens))];
    return t;
  };

  for (int i = 0; i < 20000; i++) {
    std::string a = text(), b = text();
    char old_buf[MAX_INPUT_LENGTH];

    mismatches += isname(a, b) != old_isname(a.c_str(), b.c_str());
    mismatches += is_name(a, b) != old_is_name(a.c_str(), b.c_str());
    mismatches += isname(a, "") || isname("", b) || is_name(a, ""); /* no text names nothing */
    mismatches += is_abbrev(a, b) != old_is_abbrev(a.c_str(), b.c_str());
    mismatches += str_eq_nocase(a, b) != !strcasecmp(a.c_str(), b.c_str());
    mismatches += parse_int(a) != atoi(a.c_str());
    mismatches += is_number(a) != old_is_number(a.c_str());

    std::string name = a;
    snprintf(old_buf, sizeof(old_buf), "%s", a.c_str());
    char *old_p = old_buf;
    mismatches += get_number(name) != old_get_number(&old_p) || name != old_buf;

    name = a;
    snprintf(old_buf, sizeof(old_buf), "%s", a.c_str());
    mismatches += find_all_dots(name) != old_find_all_dots(old_buf) || name != old_buf;

    name = a;
    snprintf(old_buf, sizeof(old_buf), "%s", a.c_str());
    mismatches += CAP(name) != old_CAP(old_buf);

    snprintf(old_buf, sizeof(old_buf), "%s", a.c_str());
    mismatches += search_block(a, word_views, false) != old_search_block(old_buf, words, false);
    snprintf(old_buf, sizeof(old_buf), "%s", a.c_str());
    mismatches += search_block(a, word_views, true) != old_search_block(old_buf, words, true);
  }
  CHECK(mismatches == 0);
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
    obj_to_char(g, read_object(g, 350), ch);
  CHECK(char_get_carry_items(ch) == before + 300);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_mob_memory_gets_forgotten(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *mob = read_mobile(g, 300);
  struct script_memory *mem;

  char_to_room(g, mob, char_get_in_room(ch));
  mem      = new script_memory();
  mem->id  = char_get_id(ch);
  mem->cmd = "say I remember you";
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
  std::string_view upper = juggleRaceName(ch, true), lower = juggleRaceName(ch, false);

  CHECK(upper == "Human");
  CHECK(lower == "human");
  th_shutdown(g);
}

/* Saving a new mob, object or room used to add 1 to the numbers in the zone commands another builder had open in zedit,
 * left over from when prototypes were numbered by their place in a table */
static void check_new_prototypes_leave_other_editors_alone(void)
{
  GameWorld *g            = th_boot();
  struct char_data *zoner = th_player(g, "Zoner", ADMLVL_IMPL), *maker = th_player(g, "Maker", ADMLVL_IMPL);
  std::vector<int> before, after;

  /* room 300's commands in the zone, in order: zedit's copy of them is saved back over them below */
  auto room_commands = [&](std::vector<int> &out) {
    struct zone_data *zone = find_zone_by_thing(g, 300);
    room_vnum at           = NOWHERE;
    for (const reset_com &c : zone->cmd) {
      if (c.command == 'S')
        break;
      if (strchr("MOTV", c.command))
        at = c.arg3;
      else if (strchr("DR", c.command))
        at = c.arg1;
      if (at == 300)
        out.push_back(c.arg1 * 7 + c.arg3);
    }
  };
  room_commands(before);
  th_type(g, zoner, "zedit 300");
  CHECK(char_get_desc(zoner)->olc != nullptr);

  th_type(g, maker, "medit 305"); /* below the mobs the zone loads */
  th_type(g, maker, "2");         /* a change, so that quitting asks to save */
  th_type(g, maker, "newmob");
  th_type(g, maker, "q");
  th_type(g, maker, "y");
  CHECK(mob_proto_of(g, 305) != nullptr);
  th_type(g, maker, "oedit 305"); /* below object 3034, which it loads */
  th_type(g, maker, "1");
  th_type(g, maker, "newobj");
  th_type(g, maker, "q");
  th_type(g, maker, "y");
  CHECK(obj_proto_of(g, 305) != nullptr);
  th_type(g, maker, "redit 306"); /* below rooms 310 and 311, which it names */
  th_type(g, maker, "1");
  th_type(g, maker, "A new room");
  th_type(g, maker, "q");
  th_type(g, maker, "y");
  CHECK(room_of(g, 306) != nullptr);

  th_type(g, zoner, "l"); /* a change, so that zedit saves the room's commands */
  th_type(g, zoner, "30");
  th_type(g, zoner, "q");
  th_type(g, zoner, "y");
  room_commands(after);
  CHECK(!before.empty() && before == after);
  th_shutdown(g);
}

/* oedit works on a copy of an object's spellbook pages: quitting without saving leaves the prototype's as they were
 * (the copy shared them, so changing a page changed the prototype's at once) */
static void check_spellbook_edit_can_be_abandoned(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Bookish", ADMLVL_IMPL);
  int spell            = 0;
  for (int i = 1; i < SKILL_TABLE_SIZE && !spell; i++)
    if (g->spell_info[i].skilltype == skill_kind::spell)
      spell = i;
  CHECK(spell > 0);
  std::string number = std::to_string(spell);

  th_type(g, ch, "oedit 305");
  th_type(g, ch, "1");
  th_type(g, ch, "spellbook");
  th_type(g, ch, "t");
  th_type(g, ch, "1");
  th_type(g, ch, number.c_str());
  th_type(g, ch, "0");
  th_type(g, ch, "q");
  th_type(g, ch, "y");
  CHECK(obj_proto_of(g, 305) && obj_get_sbinfo(obj_proto_of(g, 305)));
  CHECK(obj_proto_of(g, 305) && obj_get_sbinfo(obj_proto_of(g, 305)) && obj_get_sbinfo(obj_proto_of(g, 305))[0].spellname == spell);

  th_type(g, ch, "oedit 305");
  th_type(g, ch, "t");
  th_type(g, ch, "1");
  th_type(g, ch, "0"); /* clears the page */
  th_type(g, ch, "0");
  th_type(g, ch, "q");
  th_type(g, ch, "n");
  CHECK(desc_get_connected(char_get_desc(ch)) == CON_PLAYING);
  CHECK(obj_get_sbinfo(obj_proto_of(g, 305)) && obj_get_sbinfo(obj_proto_of(g, 305))[0].spellname == spell);
  th_shutdown(g);
}

/* The editor works on a copy of the text, written back when it is saved: an edit that is aborted leaves the text as
 * it was, and one that is saved replaces it */
static void check_editor_saves_and_aborts(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Writer", ADMLVL_IMPL);

  th_type(g, ch, "redit 301");
  th_type(g, ch, "2"); /* the description, in the editor */
  th_type(g, ch, "/c");
  th_type(g, ch, "A fresh line.");
  th_type(g, ch, "/i1 An inserted line.");
  th_type(g, ch, "/s");
  th_type(g, ch, "2");
  th_type(g, ch, "This is thrown away.");
  th_type(g, ch, "/a");
  th_type(g, ch, "q");
  th_type(g, ch, "y");
  std::string_view description = room_get_description(g, 301);
  CHECK(description == "An inserted line.\r\nA fresh line.\r\n");
  th_shutdown(g);
}

static void check_news_entry_leaves_the_immortal_list_alone(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Writer", ADMLVL_IMPL);
  char news[4096]      = "";

  th_type(g, ch, "newsedit A title");
  th_type(g, ch, "The entry itself.");
  th_type(g, ch, "/s");
  FILE *fl = fopen(NEWS_FILE, "r");
  CHECK(fl);
  if (fl) {
    news[fread(news, 1, sizeof(news) - 1, fl)] = '\0';
    fclose(fl);
  }
  /* the editor used to start out holding the immlist file, which it borrowed to write in */
  CHECK(strstr(news, "The entry itself.") && !strstr(news, "Fixture immlist"));
  th_shutdown(g);
}

static void check_zone_range_keeps_what_is_in_it(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_IMPL);
  struct zone_data *zone = find_zone(g, 3);
  int bot = zone->bot, top = zone->top;

  th_type(g, ch, "zedit 300");
  th_type(g, ch, "t");
  th_type(g, ch, "300");
  CHECK(saw(ch, "Range not changed"));
  th_type(g, ch, "b");
  th_type(g, ch, "399");
  CHECK(saw(ch, "Range not changed"));
  th_type(g, ch, "q");
  th_type(g, ch, "y");
  CHECK(zone->bot == bot);
  CHECK(zone->top == top);
  CHECK(room_of(g, 301));
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

/* The first keywords leave words behind in the buffer, for an unfinished quote to run on into. */
static void help_keyword_with_an_unfinished_quote(void)
{
  write_file("text/help/help.hlp", "t aaa bbb ccc ddd\nFirst.\n#0\n\"re.\nSecond.\n#0\n$\n");
}

static void check_help_keyword_with_an_unfinished_quote(void)
{
  th_before_boot(help_keyword_with_an_unfinished_quote);
  GameWorld *g = th_boot();

  /* t, aaa, bbb, ccc, ddd, and what follows the quote */
  CHECK(g->help_table.size() == 6);
  for (const help_index_element &el : g->help_table)
    CHECK(el.keywords != "bb" && el.keywords != "a");
  th_shutdown(g);
}

/* A line that starts with a '\0': the old reader took the last character off a line of no length, writing before
   its buffer. */
static void help_line_starting_with_a_nul(void)
{
  static const char text[] = "nul\nbefore\n\0after\nend\n#0\n$\n";
  FILE *fl                 = fopen("text/help/help.hlp", "w");

  if (!fl)
    abort();
  fwrite(text, 1, sizeof(text) - 1, fl);
  fclose(fl);
}

static void check_help_line_starting_with_a_nul(void)
{
  th_before_boot(help_line_starting_with_a_nul);
  GameWorld *g = th_boot();

  CHECK(g->help_table.size() == 1);
  CHECK(g->help_table[0].entry == "nul\r\nbefore\r\n\r\nend\r\n"); /* the line is empty from its '\0' */
  th_shutdown(g);
}

/* An entry with no text was a null in the context help, which showing it would crash on. */
static void empty_context_help(void)
{
  write_file(CONTEXT_HELP_FILE, "#0\n~\n#1\nSome help.~\n");
}

static void check_empty_context_help(void)
{
  th_before_boot(empty_context_help);
  GameWorld *g = th_boot();

  CHECK(g->context_help_list[0] == "No help available (yet)!\r\n");
  CHECK(g->context_help_list[1] == "Some help.");
  th_shutdown(g);
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
  struct obj_data *bag    = read_object(g, CONTAINER_VNUM);
  struct obj_data *inside = read_object(g, 350);

  while (char_get_inventory(ch))
    extract_obj(g, first_obj(g, char_get_inventory(ch)));
  for (int i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i))
      extract_obj(g, char_get_equipment(ch, i));
  obj_to_obj(g, inside, bag);
  equip_char(g, ch, bag, WEAR_HEAD);
  CHECK(char_get_equipment(ch, WEAR_HEAD) == bag);
  Crash_crashsave(g, ch);
  extract_obj(g, char_get_equipment(ch, WEAR_HEAD));

  /* now something else is on that spot, as can happen when a save has two things there */
  equip_char(g, ch, read_object(g, 350), WEAR_HEAD);
  Crash_load(g, ch);

  struct obj_data *back = nullptr;

  for (obj_data *obj : char_inventory(g, ch))
    if (obj_get_vnum(g, obj) == CONTAINER_VNUM)
      back = obj;
  CHECK(back != nullptr);
  CHECK(back && !obj_get_contains(back).empty());
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* "get all.<thing> all.<container>" takes every <thing> from every container (taking them from the first stripped
 * "all." off the name, so from the others it took only one each) */
static void check_get_all_from_all_containers(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Gatherer", ADMLVL_IMPL);

  while (char_get_inventory(ch))
    extract_obj(g, first_obj(g, char_get_inventory(ch)));
  for (int b = 0; b < 2; b++) {
    struct obj_data *bag = read_object(g, CONTAINER_VNUM);
    obj_set_name(bag, "satchel");
    obj_set_value(bag, VAL_CONTAINER_FLAGS, 0); /* open */
    for (int i = 0; i < 2; i++) {
      struct obj_data *pebble = read_object(g, 350);
      obj_set_name(pebble, "pebble");
      obj_to_obj(g, pebble, bag);
    }
    obj_to_char(g, bag, ch);
  }
  th_type(g, ch, "get all.pebble all.satchel");

  int loose = 0, left = 0;
  for (obj_data *obj : char_inventory(g, ch)) {
    if (obj_get_vnum(g, obj) == 350)
      loose++;
    else
      left += (int)obj_get_contains(obj).size();
  }
  CHECK(loose == 4);
  CHECK(left == 0);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* mjunk <name> junks the item by that name and mjunk all.<name> every one (the first read a name after "all." from
 * any name, so "mjunk sword" junked what matched "d", and the second junked only one) */
static void check_mjunk_by_name(void)
{
  GameWorld *g          = th_boot();
  struct char_data *ch  = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *mob = read_mobile(g, 300);
  const char *names[]   = {"sword", "dagger", "pebble", "pebble"};

  CHECK(mob != nullptr);
  char_to_room(g, mob, char_get_in_room(ch));
  for (const char *name : names) {
    struct obj_data *obj = read_object(g, 350);
    obj_set_name(obj, name);
    obj_to_char(g, obj, mob);
  }
  auto count = [&](const char *name) {
    int n = 0;
    for (obj_data *obj : char_inventory(g, mob))
      if (obj_get_name(obj) == name)
        n++;
    return n;
  };

  do_mjunk(g, mob, "sword", 0, 0);
  CHECK(count("sword") == 0);
  CHECK(count("dagger") == 1);
  do_mjunk(g, mob, "all.pebble", 0, 0);
  CHECK(count("pebble") == 0);
  CHECK(count("dagger") == 1);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* A zone echo longer than a typed line (a script line can be, once its variables are filled in) reaches the zone
 * whole (it was formatted into a buffer of MAX_INPUT_LENGTH) */
static void check_long_zone_echo(void)
{
  GameWorld *g          = th_boot();
  struct char_data *ch  = th_player(g, "Listener", ADMLVL_NONE);
  struct char_data *mob = read_mobile(g, 300);

  CHECK(mob != nullptr);
  char_to_room(g, mob, char_get_in_room(ch));
  std::string text(MAX_INPUT_LENGTH + 100, 'x');
  std::string line = std::to_string(char_get_in_room(ch)) + " " + text + "END";
  do_mzoneecho(g, mob, line.data(), 0, 0);
  CHECK(saw(ch, "xxxEND"));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* mload mob <vnum> <room> loads the mob into that room (the room came with the space before it, so it was never
 * a number) */
static void check_mload_mob_into_a_room(void)
{
  GameWorld *g          = th_boot();
  struct char_data *ch  = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *mob = read_mobile(g, 300);

  CHECK(mob != nullptr && room_of(g, 301) != nullptr);
  char_to_room(g, mob, char_get_in_room(ch));
  CHECK(char_get_in_room(ch) != 301);
  auto in_301 = [&]() {
    int n = 0;
    for (char_data *tch : room_people(g, 301))
      if (char_get_mob_vnum(g, tch) == 300)
        n++;
    return n;
  };
  int before = in_301();
  do_mload(g, mob, "mob 300 301", 0, 0);
  CHECK(in_301() == before + 1);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* An emote of many #s (each one becomes "$n") fits in the room's output (it was rewritten in place in a buffer of
 * MAX_INPUT_LENGTH, which doubling overflowed) */
static void check_emote_of_many_hashes(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Emoter", ADMLVL_NONE);

  std::string line = "emote " + std::string(MAX_INPUT_LENGTH - 16, '#');
  th_type(g, ch, line.c_str());
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* A wiznet line goes into each listener's history without leaving a copy behind (each was strdup()ed and kept by no
 * one; under ASan the leak fails the run) */
static void check_wiznet_keeps_no_copies(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Wizard", ADMLVL_IMPL);

  th_type(g, ch, "wiznet hello there");
  CHECK(saw(ch, "hello there"));
  CHECK_AUDIT(g);
  th_shutdown(g);
#ifdef __SANITIZE_ADDRESS__
  CHECK(__lsan_do_recoverable_leak_check() == 0); /* the run ends in _exit(), which LeakSanitizer doesn't see */
#endif
}

/* Deleting an object turns a zone's "put it in a container" commands into comments, as it does those that load it
 * anywhere else, so saving the zone afterwards has no object to look up that isn't there (it checked only the
 * container) */
static void check_deleted_object_leaves_no_put_command(void)
{
  GameWorld *g           = th_boot();
  struct zone_data *zone = find_zone_by_thing(g, 350);

  CHECK(zone != nullptr && obj_proto_of(g, 350) && obj_proto_of(g, CONTAINER_VNUM));
  reset_com put;
  put.command = 'P';
  put.arg1    = 350;
  put.arg2    = 1;
  put.arg3    = CONTAINER_VNUM;
  zone->cmd.insert(zone->cmd.end() - 1, put); /* before the closing 'S' */
  size_t at = zone->cmd.size() - 2;

  CHECK(delete_object(g, 350) == 350);
  CHECK(zone->cmd[at].command == '*');
  CHECK(save_zone(g, zone));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* Sensing someone on the same planet names where they are without leaving a copy of the name behind (it was
 * allocated for each sensing player, and only some callers freed it) */
static void check_sensing_keeps_no_copies(void)
{
  GameWorld *g          = th_boot();
  struct char_data *ch  = th_player(g, "Sensed", 0);
  struct char_data *tch = th_player(g, "Senser", 0);

  room_set_flag(g, 300, room_flag::earth, true);
  room_set_flag(g, 301, room_flag::earth, true);
  char_from_room(g, ch);
  char_to_room(g, ch, 300);
  char_from_room(g, tch);
  char_to_room(g, tch, 301);
  char_set_skills_base(tch, SKILL_SENSE, 50);

  send_to_sense(g, 1, "landing on the planet", ch);
  CHECK(saw(tch, "landing on the planet"));
  CHECK_AUDIT(g);
  th_shutdown(g);
#ifdef __SANITIZE_ADDRESS__
  CHECK(__lsan_do_recoverable_leak_check() == 0); /* the run ends in _exit(), which LeakSanitizer doesn't see */
#endif
}

/* A shop that trades with no one lists its customers as "None", not as the last shop listed did (the list was
 * kept in a static buffer that an empty one never wrote to) */
static void check_shop_with_no_customers(void)
{
  GameWorld *g           = th_boot();
  struct char_data *ch   = th_player(g, "Wizard", ADMLVL_IMPL);
  struct shop_data *shop = find_first(g->shops.all(), [](shop_data *) { return true; });

  CHECK(shop != nullptr);
  th_type(g, ch, "game shops 1");
  CHECK(saw(ch, "Customers:  , "));
  th_take_output(g, ch);

  for (int i = 0; i < NUM_TRADERS; i++)
    shop_set_with_who(shop, *flag_from_index<trade_flag>(i), true);
  th_type(g, ch, "game shops 1");
  CHECK(saw(ch, "Customers:  None"));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* The same for a guild that trains no one: its list is empty, not the last guild's */
static void check_guild_with_no_customers(void)
{
  GameWorld *g            = th_boot();
  struct char_data *ch    = th_player(g, "Wizard", ADMLVL_IMPL);
  struct guild_data *gild = find_first(g->guilds.all(), [](guild_data *) { return true; });

  CHECK(gild != nullptr);
  std::string show = str_format("game guilds {:d}", guild_get_vnum(gild));
  th_type(g, ch, show.c_str());
  CHECK(saw(ch, "Whom will train: , "));
  th_take_output(g, ch);

  for (int i = 0; i < NUM_TRADERS; i++)
    guild_set_with_who(gild, *flag_from_index<trade_flag>(i), true);
  th_type(g, ch, show.c_str());
  CHECK(saw(ch, "Whom will train: \r\n"));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* A guildmaster's own special procedure is given what was typed, where it was given a buffer that was never filled */
static std::string guildmaster_heard;
static SPECIAL(record_argument)
{
  guildmaster_heard = argument;
  return false;
}

static void check_guildmaster_special_hears_the_command(void)
{
  GameWorld *g            = th_boot();
  struct char_data *ch    = th_player(g, "Pupil", ADMLVL_NONE);
  struct guild_data *gild = find_first(g->guilds.all(), [&](guild_data *gd) { return mob_proto_of(g, guild_get_gm(gd)) != nullptr; });

  CHECK(gild != nullptr);
  struct char_data *gm = read_mobile(g, guild_get_gm(gild));
  char_to_room(g, gm, char_get_in_room(ch));
  mob_index_of(g, guild_get_gm(gild))->func = guild;
  guild_set_func(gild, record_argument);
  guildmaster_heard = "nothing yet";
  th_type(g, ch, "say hello there");
  CHECK(guildmaster_heard == " hello there"); /* the rest of the line, as every special is given it */
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* hindex lists every matching help entry however many there are (it added up what snprintf() wanted to write, so
 * a long enough list went on writing past the end of its buffer) */
static void check_long_help_index(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Wizard", ADMLVL_IMPL);

  for (int i = 0; i < 4000; i++) {
    help_index_element entry;
    entry.keywords = str_format("ZZZZ{:04d}", i);
    entry.entry    = "text\r\n";
    g->help_table.push_back(entry);
  }
  th_type(g, ch, "hindex zzzz");
  CHECK(saw(ch, "ZZZZ3999"));
  CHECK(saw(ch, "4000 Help files in index."));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* Listing a rent file with a unique item in it reads past the item's strings without keeping them */
static void check_rent_listing_keeps_no_copies(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Wizard", ADMLVL_IMPL);
  char path[256];

  save_path("plrobjs", "new", "tester", path, sizeof(path));
  write_file(path, "1 0 0 0 0 1\n#-1\n0\nXAP\nthing~\na unique thing~\nA thing lies here.~\nIt does things.~\n0 0 0 0 7\n$~\n");
  std::string tester = "tester";
  Crash_listrent(g, ch, tester.data());
  CHECK(saw(ch, "a unique thing"));
  CHECK_AUDIT(g);
  th_shutdown(g);
#ifdef __SANITIZE_ADDRESS__
  CHECK(__lsan_do_recoverable_leak_check() == 0); /* the run ends in _exit(), which LeakSanitizer doesn't see */
#endif
}

/* The exit list names a northwest exit in full (the direction went through an eight-letter buffer) */
static void check_exit_list_names_northwest(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Wizard", ADMLVL_IMPL);

  auto *exit    = new room_direction_data();
  exit->to_room = 301;
  room_set_dir_option(g, 300, 6, exit);
  char_from_room(g, ch);
  char_to_room(g, ch, 300);
  th_type(g, ch, "exits");
  CHECK(saw(ch, "Northwest"));
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* The player list capitalises each name without keeping a copy of it */
static void check_player_list_keeps_no_copies(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Wizard", ADMLVL_IMPL);

  th_type(g, ch, "players");
  CHECK(saw(ch, "players listed."));
  CHECK_AUDIT(g);
  th_shutdown(g);
#ifdef __SANITIZE_ADDRESS__
  CHECK(__lsan_do_recoverable_leak_check() == 0); /* the run ends in _exit(), which LeakSanitizer doesn't see */
#endif
}

/* log_imm_action() appends its line, plain or formatted, to the request file */
static void check_imm_action_reaches_the_log(void)
{
  GameWorld *g = th_boot();

  write_file(REQUEST_FILE, ""); /* it only appends to a file that is there */
  log_imm_action(g, "IMM: {} gave {} zenni", "Giver", 42);
  log_imm_action(g, "IMM: plain text, 100% of it");
  std::string text;
  if (FILE *fl = fopen(REQUEST_FILE, "r")) {
    char line[512];
    while (fgets(line, sizeof(line), fl))
      text += line;
    fclose(fl);
  }
  CHECK(text.find(":: IMM: Giver gave 42 zenni\n") != std::string::npos);
  CHECK(text.find(":: IMM: plain text, 100% of it\n") != std::string::npos);
  th_shutdown(g);
}

static void check_message_about_an_object_with_no_description(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct obj_data *obj = read_object(g, 350);
  std::string description(obj_get_short_description(obj)); /* a copy: clearing the field invalidates the view */

  obj_set_short_description(obj, "");
  act(g, "You look at $p.", false, ch, obj, nullptr, act_to::actor);
  CHECK(saw(ch, "You look at"));
  obj_set_short_description(obj, description);
  th_shutdown(g);
}

/* Thing number 12 with no short description, in ch's room: each of these read through the null it gave */
static struct obj_data *thing_with_no_short_description(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj = read_object(g, 350);
  obj_set_short_description(obj, "");
  obj_to_room(g, obj, char_get_in_room(ch));
  return obj;
}

static void check_looking_at_things_with_no_short_description(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);

  thing_with_no_short_description(g, ch);
  struct obj_data *obj = thing_with_no_short_description(g, ch); /* two, so the room's list compares them to stack them */
  th_take_output(g, ch);
  th_type(g, ch, "look");
  CHECK(saw(ch, "(x 2) Thing number 12 lies here."));

  extra_descr_data look;
  look.keyword     = "thing12";
  look.description = "A twelfth thing.\r\n";
  obj_ex_description_slot(obj)->push_back(look); /* looked at by it, then told of its condition by its short description */
  th_take_output(g, ch);
  th_type(g, ch, "look thing12");
  CHECK(saw(ch, "A twelfth thing."));
  CHECK(saw(ch, "\r\n is in"));
  th_shutdown(g);
}

static void check_naming_things_with_no_short_description(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct obj_data *obj = thing_with_no_short_description(g, ch);

  th_take_output(g, ch);
  spell_locate_object(g, 30, ch, nullptr, obj, "");
  CHECK(saw(ch, " is in The Hall."));
  th_take_output(g, ch);
  sub_write(g, "You notice _thing12.", ch, act_to::actor); /* a script's message naming it */
  CHECK(saw(ch, "You notice ."));
  obj_from_room(g, obj);
  obj_to_char(g, obj, ch);
  th_take_output(g, ch);
  th_type(g, ch, "nickname thing12 Bob");
  CHECK(saw(ch, "You nickname  as 'bob'."));
  obj_set_name(obj, "");
  th_take_output(g, ch);
  act(g, "You hold $o.", false, ch, obj, nullptr, act_to::actor); /* $o: the first of its keywords */
  CHECK(saw(ch, "You hold ."));
  th_shutdown(g);
}

static void check_nameless_corpse_decaying(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct obj_data *corpse = thing_with_no_short_description(g, ch);

  obj_set_name(corpse, ""); /* the decay checks its keywords for "android" */
  obj_set_type_flag(corpse, ITEM_CONTAINER);
  obj_set_value(corpse, VAL_CONTAINER_CORPSE, 1);
  obj_set_timer(corpse, 1);
  point_update(g);
  CHECK(count_members(room_contents(g, char_get_in_room(ch))) == 0); /* it decayed, as any corpse does */
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* A mob with no short description, sitting where it doesn't by default: the room's list named it by its first letter */
static void check_looking_at_a_sitting_mob_with_no_short_description(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch  = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *mob = read_mobile(g, 300);

  char_to_room(g, mob, char_get_in_room(ch));
  char_set_sdesc(mob, "");
  char_set_position(mob, POS_SITTING);
  th_take_output(g, ch);
  th_type(g, ch, "look");
  CHECK(saw(ch, "\r\n is sitting here."));
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
  CHECK(char_get_defender(g, first) == ch);
  th_take_output(g, first);
  th_type(g, ch, "defend second");
  CHECK(saw(first, "stops defending you"));
  CHECK(char_get_defender(g, first) == nullptr);
  CHECK(char_get_defender(g, second) == ch);
  CHECK(char_get_defending(g, ch) == second);
  CHECK_AUDIT(g);
  /* one defender at a time: a second takes over from the first, who may not be there to see it */
  char_from_room(g, ch);
  char_to_room(g, ch, 301);
  th_take_output(g, ch);
  th_type(g, rival, "defend second");
  CHECK(saw(ch, "no longer defending"));
  CHECK(char_get_defender(g, second) == rival);
  CHECK(char_get_defending(g, ch) == nullptr);
  CHECK_AUDIT(g);
  th_take_output(g, first);
  th_type(g, rival, "defend second"); /* nothing has changed, so nothing to tell */
  CHECK(!saw(first, "stops defending"));
  CHECK(char_get_defender(g, second) == rival);
  th_shutdown(g);
}

static void make_the_mail_file(void)
{
  write_file(MAIL_FILE, "");
}

static void check_letter_reaches_its_reader(void)
{
  th_before_boot(make_the_mail_file);
  GameWorld *g           = th_boot();
  struct char_data *ch   = th_player(g, "Writer", ADMLVL_NONE);
  struct char_data *rd   = th_player(g, "Reader", ADMLVL_NONE);
  struct char_data *post = read_mobile(g, 300);

  mob_index_of(g, 300)->func = postmaster;
  char_to_room(g, post, char_get_in_room(ch));
  move_next_to(g, rd, ch);
  char_set_gold(ch, 1000);
  char_set_class_level(ch, MIN_MAIL_LEVEL); /* "mail" needs a level of 2, and sending one 3 */
  th_type(g, ch, "mail Reader");
  th_type(g, ch, "Dear Reader.");
  th_type(g, ch, "/s");
  CHECK(saw(ch, "Message sent!"));
  th_take_output(g, ch);

  th_type(g, ch, "mail Reader"); /* and one thrown away */
  th_type(g, ch, "Never mind.");
  th_type(g, ch, "/a");
  CHECK(saw(ch, "Mail aborted."));

  th_type(g, rd, "receive");
  int sent = 0, aborted = 0;
  for (struct obj_data *obj : char_inventory(g, rd)) {
    std::string_view text = obj_get_action_description(obj);
    sent += text.contains("Dear Reader.");
    aborted += text.contains("Never mind.");
  }
  CHECK(sent == 1 && aborted == 0);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_board_post_saves_and_aborts(void)
{
  GameWorld *g             = th_boot();
  struct char_data *ch     = th_player(g, "Writer", ADMLVL_IMPL);
  struct board_info *board = create_new_board(g, 350);

  CHECK(board);
  if (!board) {
    th_shutdown(g);
    return;
  }
  write_board_message(g, 350, ch, "A subject");
  th_type(g, ch, "First post.");
  th_type(g, ch, "/s");
  CHECK(board->num_messages == 1);

  write_board_message(g, 350, ch, "Thrown away");
  th_type(g, ch, "Never mind.");
  th_type(g, ch, "/a");
  CHECK(saw(ch, "Post aborted."));
  CHECK(board->num_messages == 1);

  board_respond(g, 350, ch, 1);
  th_type(g, ch, "A reply.");
  th_type(g, ch, "/s");
  CHECK(board->num_messages == 2);

  th_take_output(g, ch);
  board_display_msg(g, 350, ch, 1);
  board_display_msg(g, 350, ch, 2);
  CHECK(saw(ch, "First post.") && saw(ch, "A subject") && saw(ch, "Re: A subject") && saw(ch, "A reply."));
  CHECK(!saw(ch, "Never mind."));
  th_shutdown(g);
}

/* The lexer reads keywords as whole words, ending at a space or '(', and numbers lines as written. */
static void check_lexer_reads_keywords_strictly(void)
{
  static const struct {
    const char *text;
    dg::keyword kw;
    const char *argument;
  } lines[] = {
      {"if %x% == 1", dg::keyword::if_, "%x% == 1"},
      {"  IF(%x%)", dg::keyword::if_, "(%x%)"},
      {"elseif %y%", dg::keyword::elseif_, "%y%"},
      {"else", dg::keyword::else_, ""},
      {"else return 0", dg::keyword::else_, "return 0"},
      {"elsewhere", dg::keyword::command, ""},
      {"end  ", dg::keyword::end_, ""},
      {"ending", dg::keyword::command, ""},
      {"while %i% < 3", dg::keyword::while_, "%i% < 3"},
      {"done", dg::keyword::done_, ""},
      {"switch %arg%", dg::keyword::switch_, "%arg%"},
      {"case 1", dg::keyword::case_, "1"},
      {"default", dg::keyword::default_, ""},
      {"break", dg::keyword::break_, ""},
      {"* if this", dg::keyword::comment, ""},
      {"   ", dg::keyword::blank, ""},
      {"if%x%", dg::keyword::command, ""},
      {"%echo% if", dg::keyword::command, ""},
  };
  for (const auto &want : lines) {
    auto prog = dg::lex(want.text, "* none");
    CHECK(prog->lines.size() == 1 && prog->lines[0].kw == want.kw);
    if (want.kw != dg::keyword::command && want.kw != dg::keyword::comment)
      CHECK(prog->lines[0].argument() == want.argument);
  }

  auto prog = dg::lex("say one\r\n\r\n  say three\n\nsay five", "* none");
  CHECK(prog->lines.size() == 3);
  CHECK(prog->lines[1].number == 3 && prog->lines[1].body() == "say three" && prog->lines[2].number == 5);
  CHECK(prog->script() == "say one\r\n  say three\r\nsay five\r\n");
  CHECK(dg::lex("\r\n\r\n", "* none")->lines[0].text == "* none");
}

/* The parser builds the tree a script's blocks make, and refuses a script whose blocks don't fit, saying where */
/* A template written out, to compare: text as it is, and each variable as {name|field|>next field:subfield so
   far|(subfield)} */
static std::string describe(const dg::text_template &t)
{
  std::string out;
  for (const dg::template_piece &piece : t.pieces) {
    if (const std::string *text = std::get_if<std::string>(&piece.what)) {
      out += *text;
      continue;
    }
    const dg::var_ref &r = std::get<dg::var_ref>(piece.what);
    out += "{" + r.name;
    if (!r.field.empty() || !r.chain.empty())
      out += "|" + r.field;
    for (const dg::var_ref::link &l : r.chain)
      out += "|>" + l.next_field + (l.subfield_so_far.empty() ? "" : ":" + l.subfield_so_far);
    if (!r.subfield.pieces.empty())
      out += "|(" + describe(r.subfield) + ")";
    out += "}";
  }
  return out;
}

/* An expression written out: (lhs op rhs), [bracketed], or its text */
static std::string describe(const dg::expression &e)
{
  switch (e.what) {
    case dg::expression::kind::op:
      return "(" + describe(e.operands[0]) + e.op + describe(e.operands[1]) + ")";
    case dg::expression::kind::group:
      return "[" + describe(e.operands[0]) + "]";
    case dg::expression::kind::text:
      break;
  }
  return describe(e.text);
}

/* Templates and expressions compile to what the old engine's scan of the text found, quirks and all */
static void check_scripts_compile_as_the_old_scan_read_them(void)
{
  static const struct {
    const char *text, *want;
  } templates[] = {
      {"say %%hi%% %actor.name%!", "say %hi% {actor|name}!"},
      {"%x.y.z%", "{x|y|>z}"},
      {"%a.b(1).c(2)%", "{a|b|>c:1|(12)}"},         /* the subfields run together */
      {"%a.b(x% and %c%", "{a|b|(x{ and }c)}"},       /* an unclosed bracket takes the rest of the line */
      {"%a.%", "{a}"},
      {"100%", "100"},                                /* a lone % at the end is dropped */
      {"%x", "{x}"},                                  /* a variable left open runs to the end */
      {"%a.b.%c%", "{a|b|>%c}"},                      /* the character after a '.' is never looked at */
  };
  for (const auto &t : templates)
    CHECK(describe(dg::compile_template(t.text)) == t.want);

  static const struct {
    const char *text, *want;
  } expressions[] = {
      {"1 + 2 * 3", "(1 +(2 *3))"},
      {"(a == b) && c", "([(a ==b)]&&c)"},
      {"\"a-b\" - 1", "(\"a-b\" -1)"},
      {"x y-1", "(x y-1)"},
      {"-5 + 3", "(-(5 +3))"},
      {"(1", "[]"},                  /* without a ')', the last character goes as though it were one */
      {"(1) junk", "[1]"},           /* and after one, the rest is ignored */
      {"!%x%", "(!{x})"},
      {"\"ab\\", "\"ab\\"},          /* a quote ending in a backslash stops at the end of the text */
  };
  for (const auto &e : expressions)
    CHECK(describe(dg::compile_expression(e.text, dg::max_expression_depth)) == e.want);

  /* nested past the limit, what is left is never compiled */
  std::string deep = std::string(300, '(') + "1" + std::string(300, ')');
  const dg::expression *e = nullptr;
  dg::expression top      = dg::compile_expression(deep, dg::max_expression_depth);
  int depth               = 0;
  for (e = &top; e->what == dg::expression::kind::group; e = &e->operands[0])
    depth++;
  CHECK(depth == dg::max_expression_depth && e->text.pieces.empty());
}

/* Once a line is as long as a command can be, nothing more is looked up in it */
static void check_substituting_stops_at_the_longest_line(void)
{
  GameWorld *g          = th_boot();
  struct char_data *mob = read_mobile(g, 300);
  trig_data with{}, full{};

  char_to_room(g, mob, 300);
  for (trig_data *t : {&with, &full}) {
    add_char_uid_var(t, "actor", mob, 0);
    add_var(t->var_list, "big", std::string(MAX_INPUT_LENGTH, 'x'), 0);
  }
  var_subst(g, mob, nullptr, &with, "%actor.name.car%");
  CHECK(with.var_list.find("tmpvr")); /* the chain worked out the name into tmpvr */
  CHECK(var_subst(g, mob, nullptr, &full, "%big%%actor.name.car%").size() == (size_t)MAX_INPUT_LENGTH - 1);
  CHECK(!full.var_list.find("tmpvr"));
  CHECK(var_subst(g, mob, nullptr, &full, "%big%%big%") == std::string(MAX_INPUT_LENGTH - 1, 'x'));
  th_shutdown(g);
}

/*
 * A message that names more people than its buffer could hold: each '|' with no name after it is "someone's", so a
 * line of them came out nine times as long, and overran the buffer sub_write() built it in.
 */
static void check_message_naming_too_many_people(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("set s ||||||||||||||||||||||||||||||||\n"
                            "set n 0\n"
                            "while %n% < 9\n"
                            "  set s %s%%s%\n"
                            "  eval n %n% + 1\n"
                            "done\n"
                            "wsend %actor% %s%",
                            &ch);

  CHECK(saw(ch, "Someone'ssomeone'ssomeone's"));
  th_shutdown(g);
}

/* eval reads its expression after the line is substituted, so a variable can hold an operator */
static void check_eval_reads_its_line_after_substitution(void)
{
  struct char_data *ch;
  GameWorld *g = run_script("set op +\neval x 2 %op% 3\n%echo% x=%x%.", &ch);

  CHECK(saw(ch, "X=5."));
  th_shutdown(g);
}

static void check_parser_refuses_misplaced_keywords(void)
{
  auto good = dg::lex("if 1\n  say a\nelseif 2\nelse\nend\nwhile 1\n  break\ndone\n"
                      "switch x\n  say before\n  case x\n  default\ndone\n* note\n   \n",
                      "* none");
  CHECK(good->runs() && good->body.size() == 4); /* if, while, switch, and the blank line, which runs */
  const auto &i = std::get<dg::if_node>(good->body[0].what);
  CHECK(i.branches.size() == 3 && i.branches[0].tests && !i.branches[2].tests && i.end == 4);
  CHECK(i.branches[0].body.size() == 1 && i.branches[1].body.empty());
  const auto &w = std::get<dg::while_node>(good->body[1].what);
  CHECK(w.done == 7 && w.body.size() == 1 && std::holds_alternative<dg::break_node>(w.body[0].what));
  const auto &sw = std::get<dg::switch_node>(good->body[2].what);
  CHECK(sw.labels.size() == 2 && sw.labels[0].at == 1 && sw.labels[1].is_default && sw.labels[1].at == 1);

  static const struct {
    const char *script;
    int line;
    const char *message;
  } bad[] = {
      {"say a\nend", 2, "'end' closes nothing"},
      {"if 1\n  done\nend", 2, "'done' closes nothing (the open block is the 'if' at line 1)"},
      {"if 1\nsay a", 1, "'if' never closed"},
      {"if 1\nelse return 0\nend", 2, "text after 'else': return 0"},
      {"while\ndone", 1, "'while' with nothing to test"},
      {"case 1", 1, "'case' outside a switch's own lines"},
      {"switch 1\n  if 1\n    case 1\n  end\ndone", 3, "'case' outside a switch's own lines"},
      {"if 1\n  break\nend", 2, "'break' outside a while or switch"},
      {"say a\n\nelse", 3, "'else' with no 'if' open"},
      {"wait until x", 1, "'wait' needs a time it can read, such as 'wait 5 s' or 'wait until 14:30'"},
      {"wait   ", 1, "'wait' needs a time it can read, such as 'wait 5 s' or 'wait until 14:30'"},
  };
  for (const auto &want : bad) {
    auto prog = dg::lex(want.script, "* none");
    CHECK(!prog->runs() && prog->body.empty());
    CHECK(prog->errors.size() == 1 && prog->lines[prog->errors[0].line].number == want.line &&
          prog->errors[0].message == want.message);
  }
  CHECK(dg::lex("wait %t%", "* none")->runs()); /* known only when it runs */
  CHECK(dg::lex("wait  5 s", "* none")->runs());

  /* Where a waiting run carries on, as tstat shows it */
  auto prog = dg::lex("say a\nwhile 1\n  wait 1\n  say b\ndone\nsay c", "* none");
  const auto &loop = std::get<dg::while_node>(prog->body[1].what);
  dg::run_state run;
  run.prog   = prog;
  run.frames = {{dg::block_role::script, &prog->body, 2, nullptr}, {dg::block_role::loop, &loop.body, 1, &prog->body[1]}};
  CHECK(run.next_line() == &prog->lines[3]); /* say b */
  run.frames.back().next = 2;
  CHECK(run.next_line() == &prog->lines[4]); /* done, where the loop is tested again */
  run.frames.pop_back();
  CHECK(run.next_line() == &prog->lines[5]); /* say c */
  run.frames.back().next = 3;
  CHECK(run.next_line() == nullptr);
}

/* An object given a trigger in play, so with none from its prototype, shows to a builder with room flags on. */
static void check_attached_trigger_shows_with_room_flags(void)
{
  GameWorld *g          = th_boot();
  struct char_data *ch  = th_player(g, "Builder", ADMLVL_IMPL);
  struct obj_data *obj  = read_object(g, 350);

  obj_set_proto_script(obj, {}); /* as if its prototype had none */
  obj_to_room(g, obj, char_get_in_room(ch));
  add_trigger(g, dg_ensure_script(obj_script_slot(obj)), read_trigger(g, find_trigger(g, 312)), -1);
  char_prf_set(g, ch, pref_flag::roomflags, true);
  th_take_output(g, ch);
  th_type(g, ch, "look"); /* this read the prototype's first trigger, which there isn't */
  CHECK(strstr(th_output(ch), "350") != nullptr);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_trigger_argument_can_be_cleared(void)
{
  GameWorld *g         = th_boot();
  struct char_data *ch = th_player(g, "Writer", ADMLVL_IMPL);

  CHECK(find_trigger(g, 312)->proto->arglist == "dance");
  th_type(g, ch, "trigedit 312");
  th_type(g, ch, "5");
  th_type(g, ch, ""); /* no argument at all */
  th_type(g, ch, "q");
  th_type(g, ch, "y");
  CHECK(find_trigger(g, 312)->proto->arglist.empty());
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* Saving a trigger in trigedit gives its live copies the new script for their next runs, while a run that waits
 * finishes on the script it started; tstat shows the copies and where each run is, and tcheck the triggers that don't
 * compile. Saving used to cancel the waiting run, with its variables. */
static void check_trigger_hot_reload(void)
{
  GameWorld *g          = th_boot();
  struct char_data *ch  = th_player(g, "Builder", ADMLVL_IMPL);
  struct char_data *mob = read_mobile(g, 300);
  char_to_room(g, mob, char_get_in_room(ch));

  find_trigger(g, 312)->proto->program =
      dg::lex("set word old\nwhile 1\n  wait 1\n  %echo% the %word% script\n  break\ndone", "* none");
  trig_data *trig = read_trigger(g, find_trigger(g, 312));
  add_trigger(g, dg_ensure_script(char_script_slot(mob)), trig, -1);
  script_driver(g, mob, trig, TRIG_NEW);
  CHECK(trig->wait_event != nullptr);

  th_take_output(g, ch);
  th_type(g, ch, "tstat 312");
  CHECK(strstr(th_output(ch), "Live instances: 2") != nullptr); /* and the one critter2 has from its prototype */
  CHECK(strstr(th_output(ch), str_format("#{:d} on mob 'a training dummy' #300", trig->id).c_str()) != nullptr);
  CHECK(strstr(th_output(ch), ": waiting, 1 pulse left") != nullptr);
  CHECK(strstr(th_output(ch), "at line 4: %echo% the %word% script") != nullptr);
  CHECK(strstr(th_output(ch), "inside while at line 2") != nullptr);

  th_type(g, ch, "trigedit 312");
  th_type(g, ch, "6");
  th_type(g, ch, "/c");
  th_type(g, ch, "%echo% the new script");
  th_type(g, ch, "/s");
  th_type(g, ch, "q");
  th_type(g, ch, "y");
  CHECK(trig->wait_event != nullptr && trig->run.prog != trig->program);
  CHECK(trig->program == find_trigger(g, 312)->proto->program);
  th_take_output(g, ch);
  th_type(g, ch, "tstat 312");
  CHECK(strstr(th_output(ch), "on the script as it was before it was last saved") != nullptr);

  th_take_output(g, ch);
  th_pulses(g, 5 * PASSES_PER_SEC);
  CHECK(strstr(th_output(ch), "The old script") != nullptr);
  CHECK(!trig->run.running() && !trig->run.prog);
  th_take_output(g, ch);
  script_driver(g, mob, trig, TRIG_NEW);
  CHECK(strstr(th_output(ch), "The new script") != nullptr);

  th_take_output(g, ch);
  th_type(g, ch, "tcheck");
  CHECK(strstr(th_output(ch), "Every trigger compiles.") != nullptr);
  th_type(g, ch, "trigedit 312");
  th_type(g, ch, "6");
  th_type(g, ch, "/c");
  th_type(g, ch, "if 1");
  th_type(g, ch, "/s");
  th_type(g, ch, "q");
  th_take_output(g, ch);
  th_type(g, ch, "y");
  CHECK(strstr(th_output(ch), "Error: line 1: 'if' never closed") != nullptr);
  th_take_output(g, ch);
  th_type(g, ch, "tcheck");
  CHECK(strstr(th_output(ch), "1 of ") != nullptr);
  CHECK(strstr(th_output(ch), "line 1: 'if' never closed") != nullptr);
  th_take_output(g, ch);
  th_type(g, ch, "tcheck 312");
  CHECK(strstr(th_output(ch), "does not run:\r\n  line 1: 'if' never closed") != nullptr);
  th_take_output(g, ch);
  th_type(g, ch, "tstat 312");
  CHECK(strstr(th_output(ch), "It does not run: its script has errors.") != nullptr);
  CHECK(strstr(th_output(ch), str_format("#{:d} on mob 'a training dummy'", trig->id).c_str()) != nullptr);
  CHECK(strstr(th_output(ch), ": idle") != nullptr);

  th_take_output(g, ch);
  th_type(g, ch, "detach dummy 312");
  CHECK(strstr(th_output(ch), "Trigger removed.") != nullptr);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_mob_given_armor_with_no_wear_slot(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *mob  = read_mobile(g, 300);
  struct obj_data *armor = read_object(g, 350);

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
  room_vnum start = CONFIG_MORTAL_START;

  CHECK(room_of(g, start));
  CHECK(!delete_room(g, start));
  CHECK(CONFIG_MORTAL_START == start);
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
  CHECK(g->shops.size() == 1); /* the one shop, still there */
  struct shop_data *shop = *g->shops.all().begin();
  const std::vector<shop_buy_data> &buys = shop_buy_types(shop);
  CHECK(buys.size() == 2);
  CHECK(buys[0].type == ITEM_ARMOR);
  CHECK(buys[0].keywords == "potions");
  CHECK(buys[1].type == 19);
  CHECK(buys[1].keywords.empty());
  th_shutdown(g);
}

static void empty_record_before_the_shops(void)
{
  FILE *fl = fopen("world/shp/3.shp", "r");
  char text[65536];
  size_t len;

  if (!fl)
    abort();
  len       = fread(text, 1, sizeof(text) - 1, fl);
  text[len] = '\0';
  fclose(fl);
  /* the version line, then an empty record, then the shops as they were */
  char *rest = strchr(text, '\n');
  if (!rest)
    abort();
  fl = fopen("world/shp/3.shp", "w");
  fwrite(text, 1, rest + 1 - text, fl);
  fputs("~\n", fl);
  fputs(rest + 1, fl);
  fclose(fl);
}

static void replace_in_file(const char *path, const char *from, const char *to);

static void shop_with_negative_flags(void)
{
  replace_in_file("world/shp/3.shp", "\n0\n0\n301\n", "\n0\n-1\n301\n");
}

/* quirk: the flags were read as a long into 4 bytes, spilling into the keeper read after them; a negative one leaves
   the keeper a vnum no mob has */
static void check_shop_flags_spill_into_the_keeper(void)
{
  GameWorld *g = th_boot();
  CHECK(shop_get_keeper(*g->shops.all().begin()) == 301);
  th_shutdown(g);

  th_before_boot(shop_with_negative_flags);
  g = th_boot();
  shop_data *shop = *g->shops.all().begin();
  CHECK(shop_get_keeper(shop) == NOBODY);
  CHECK(shop_get_bitvector(shop).word(0) == 0xFFFFFFFFu);
  th_shutdown(g);
}

static void check_empty_record_in_a_shop_file(void)
{
  th_before_boot(empty_record_before_the_shops);
  GameWorld *g = th_boot();

  CHECK(g->shops.size() == 1); /* the fixture's shop, still read after it */
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
  CHECK(!mob_proto_of(g, SHADOW_DRAGON1_VNUM)); /* or this checks nothing */
  CHECK(g->SHADOW_DRAGON1 == -1);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

static void check_homing_ball_that_someone_is_carrying(void)
{
  GameWorld *g = th_boot();
  struct char_data *ch = th_player(g, "Tester", ADMLVL_NONE);
  struct char_data *target = th_player(g, "Target", ADMLVL_NONE);
  struct obj_data *ball    = read_object(g, 80);
  struct obj_data *other   = read_object(g, 350);

  char_from_room(g, target);
  char_to_room(g, target, 301);
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
  struct obj_data *obj = read_object(g, 350);
  char text[400], name[201];

  memset(name, 'x', 200);
  name[200] = '\0';
  snprintf(text, sizeof(text), "1 %s 1 1 1 1 1\n", name);
  write_file(AUCTION_FILE, text);
  auc_load(obj);
  CHECK_AUDIT(g);
  th_shutdown(g);
}

/* the first from in path becomes to */
static void replace_in_file(const char *path, const char *from, const char *to)
{
  FILE *fl = fopen(path, "r");
  std::string text;
  char chunk[4096];
  size_t n;

  if (!fl)
    abort();
  while ((n = fread(chunk, 1, sizeof(chunk), fl)) > 0)
    text.append(chunk, n);
  fclose(fl);
  size_t at = text.find(from);
  if (at == std::string::npos)
    abort();
  text.replace(at, strlen(from), to);
  write_file(path, text.c_str());
}

/* boots with prepare() run first; true if the game refused to start (exit 1), saying why in a SYSERR that has
 * reason in it: UBSan stops a run with an exit of 1 too */
static void (*refused_prepare)(void);

static void prepare_and_log_to_stderr(void)
{
  refused_prepare();
  if (symlink("/dev/stderr", "test.log")) /* where the harness logs */
    abort();
}

static bool boot_is_refused(void (*prepare)(void), const char *reason)
{
  int status = 0, out[2];
  pid_t child;
  std::string said;
  char chunk[4096];
  ssize_t n;

  fflush(stdout);
  fflush(stderr);
  if (pipe(out))
    abort();
  if (!(child = fork())) {
    close(out[0]);
    dup2(out[1], STDOUT_FILENO);
    dup2(out[1], STDERR_FILENO);
    refused_prepare = prepare;
    th_before_boot(prepare_and_log_to_stderr);
    th_boot();
    th_exit(0);
  }
  close(out[1]);
  while ((n = read(out[0], chunk, sizeof(chunk))) > 0)
    said.append(chunk, (size_t)n);
  close(out[0]);
  waitpid(child, &status, 0);
  if (said.find("runtime error") != std::string::npos || said.find("ERROR: AddressSanitizer") != std::string::npos ||
      said.find("Assertion") != std::string::npos)
    file_print(stdout, "{}", said);
  if (getenv("SHOW_REFUSAL")) file_print(stdout, "[status {:d}] {}\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1, said.substr(said.size() > 1500 ? said.size() - 1500 : 0));
  return WIFEXITED(status) && WEXITSTATUS(status) == 1 && said.find(reason) != std::string::npos;
}

static void exit_in_a_direction_that_does_not_exist(void)
{
  replace_in_file("world/wld/3.wld", "D0\n", "D300\n");
}

/* There are only NUM_OF_DIRS exits: the direction was written past the end of them. */
static void check_exit_in_a_direction_that_does_not_exist(void)
{
  CHECK(boot_is_refused(exit_in_a_direction_that_does_not_exist, "SYSERR: world/wld/3.wld:6: room #300: there is no direction D300"));
}

static void zone_command_with_a_space_before_it(void)
{
  replace_in_file("world/zon/3.zon", "\nM 0 300 5 301 2 0\n", "\n M 0 300 5 301 2 0\n");
}

/* The commands were counted, to make room for them, by a looser rule than they are read by: one with a space before
 * it was read but not counted, and written past the end. */
static void check_zone_command_with_a_space_before_it(void)
{
  CHECK(boot_is_refused(zone_command_with_a_space_before_it,
                        "SYSERR: world/zon/3.zon:72: zone #3: more commands than the 66 counted"));
}

/* Damaged files of each kind the boot reads, each with what the game says when it refuses to start. These fix
 * today's messages, so that a change to them shows; the loaders are being rewritten (see the file I/O plan). */
static const struct damaged_file {
  const char *path, *from, *to;
  const char *reason;
} damaged_files[] = {
    {"world/obj/3.obj", "15 0 0 0 0 a 0 0 0 0 0 0 0\n", "15 0 0 0 0\n",
     "SYSERR: world/obj/3.obj:6: object #300: expected its type, 4 extra flags, 4 wear flags and 4 affect flags, got '15 0 0 0 0'"},
    {"world/obj/3.obj", "#301\nsword blade~", "#300\nsword blade~", "SYSERR: world/obj/3.obj:11: object #300 is defined twice"},
    {"world/obj/3.obj", "3 50 0 0\nZ\n4\n#302", "3 50 0 0\nA\nx\n#302", "SYSERR: world/obj/3.obj:20: object #301: after its 'A', expected its location, modifier and specific, got 'x'"},
    {"world/obj/3.obj", "3 50 0 0\nZ\n4\n#302", "3 50 0 0\nQ\n#302", "SYSERR: world/obj/3.obj:19: object #301: expected E, A, S, T or Z, or the next '#<vnum>' or the '$', got 'Q'"},
    {"world/mob/3.mob", "d 0 0 0 0 0 0 0 0 E\n", "d 0 0 0 0 0 0 0 0 X\n", "SYSERR: world/mob/3.mob:8: mob #300: there is no mob type 'X', only S and E"},
    {"world/mob/3.mob", "#301\nkeeper", "#300\nkeeper", "SYSERR: world/mob/3.mob:19: mob #300 is defined twice"},
    {"world/mob/3.mob", "d 0 0 0 0 0 0 0 0 E\n", "d 0 E\n",
     "SYSERR: world/mob/3.mob:8: mob #300: expected 4 act flags, 4 affect flags, its alignment and S or E, got 'd 0 E'"},
    {"world/mob/3.mob", "1 0 10 0d0+0 0d0+0\n0 0 0 0\n8 8 0\nStr", "1 0 10 0x0+0 0d0+0\n0 0 0 0\n8 8 0\nStr",
     "SYSERR: world/mob/3.mob:9: mob #300: expected its level, hit roll, armor, points and damage, as '# # # #d#+# #d#+#'"},
    {"world/mob/3.mob", "Cha: 10\nE\n#301", "Cha: 10\n#301", "SYSERR: world/mob/3.mob:18: mob #300: its especs have no 'E' to end them"},
    {"world/obj/3.obj", "4\n#301", "x\n#301", "SYSERR: world/obj/3.obj:10: object #300: after its 'Z', expected its size, got 'x'"},
    {"world/zon/3.zon", "300 399 5 2 0 0 0 0 0 0\n", "300 399 5\n", "SYSERR: world/zon/3.zon:5: zone #3: expected 10 numbers and flags"},
    {"world/zon/3.zon", "@Version: 2\n", "@Version: x\n", "SYSERR: world/zon/3.zon:1: expected \"@Version: <number>\", got '@Version: x'"},
    {"world/obj/index", "3.obj\n", "3.obj\nmissing.obj\n", "SYSERR: world/obj/missing.obj: can't be opened, and world/obj/index lists it"},
    {"world/wld/3.wld", "#301\n", "#300\n", "SYSERR: world/wld/3.wld:19: room #300 is defined twice"},
    {"world/zon/3.zon", "\nS\n$\n", "\n", "SYSERR: world/zon/3.zon:71: zone #3: the file ends before the 'S' that ends its commands"},
    {"world/trg/3.trg", "2 g 100\n", "2\n", "SYSERR: world/trg/3.trg:3: trigger #300: expected its attach type and flags, got '2'"},
    {"world/trg/3.trg", "#301\n", "301\n", "SYSERR: world/trg/3.trg:7: expected a #<vnum>, or the '$' that ends the file, after trg #300; got '301'"},
    {"world/trg/3.trg", "#301\n", "#x\n", "SYSERR: world/trg/3.trg:7: expected a number after the '#', after trg #300"},
    /* the old 128-bit conversion and the exits without DCs are gone: those lines are now refused */
    {"world/wld/3.wld", "3 0 0 0 0 0\n", "3 0 0\n", "SYSERR: world/wld/3.wld:5: room #300: expected its zone, 4 flags and its sector, got '3 0 0'"},
    {"world/wld/3.wld", "0 0 301 0 0 0 0 0 0 0 0\n", "0 0 301 20 20 0 0\n", "SYSERR: world/wld/3.wld:9: room #300, exit D0: expected 11 numbers"},
    {"world/wld/3.wld", "0 0 303 0 0 0 0 0 0 0 0\nS\n", "0 0 303 0 0 0 0 0 0 0 0\nQ\n", "SYSERR: world/wld/3.wld:18: room #300: expected D (an exit), E (an extra description) or S (the end), got 'Q'"},
    {"world/wld/3.wld", "#300\n", "#-5\n", "SYSERR: world/wld/3.wld:1: room #-5 is outside of any zone"},
    {"world/shp/3.shp", "-1\n1.00\n", "-1\nx\n", "SYSERR: world/shp/3.shp:5: shop #300: expected its buy profit, got 'x'"},
    {"world/shp/3.shp", "\n301\n0 0 16384", "\nx\n0 0 16384", "SYSERR: world/shp/3.shp:18: shop #300: expected its keeper's vnum, got 'x'"},
    {"world/shp/3.shp", "CircleMUD v3.0 Shop File~", "CircleMUD Shop File~",
     "SYSERR: world/shp/3.shp:2: shop #300 comes before the \"v3.0\" header; shops older than v3.0 aren't read any more"},
    {"world/gld/3.gld", "-1\n1.00\n", "-1\nx\n", "SYSERR: world/gld/3.gld:10: guild #300: expected its charge, got 'x'"},
    {"world/gld/3.gld", "427 1\n", "427 3\n", "SYSERR: world/gld/3.gld:8: guild #300: 3 after skill 427 is neither 1 (a skill) nor 2 (a feat)"},
    {"world/obj/3.obj", "15 0 0 0 0 a 0 0 0 0 0 0 0\n", "15 0 a 0\n",
     "SYSERR: world/obj/3.obj:6: object #300: expected its type, 4 extra flags, 4 wear flags and 4 affect flags, got '15 0 a 0'"},
    /* quirk: a string missing its ~ takes in the next one, and the error is found on a later line */
    {"world/obj/3.obj", "A wooden chest sits here.~\n", "A wooden chest sits here.\n",
     "SYSERR: world/obj/3.obj:13: object #300: expected its type, 4 extra flags, 4 wear flags and 4 affect flags, got 'a plain sword~'"},
};
static const damaged_file *damaging;

static void damage_a_file(void)
{
  replace_in_file(damaging->path, damaging->from, damaging->to);
}

static void check_damaged_files_refuse_the_boot(void)
{
  for (const damaged_file &d : damaged_files) {
    damaging = &d;
    if (!boot_is_refused(damage_a_file, d.reason)) {
      file_print(stdout, "not refused as expected: {} ('{}'), for '{}'\n", d.path, d.to, d.reason);
      CHECK(false);
    }
  }
}

static const struct {
  const char *name;
  void (*check)(void);
} checks[] = {
    {"while_without_done",                          check_while_without_done                         },
    {"break_without_done",                          check_break_without_done                         },
    {"switch_around_an_unfinished_while",           check_switch_around_an_unfinished_while          },
    {"a_finished_while_still_runs",                 check_a_finished_while_still_runs                },
    {"long_chain_of_operators",                     check_long_chain_of_operators                    },
    {"chain_as_long_as_real_scripts_have",          check_chain_as_long_as_real_scripts_have         },
    {"expression_values",                           check_expression_values                          },
    {"arithmetic_at_the_limits",                    check_arithmetic_at_the_limits                   },
    {"script_variable_has_a_limit",                 check_script_variable_has_a_limit                },
    {"chained_field_on_a_short_variable",           check_chained_field_on_a_short_variable          },
    {"attached_trigger_shows_with_room_flags",      check_attached_trigger_shows_with_room_flags     },
    {"lexer_reads_keywords_strictly",               check_lexer_reads_keywords_strictly              },
    {"parser_refuses_misplaced_keywords",           check_parser_refuses_misplaced_keywords          },
    {"scripts_compile_as_the_old_scan_read_them",   check_scripts_compile_as_the_old_scan_read_them  },
    {"substituting_stops_at_the_longest_line",      check_substituting_stops_at_the_longest_line     },
    {"eval_reads_its_line_after_substitution",      check_eval_reads_its_line_after_substitution     },
    {"message_naming_too_many_people",              check_message_naming_too_many_people             },
    {"variable_left_open_at_the_end",               check_variable_left_open_at_the_end              },
    {"wait_too_long_to_count",                      check_wait_too_long_to_count                     },
    {"wait_until_no_time",                          check_wait_until_no_time                         },
    {"message_ending_in_a_backslash",               check_message_ending_in_a_backslash              },
    {"text_field_of_self",                          check_text_field_of_self                         },
    {"add_commas",                                  check_add_commas                                 },
    {"flag_sets",                                   check_flag_sets                                  },
    {"act_split",                                   check_act_split                                  },
    {"asctime_text",                                check_asctime_text                               },
    {"guildmaster_special_hears_the_command",       check_guildmaster_special_hears_the_command      },
    {"scan_range",                                  check_scan_range                                 },
    {"auction_list_leaves_player_names_alone",      check_auction_list_leaves_player_names_alone     },
    {"args_match_the_old_parsers",                  check_args_match_the_old_parsers                 },
    {"name_matching_matches_the_old_code",          check_name_matching_matches_the_old_code         },
    {"spellbook_edit_can_be_abandoned",             check_spellbook_edit_can_be_abandoned            },
    {"get_all_from_all_containers",                 check_get_all_from_all_containers                },
    {"mjunk_by_name",                               check_mjunk_by_name                              },
    {"long_zone_echo",                              check_long_zone_echo                             },
    {"mload_mob_into_a_room",                       check_mload_mob_into_a_room                      },
    {"emote_of_many_hashes",                        check_emote_of_many_hashes                       },
    {"wiznet_keeps_no_copies",                      check_wiznet_keeps_no_copies                     },
    {"deleted_object_leaves_no_put_command",        check_deleted_object_leaves_no_put_command       },
    {"imm_action_reaches_the_log",                  check_imm_action_reaches_the_log                 },
    {"sensing_keeps_no_copies",                     check_sensing_keeps_no_copies                    },
    {"shop_with_no_customers",                      check_shop_with_no_customers                     },
    {"guild_with_no_customers",                     check_guild_with_no_customers                    },
    {"long_help_index",                             check_long_help_index                            },
    {"rent_listing_keeps_no_copies",                check_rent_listing_keeps_no_copies               },
    {"exit_list_names_northwest",                   check_exit_list_names_northwest                  },
    {"player_list_keeps_no_copies",                 check_player_list_keeps_no_copies                },
    {"specific_goes_with_its_location",             check_specific_goes_with_its_location            },
    {"affects_on_two_skills_stay_apart",            check_affects_on_two_skills_stay_apart           },
    {"saved_affects_are_cleaned_up",                check_saved_affects_are_cleaned_up               },
    {"infravision_comes_off_an_android",            check_infravision_comes_off_an_android           },
    {"cut_off_skill_list",                          check_cut_off_skill_list                         },
    {"impossible_indexes_in_a_save",                check_impossible_indexes_in_a_save               },
    {"birth_at_the_end_of_time",                    check_birth_at_the_end_of_time                   },
    {"remote_display_survives_a_load",              check_remote_display_survives_a_load             },
    {"no_object_file_is_no_problem",                check_no_object_file_is_no_problem               },
    {"too_many_applies_on_a_saved_object",          check_too_many_applies_on_a_saved_object         },
    {"saved_object_with_a_negative_unique_id",      check_saved_object_with_a_negative_unique_id     },
    {"boot_with_no_socials",                        check_boot_with_no_socials                       },
    {"index_without_an_end",                        check_index_without_an_end                       },
    {"guild_with_impossible_numbers",               check_guild_with_impossible_numbers              },
    {"carrying_more_than_a_byte_can_count",         check_carrying_more_than_a_byte_can_count        },
    {"mob_memory_gets_forgotten",                   check_mob_memory_gets_forgotten                  },
    {"android_without_a_model",                     check_android_without_a_model                    },
    {"race_name_forms_stay_apart",                  check_race_name_forms_stay_apart                 },
    {"zone_range_keeps_what_is_in_it",              check_zone_range_keeps_what_is_in_it             },
    {"editor_saves_and_aborts",                     check_editor_saves_and_aborts                    },
    {"news_entry_leaves_the_immortal_list_alone",   check_news_entry_leaves_the_immortal_list_alone  },
    {"letter_reaches_its_reader",                   check_letter_reaches_its_reader                  },
    {"board_post_saves_and_aborts",                 check_board_post_saves_and_aborts                },
    {"trigger_argument_can_be_cleared",             check_trigger_argument_can_be_cleared            },
    {"trigger_hot_reload",                          check_trigger_hot_reload                         },
    {"new_prototypes_leave_other_editors_alone",    check_new_prototypes_leave_other_editors_alone   },
    {"worn_container_that_cannot_go_back_on",       check_worn_container_that_cannot_go_back_on      },
    {"long_words_in_ban_file",                      check_long_words_in_ban_file                     },
    {"long_word_in_socials",                        check_long_word_in_socials                       },
    {"long_word_in_index",                          check_long_word_in_index                         },
    {"help_keyword_with_an_unfinished_quote",       check_help_keyword_with_an_unfinished_quote      },
    {"more_affects_than_a_save_holds",              check_more_affects_than_a_save_holds             },
    {"message_about_an_object_with_no_description", check_message_about_an_object_with_no_description},
    {"looking_at_things_with_no_short_description", check_looking_at_things_with_no_short_description},
    {"naming_things_with_no_short_description",     check_naming_things_with_no_short_description    },
    {"nameless_corpse_decaying",                    check_nameless_corpse_decaying                   },
    {"looking_at_a_sitting_mob_with_no_short_description", check_looking_at_a_sitting_mob_with_no_short_description},
    {"saying_something_to_someone",                 check_saying_something_to_someone                },
    {"defending_someone_else_lets_go_of_the_first", check_defending_someone_else_lets_go_of_the_first},
    {"mob_given_armor_with_no_wear_slot",           check_mob_given_armor_with_no_wear_slot          },
    {"start_room_cannot_be_deleted",                check_start_room_cannot_be_deleted               },
    {"start_room_missing_from_the_world",           check_start_room_missing_from_the_world          },
    {"shop_with_impossible_item_types",             check_shop_with_impossible_item_types            },
    {"empty_record_in_a_shop_file",                 check_empty_record_in_a_shop_file                },
    {"shop_flags_spill_into_the_keeper",            check_shop_flags_spill_into_the_keeper           },
    {"shadow_dragon_missing_from_the_world",        check_shadow_dragon_missing_from_the_world       },
    {"homing_ball_that_someone_is_carrying",        check_homing_ball_that_someone_is_carrying       },
    {"long_name_in_auction_file",                   check_long_name_in_auction_file                  },
    {"exit_in_a_direction_that_does_not_exist",     check_exit_in_a_direction_that_does_not_exist    },
    {"zone_command_with_a_space_before_it",         check_zone_command_with_a_space_before_it        },
    {"help_line_starting_with_a_nul",               check_help_line_starting_with_a_nul              },
    {"empty_context_help",                          check_empty_context_help                         },
    {"player_file_cut_short_in_its_description",    check_player_file_cut_short_in_its_description   },
    {"old_binary_mail_is_converted",                check_old_binary_mail_is_converted               },
    {"old_binary_mail_with_a_broken_link",          check_old_binary_mail_with_a_broken_link         },
    {"old_binary_houses_are_read",                  check_old_binary_houses_are_read                 },
    {"hcontrol_that_cant_be_read_refuses_the_boot", check_hcontrol_that_cant_be_read_refuses_the_boot},
    {"house_records_past_their_limits",             check_house_records_past_their_limits            },
    {"rent_file_cut_short_in_a_string",             check_rent_file_cut_short_in_a_string            },
    {"long_sense_memory",                           check_long_sense_memory                          },
    {"toplist_name_listed_twice_at_the_bottom",     check_toplist_name_listed_twice_at_the_bottom    },
    {"damaged_files_refuse_the_boot",               check_damaged_files_refuse_the_boot              },
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
      file_print(stdout, "{} failed ({} {:d})\n", checks[i].name, WIFEXITED(status) ? "status" : "signal", WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status));
      failed++;
    }
  }
  if (failed)
    file_print(stdout, "{:d} check(s) failed\n", failed);
  else
    file_print(stdout, "all checks passed\n");
  return failed != 0;
}
