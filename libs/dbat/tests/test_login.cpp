/*
 * Journeys through the login and chargen menus, each over a real connection (th_connect), to compare two builds:
 *
 *   dbat_test_login [--transcript FILE] [--shard K/N] [PREFIX]
 *
 * A journey is the lines a client types from connecting. Lines starting with @ are the client's own doings:
 *   @connect   connects again, as a new connection that the lines after it are typed at
 *   @leave     hangs up the current connection
 *   @pulse N   runs N pulses of the game
 * Each journey runs in its own process, from the same fresh boot, with its own seed and the clock stopped. After
 * every line the transcript has what the client was sent and the state its connection is in; at the end, the
 * account and player files the journey left, and who and what is in the fixture's rooms.
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/create.hpp"
#include "dbat/consts/constates.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/iterate.hpp"
#include "dbat/players.hpp"
#include "dbat/random.hpp"
#include "dbat/utils.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/game_world.hpp"
#include "harness.hpp"

namespace {
  constexpr time_t TRANSCRIPT_TIME = 1700000000; /* the clock, stopped */
  constexpr int CASE_SECONDS       = 120;

  FILE *transcript;
  int cases, failures;

  /* An answer to give whenever the connection is in this state, in turn */
  using answers = std::vector<std::pair<int, const char *>>;

  struct journey {
    const char *name;
    std::vector<const char *> lines;
    void (*setup)(GameWorld *g) = nullptr; /* before connecting */
    answers by_state = {};                 /* after the lines, answer by state until in the game */
    bool drive       = false;
    std::vector<const char *> after = {};  /* typed once in the game, as the lines are */
  };

  /* What a journey answers in a state it says nothing of; "1" for the rest */
  const answers DEFAULTS = {
      {CON_QRACE, "human"}, {CON_QCLASS, "roshi"}, {CON_QSEX, "m"},   {CON_SKILLS, "keep"}, {CON_BONUS, "x"},
      {CON_NEGATIVE, "x"},  {CON_QROLLSTATS, ""},  {CON_RMOTD, ""},   {CON_MENU, "1"},      {CON_GENOME, "1"},
      {CON_GENOME, "2"},    {CON_GENOME, ""},      {CON_GENOME, ""},
  };

  /* The account and character a new journey makes, and the answers to get one into the game */
  const std::vector<const char *> NEW_ACCOUNT = {"Tester", "y", "tester@example.com", "Pa55word1"};

  std::vector<const char *> then(std::vector<const char *> first, std::initializer_list<const char *> rest)
  {
    first.insert(first.end(), rest);
    return first;
  }

  /* A new character, made by answering by state */
  journey made(const char *name, answers by_state, std::vector<const char *> after = {}, void (*setup)(GameWorld *) = nullptr)
  {
    return {name, then(NEW_ACCOUNT, {"1", "Testchar"}), setup, std::move(by_state), true, std::move(after)};
  }

  /* An account already made, before connecting: Tester, with its password, these RPP and slots, and its first
     character if named */
  void account_file(int rpp, int slots, const char *first = "Empty")
  {
    system("mkdir -p user/P-T");
    FILE *fl = fopen("user/P-T/tester.usr", "w");
    if (!fl)
      return;
    fprintf(fl, "Tester\ntester<AT>example.com\nPa55word1\n%d\n%d\n%s\nEmpty\nEmpty\nEmpty\nEmpty\n1\n1\n0\n", slots, rpp, first);
    fclose(fl);
  }
  void rich_account(GameWorld *) { account_file(40, 3); }
  void full_account(GameWorld *) { account_file(40, 5); }
  void four_slot_account(GameWorld *) { account_file(5, 4); }
  /* The account's first slot holds Gonechar, whose player file is marked deleted (as one deleted with the fast wipe
     off leaves it) */
  void deleted_in_slot(GameWorld *g)
  {
    struct char_data *pc = new char_data();
    clear_char(pc);
    char_make_player_specials(pc);
    char_set_pc_name(pc, "Gonechar");
    char_set_race(pc, RACE_HUMAN);
    char_set_class(pc, CLASS_ROSHI);
    char_set_pfilepos(pc, create_entry(g, char_get_pc_name(pc)));
    init_char(g, pc);
    char_plr_set(pc, PLR_DELETED, true);
    save_char(g, pc);
    save_player_index(g);
    free_char(g, pc);
    account_file(40, 3, "Gonechar");
  }

  /* Base stats set by points (creation methods 2 and 3), with rerolling or not */
  void points_method_2(GameWorld *g)
  {
    CONFIG_CREATION_METHOD        = CEDIT_CREATION_METHOD_2;
    CONFIG_REROLL_PLAYER_CREATION = 1;
  }
  void points_method_2_no_reroll(GameWorld *g)
  {
    CONFIG_CREATION_METHOD        = CEDIT_CREATION_METHOD_2;
    CONFIG_REROLL_PLAYER_CREATION = 0;
  }
  void points_method_3(GameWorld *g) { CONFIG_CREATION_METHOD = CEDIT_CREATION_METHOD_3; }
  void reroll_method_1(GameWorld *g)
  {
    CONFIG_CREATION_METHOD        = CEDIT_CREATION_METHOD_1;
    CONFIG_REROLL_PLAYER_CREATION = 1;
  }

  /* The stats editor's answers: every stat, out of range both ways, then quitting */
  const answers STATS = {{CON_QSTATS, "?"}, {CON_QSTATS, "s"}, {CON_QSTATS, "20"}, {CON_QSTATS, "d"}, {CON_QSTATS, "200"}, {CON_QSTATS, "n"},
                         {CON_QSTATS, "1"}, {CON_QSTATS, "i"}, {CON_QSTATS, "abc"}, {CON_QSTATS, "w"}, {CON_QSTATS, "10"}, {CON_QSTATS, "c"},
                         {CON_QSTATS, "10"}, {CON_QSTATS, "S"}, {CON_QSTATS, "15"}, {CON_QSTATS, "q"}};

  answers with_stats(answers first)
  {
    first.insert(first.end(), STATS.begin(), STATS.end());
    return first;
  }

  const journey JOURNEYS[] = {
      made("chargen, stats by points, rerolling first", with_stats({{CON_QROLLSTATS, "n"}, {CON_QROLLSTATS, "y"}}), {}, points_method_2),
      made("chargen, stats by points, no rerolling", with_stats({{CON_QROLLSTATS, ""}}), {}, points_method_2_no_reroll),
      made("chargen, stats from a pool", with_stats({{CON_QROLLSTATS, ""}}), {}, points_method_3),
      made("chargen, stats rerolled", {{CON_QROLLSTATS, "n"}, {CON_QROLLSTATS, "maybe"}, {CON_QROLLSTATS, "Y"}}, {}, reroll_method_1),
      /* the menus once made, and coming back */
      made("menu, a bad choice, then a description", {{CON_MENU, "9"}, {CON_MENU, "2"}, {CON_EXDESC, "A tall fellow."}, {CON_EXDESC, "/s"}}),
      made("menu, deleting the character, not", {{CON_MENU, "4"}, {CON_DELCNF2, "no"}}),
      made("menu, deleting the character", {{CON_MENU, "4"}, {CON_DELCNF2, "yes"}, {CON_UMENU, "q"}}),
      made("menu, quitting", {{CON_MENU, "0"}}),
      made("menu, to the account menu", {{CON_MENU, "3"}, {CON_UMENU, "1"}, {CON_GET_NAME, ""}}),
      made("account menu, its options", {{CON_UMENU, "B"}, {CON_UMENU, "C"}, {CON_UMENU, "E"}, {CON_GET_EMAIL, ""}, {CON_GET_EMAIL, "M"}, {CON_UMENU, "P"}, {CON_NEWPASSWD, ""}, {CON_NEWPASSWD, "M"}, {CON_UMENU, "4"}, {CON_UMENU, "6"}, {CON_UMENU, "0"}}),
      made("account menu, a new email and password", {{CON_UMENU, "E"}, {CON_GET_EMAIL, "new@example.com"}, {CON_UMENU, "P"}, {CON_NEWPASSWD, "Newpass2"}}),
      made("account menu, deleting the account, not", {{CON_UMENU, "D"}, {CON_DELCNF1, "maybe"}, {CON_DELCNF1, "no"}}),
      made("account menu, deleting the account", {{CON_UMENU, "D"}, {CON_DELCNF1, "yes"}}),
      made("account menu, buying a slot", {{CON_UMENU, "B"}, {CON_UMENU, "4"}, {CON_GET_NAME, "Fourth"}}, {}, rich_account),
      made("account menu, buying a slot too many", {{CON_UMENU, "B"}}, {}, full_account),
      made("account menu, buying a slot, poor", {{CON_UMENU, "B"}, {CON_UMENU, "5"}}, {}, four_slot_account),
      made("account, coming back", {}, {"quit", "@leave", "@connect", "Tester", "Pa55word1", "1", "", "1"}),
      made("account, coming back twice at once", {}, {"@connect", "Tester", "Pa55word1", "1", "", "1"}),
      made("account, coming back after linkdeath", {}, {"@leave", "@pulse 2", "@connect", "Tester", "Pa55word1", "1", "", "1"}),
      made("account, coming back to delete", {}, {"quit", "@leave", "@connect", "Tester", "Pa55word1", "1", "", "4", "yes", "q"}),
      made("account, a taken name", {}, {"quit", "@leave", "@connect", "Tester", "Pa55word1", "2", "Testchar", "Otherchar"}),
      {"account, a deleted character remade", {"Tester", "Pa55word1", "1", ""}, deleted_in_slot, {}, true},
      {"account, existing, a bad password three times", {"Tester", "no", "no", "no"}, rich_account},
      {"account, existing, the password", {"Tester", "Pa55word1", "x"}, rich_account},
      /* accounts */
      {"account, nothing typed", {""}},
      {"account, a reserved name", {"index"}},
      {"account, an invalid name", {"Zz"}},
      {"account, with digits", {"Tester1"}},
      {"account, too short", {"Ab"}},
      {"account, too long", {"Abcdefghijk"}},
      {"account, a new name, refused", {"Tester", "n", "Tester", "maybe", "y"}},
      {"account, a new name, nothing confirmed", {"Tester", ""}},
      {"account, a new one", NEW_ACCOUNT},
      {"account, a new one, bad emails", {"Tester", "y", "", "nope", "has space@x.com", "tester@example.com", "short", "Pa55word1", "Different1"}},
      {"account, a new one, then back", then(NEW_ACCOUNT, {"@leave", "@connect", "Tester", "wrong", "wrong", "Pa55word1"})},
      {"account, a new one, then a bad password three times", then(NEW_ACCOUNT, {"@leave", "@connect", "Tester", "wrong", "wrong", "wrong"})},
      {"account, the menu", then(NEW_ACCOUNT, {"", "x", "P", "M", "E", "M", "C", "", "B", "1", "2", "3", "4", "5", "6"})},
      {"account, a new character's name", then(NEW_ACCOUNT, {"1", "", "!!", "Zz", "Testchar", "n", "Testchar", "y"})},
      made("chargen, a saiyan", {{CON_QRACE, "saiyan"}}),
      made("chargen, an icer", {{CON_QRACE, "icer"}}),
      made("chargen, a konatsu", {{CON_QRACE, "konatsu"}}),
      made("chargen, a namekian", {{CON_QRACE, "namekian"}, {CON_QCLASS, "piccolo"}}),
      made("chargen, a mutant", {{CON_QRACE, "mutant"}, {CON_GENOME, "3"}}),
      made("chargen, a kanassan", {{CON_QRACE, "kanassan"}, {CON_QCLASS, "tsuna"}}),
      made("chargen, a halfbreed like a human", {{CON_QRACE, "halfbreed"}, {CON_RACIAL, "1"}}),
      made("chargen, a halfbreed like a saiyan", {{CON_QRACE, "halfbreed"}, {CON_RACIAL, "2"}}),
      made("chargen, a bioandroid", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "7"}, {CON_GENOME, "7"}, {CON_GENOME, "9"}, {CON_GENOME, "3"}}),
      made("chargen, an android", {{CON_QRACE, "android"}, {CON_RACIAL, "1"}, {CON_QCLASS, "android 16"}, {CON_ANDROID, "1"}}),
      made("chargen, an android like a human", {{CON_QRACE, "android"}, {CON_RACIAL, "2"}, {CON_QCLASS, "android 16"}, {CON_ANDROID, "2"}}),
      made("chargen, an android robotic", {{CON_QRACE, "android"}, {CON_RACIAL, "3"}, {CON_QCLASS, "android 16"}, {CON_ANDROID, "3"}}),
      made("chargen, a demon", {{CON_QRACE, "demon"}, {CON_QCLASS, "dabura"}}),
      made("chargen, a majin", {{CON_QRACE, "majin"}}),
      made("chargen, a kai", {{CON_QRACE, "kai"}, {CON_QSEX, "f"}}),
      made("chargen, a truffle", {{CON_QRACE, "truffle"}, {CON_QSEX, "n"}}),
      made("chargen, a hoshijin", {{CON_QRACE, "hoshijin"}, {CON_QCLASS, "jinto"}}),
      made("chargen, an arlian", {{CON_QRACE, "arlian"}, {CON_QCLASS, "kurzak"}}),
      made("chargen, negatives then bonuses", {{CON_BONUS, "n"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "14"}, {CON_NEGATIVE, "15"}, {CON_NEGATIVE, "16"}, {CON_NEGATIVE, "17"}, {CON_NEGATIVE, "18"}, {CON_NEGATIVE, "19"}, {CON_NEGATIVE, "20"}, {CON_NEGATIVE, "21"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "23"}, {CON_NEGATIVE, "24"}, {CON_NEGATIVE, "25"}, {CON_NEGATIVE, "26"}, {CON_NEGATIVE, "27"}, {CON_NEGATIVE, "28"}, {CON_NEGATIVE, "29"}, {CON_NEGATIVE, "30"}, {CON_NEGATIVE, "31"}, {CON_NEGATIVE, "32"}, {CON_NEGATIVE, "33"}, {CON_NEGATIVE, "34"}, {CON_NEGATIVE, "35"}, {CON_NEGATIVE, "36"}, {CON_NEGATIVE, "37"}, {CON_NEGATIVE, "b"}, {CON_BONUS, "1"}, {CON_BONUS, "2"}, {CON_BONUS, "3"}, {CON_BONUS, "4"}, {CON_BONUS, "5"}, {CON_BONUS, "6"}, {CON_BONUS, "7"}, {CON_BONUS, "8"}, {CON_BONUS, "9"}, {CON_BONUS, "10"}, {CON_BONUS, "11"}, {CON_BONUS, "12"}, {CON_BONUS, "13"}, {CON_BONUS, "14"}, {CON_BONUS, "15"}, {CON_BONUS, "16"}, {CON_BONUS, "17"}, {CON_BONUS, "18"}, {CON_BONUS, "19"}, {CON_BONUS, "20"}, {CON_BONUS, "21"}, {CON_BONUS, "22"}, {CON_BONUS, "23"}, {CON_BONUS, "24"}, {CON_BONUS, "25"}, {CON_BONUS, "26"}, {CON_BONUS, "x"}}),
      made("chargen, negatives down then bonuses down", {{CON_BONUS, "n"}, {CON_NEGATIVE, "37"}, {CON_NEGATIVE, "36"}, {CON_NEGATIVE, "35"}, {CON_NEGATIVE, "34"}, {CON_NEGATIVE, "33"}, {CON_NEGATIVE, "32"}, {CON_NEGATIVE, "31"}, {CON_NEGATIVE, "30"}, {CON_NEGATIVE, "29"}, {CON_NEGATIVE, "28"}, {CON_NEGATIVE, "27"}, {CON_NEGATIVE, "26"}, {CON_NEGATIVE, "25"}, {CON_NEGATIVE, "24"}, {CON_NEGATIVE, "23"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "21"}, {CON_NEGATIVE, "20"}, {CON_NEGATIVE, "19"}, {CON_NEGATIVE, "18"}, {CON_NEGATIVE, "17"}, {CON_NEGATIVE, "16"}, {CON_NEGATIVE, "15"}, {CON_NEGATIVE, "14"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "b"}, {CON_BONUS, "26"}, {CON_BONUS, "25"}, {CON_BONUS, "24"}, {CON_BONUS, "23"}, {CON_BONUS, "22"}, {CON_BONUS, "21"}, {CON_BONUS, "20"}, {CON_BONUS, "19"}, {CON_BONUS, "18"}, {CON_BONUS, "17"}, {CON_BONUS, "16"}, {CON_BONUS, "15"}, {CON_BONUS, "14"}, {CON_BONUS, "13"}, {CON_BONUS, "12"}, {CON_BONUS, "11"}, {CON_BONUS, "10"}, {CON_BONUS, "9"}, {CON_BONUS, "8"}, {CON_BONUS, "7"}, {CON_BONUS, "6"}, {CON_BONUS, "5"}, {CON_BONUS, "4"}, {CON_BONUS, "3"}, {CON_BONUS, "2"}, {CON_BONUS, "1"}, {CON_BONUS, "x"}}),
      made("chargen, bonuses then negatives", {{CON_BONUS, "1"}, {CON_BONUS, "2"}, {CON_BONUS, "3"}, {CON_BONUS, "4"}, {CON_BONUS, "5"}, {CON_BONUS, "6"}, {CON_BONUS, "7"}, {CON_BONUS, "8"}, {CON_BONUS, "9"}, {CON_BONUS, "10"}, {CON_BONUS, "11"}, {CON_BONUS, "12"}, {CON_BONUS, "13"}, {CON_BONUS, "14"}, {CON_BONUS, "15"}, {CON_BONUS, "16"}, {CON_BONUS, "17"}, {CON_BONUS, "18"}, {CON_BONUS, "19"}, {CON_BONUS, "20"}, {CON_BONUS, "21"}, {CON_BONUS, "22"}, {CON_BONUS, "23"}, {CON_BONUS, "24"}, {CON_BONUS, "25"}, {CON_BONUS, "26"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "14"}, {CON_NEGATIVE, "15"}, {CON_NEGATIVE, "16"}, {CON_NEGATIVE, "17"}, {CON_NEGATIVE, "18"}, {CON_NEGATIVE, "19"}, {CON_NEGATIVE, "20"}, {CON_NEGATIVE, "21"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "23"}, {CON_NEGATIVE, "24"}, {CON_NEGATIVE, "25"}, {CON_NEGATIVE, "26"}, {CON_NEGATIVE, "27"}, {CON_NEGATIVE, "28"}, {CON_NEGATIVE, "29"}, {CON_NEGATIVE, "30"}, {CON_NEGATIVE, "31"}, {CON_NEGATIVE, "32"}, {CON_NEGATIVE, "33"}, {CON_NEGATIVE, "34"}, {CON_NEGATIVE, "35"}, {CON_NEGATIVE, "36"}, {CON_NEGATIVE, "37"}, {CON_NEGATIVE, "x"}}),
      made("chargen, negatives numbers past the last", {{CON_BONUS, "n"}, {CON_NEGATIVE, "38"}, {CON_NEGATIVE, "51"}, {CON_NEGATIVE, "x"}}),
      made("chargen, bonuses and their cancelling", {{CON_BONUS, "n"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "n"}, {CON_NEGATIVE, "1"}, {CON_NEGATIVE, "99"}, {CON_NEGATIVE, ""}, {CON_NEGATIVE, "b"}, {CON_BONUS, "1"}, {CON_BONUS, "1"}, {CON_BONUS, "b"}, {CON_BONUS, "30"}, {CON_BONUS, "abc"}, {CON_BONUS, ""}, {CON_BONUS, "2"}, {CON_BONUS, "x"}}),
      made("chargen, an android's bonuses", {{CON_QRACE, "android"}, {CON_QCLASS, "android 16"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "14"}, {CON_NEGATIVE, "15"}, {CON_NEGATIVE, "16"}, {CON_NEGATIVE, "17"}, {CON_NEGATIVE, "18"}, {CON_NEGATIVE, "19"}, {CON_NEGATIVE, "20"}, {CON_NEGATIVE, "21"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "23"}, {CON_NEGATIVE, "24"}, {CON_NEGATIVE, "25"}, {CON_NEGATIVE, "26"}, {CON_NEGATIVE, "27"}, {CON_NEGATIVE, "28"}, {CON_NEGATIVE, "29"}, {CON_NEGATIVE, "30"}, {CON_NEGATIVE, "31"}, {CON_NEGATIVE, "32"}, {CON_NEGATIVE, "33"}, {CON_NEGATIVE, "34"}, {CON_NEGATIVE, "35"}, {CON_NEGATIVE, "36"}, {CON_NEGATIVE, "37"}, {CON_NEGATIVE, "b"}, {CON_BONUS, "1"}, {CON_BONUS, "2"}, {CON_BONUS, "3"}, {CON_BONUS, "4"}, {CON_BONUS, "5"}, {CON_BONUS, "6"}, {CON_BONUS, "7"}, {CON_BONUS, "8"}, {CON_BONUS, "9"}, {CON_BONUS, "10"}, {CON_BONUS, "11"}, {CON_BONUS, "12"}, {CON_BONUS, "13"}, {CON_BONUS, "14"}, {CON_BONUS, "15"}, {CON_BONUS, "16"}, {CON_BONUS, "17"}, {CON_BONUS, "18"}, {CON_BONUS, "19"}, {CON_BONUS, "20"}, {CON_BONUS, "21"}, {CON_BONUS, "22"}, {CON_BONUS, "23"}, {CON_BONUS, "24"}, {CON_BONUS, "25"}, {CON_BONUS, "26"}, {CON_BONUS, "x"}}),
      made("chargen, negatives to nothing", {{CON_BONUS, "n"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "14"}, {CON_NEGATIVE, "15"}, {CON_NEGATIVE, "16"}, {CON_NEGATIVE, "17"}, {CON_NEGATIVE, "18"}, {CON_NEGATIVE, "19"}, {CON_NEGATIVE, "20"}, {CON_NEGATIVE, "21"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "23"}, {CON_NEGATIVE, "24"}, {CON_NEGATIVE, "25"}, {CON_NEGATIVE, "26"}, {CON_NEGATIVE, "27"}, {CON_NEGATIVE, "28"}, {CON_NEGATIVE, "29"}, {CON_NEGATIVE, "30"}, {CON_NEGATIVE, "31"}, {CON_NEGATIVE, "32"}, {CON_NEGATIVE, "33"}, {CON_NEGATIVE, "34"}, {CON_NEGATIVE, "35"}, {CON_NEGATIVE, "36"}, {CON_NEGATIVE, "37"}, {CON_NEGATIVE, "x"}}),
      made("chargen, answering 2", {{CON_Q1, "2"}, {CON_Q2, "2"}, {CON_Q3, "2"}, {CON_Q4, "2"}, {CON_Q5, "2"}, {CON_Q6, "2"}, {CON_Q7, "2"}, {CON_Q8, "2"}, {CON_Q9, "2"}, {CON_QX, "2"}, {CON_ALIGN, "2"}}),
      made("chargen, answering 3", {{CON_Q1, "3"}, {CON_Q2, "3"}, {CON_Q3, "3"}, {CON_Q4, "3"}, {CON_Q5, "3"}, {CON_Q6, "3"}, {CON_Q7, "3"}, {CON_Q8, "3"}, {CON_Q9, "3"}, {CON_QX, "3"}, {CON_ALIGN, "3"}}),
      made("chargen, answering 4", {{CON_Q1, "4"}, {CON_Q2, "4"}, {CON_Q3, "4"}, {CON_Q4, "4"}, {CON_Q5, "4"}, {CON_Q6, "4"}, {CON_Q7, "4"}, {CON_Q8, "4"}, {CON_Q9, "4"}, {CON_QX, "4"}, {CON_ALIGN, "4"}}),
      made("chargen, answering 5", {{CON_Q1, "5"}, {CON_Q2, "5"}, {CON_Q3, "5"}, {CON_Q4, "5"}, {CON_Q5, "5"}, {CON_Q6, "5"}, {CON_Q7, "5"}, {CON_Q8, "5"}, {CON_Q9, "5"}, {CON_QX, "5"}, {CON_ALIGN, "5"}}),
      made("chargen, answering 6", {{CON_Q1, "6"}, {CON_Q2, "6"}, {CON_Q3, "6"}, {CON_Q4, "6"}, {CON_Q5, "6"}, {CON_Q6, "6"}, {CON_Q7, "6"}, {CON_Q8, "6"}, {CON_Q9, "6"}, {CON_QX, "6"}, {CON_ALIGN, "6"}}),
      made("chargen, answering nothing", {{CON_Q1, ""}, {CON_Q2, ""}, {CON_Q3, ""}, {CON_Q4, ""}, {CON_Q5, ""}, {CON_Q6, ""}, {CON_Q7, ""}, {CON_Q8, ""}, {CON_Q9, ""}, {CON_QX, ""}, {CON_ALIGN, ""}}),
      made("chargen, the last answer a", {{CON_QX, "a"}, {CON_ALIGN, "a"}}),
      made("chargen, the last answer b", {{CON_QX, "b"}, {CON_ALIGN, "b"}}),
      made("chargen, the last answer c", {{CON_QX, "c"}, {CON_ALIGN, "c"}}),
      made("chargen, the last answer d", {{CON_QX, "d"}, {CON_ALIGN, "d"}}),
      made("chargen, the last answer e", {{CON_QX, "e"}, {CON_ALIGN, "e"}}),
      made("chargen, the last answer f", {{CON_QX, "f"}, {CON_ALIGN, "f"}}),
      made("chargen, the last answer g", {{CON_QX, "g"}, {CON_ALIGN, "g"}}),
      made("chargen, the last answer h", {{CON_QX, "h"}, {CON_ALIGN, "h"}}),
      made("chargen, the last answer i", {{CON_QX, "i"}, {CON_ALIGN, "i"}}),
      made("chargen, the last answer k", {{CON_QX, "k"}, {CON_ALIGN, "k"}}),
      made("chargen, the last answer 0", {{CON_QX, "0"}, {CON_ALIGN, "0"}}),
      made("chargen, looks 2", {{CON_HAIRL, "2"}, {CON_HAIRC, "2"}, {CON_HAIRS, "2"}, {CON_SKIN, "2"}, {CON_EYE, "2"}, {CON_DISTFEA, "2"}, {CON_HW, "2"}, {CON_AURA, "2"}}),
      made("chargen, looks 3", {{CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, looks 4", {{CON_HAIRL, "4"}, {CON_HAIRC, "4"}, {CON_HAIRS, "4"}, {CON_SKIN, "4"}, {CON_EYE, "4"}, {CON_DISTFEA, "4"}, {CON_HW, "4"}, {CON_AURA, "4"}}),
      made("chargen, looks 5", {{CON_HAIRL, "5"}, {CON_HAIRC, "5"}, {CON_HAIRS, "5"}, {CON_SKIN, "5"}, {CON_EYE, "5"}, {CON_DISTFEA, "5"}, {CON_HW, "5"}, {CON_AURA, "5"}}),
      made("chargen, looks 6", {{CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, looks 7", {{CON_HAIRL, "7"}, {CON_HAIRC, "7"}, {CON_HAIRS, "7"}, {CON_SKIN, "7"}, {CON_EYE, "7"}, {CON_DISTFEA, "7"}, {CON_HW, "7"}, {CON_AURA, "7"}}),
      made("chargen, looks 8", {{CON_HAIRL, "8"}, {CON_HAIRC, "8"}, {CON_HAIRS, "8"}, {CON_SKIN, "8"}, {CON_EYE, "8"}, {CON_DISTFEA, "8"}, {CON_HW, "8"}, {CON_AURA, "8"}}),
      made("chargen, looks 9", {{CON_HAIRL, "9"}, {CON_HAIRC, "9"}, {CON_HAIRS, "9"}, {CON_SKIN, "9"}, {CON_EYE, "9"}, {CON_DISTFEA, "9"}, {CON_HW, "9"}, {CON_AURA, "9"}}),
      made("chargen, looks a", {{CON_HAIRL, "a"}, {CON_HAIRC, "a"}, {CON_HAIRS, "a"}, {CON_SKIN, "a"}, {CON_EYE, "a"}, {CON_DISTFEA, "a"}, {CON_HW, "a"}, {CON_AURA, "a"}}),
      made("chargen, looks b", {{CON_HAIRL, "b"}, {CON_HAIRC, "b"}, {CON_HAIRS, "b"}, {CON_SKIN, "b"}, {CON_EYE, "b"}, {CON_DISTFEA, "b"}, {CON_HW, "b"}, {CON_AURA, "b"}}),
      made("chargen, looks c", {{CON_HAIRL, "c"}, {CON_HAIRC, "c"}, {CON_HAIRS, "c"}, {CON_SKIN, "c"}, {CON_EYE, "c"}, {CON_DISTFEA, "c"}, {CON_HW, "c"}, {CON_AURA, "c"}}),
      made("chargen, looks d", {{CON_HAIRL, "d"}, {CON_HAIRC, "d"}, {CON_HAIRS, "d"}, {CON_SKIN, "d"}, {CON_EYE, "d"}, {CON_DISTFEA, "d"}, {CON_HW, "d"}, {CON_AURA, "d"}}),
      made("chargen, looks z", {{CON_HAIRL, "z"}, {CON_HAIRC, "z"}, {CON_HAIRS, "z"}, {CON_SKIN, "z"}, {CON_EYE, "z"}, {CON_DISTFEA, "z"}, {CON_HW, "z"}, {CON_AURA, "z"}}),
      made("chargen, looks unanswered", {{CON_HAIRL, ""}, {CON_HAIRC, ""}, {CON_HAIRS, ""}, {CON_SKIN, ""}, {CON_EYE, ""}, {CON_DISTFEA, ""}, {CON_HW, ""}, {CON_AURA, ""}}),
      made("chargen, a woman's looks", {{CON_QSEX, "f"}, {CON_HAIRL, "1"}}),
      made("chargen, a sexless one", {{CON_QSEX, "x"}, {CON_QSEX, ""}, {CON_QSEX, "n"}}),
      made("chargen, bald", {{CON_HAIRL, "1"}}),
      made("chargen, a truffle's looks", {{CON_QRACE, "truffle"}, {CON_HW, "1"}, {CON_HW, "2"}, {CON_HW, "3"}}),
      made("chargen, a icer's looks 3", {{CON_QRACE, "icer"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a icer's looks 6", {{CON_QRACE, "icer"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, a majin's looks 3", {{CON_QRACE, "majin"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a majin's looks 6", {{CON_QRACE, "majin"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, a namekian's looks 3", {{CON_QRACE, "namekian"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a namekian's looks 6", {{CON_QRACE, "namekian"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, a arlian's looks 3", {{CON_QRACE, "arlian"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a arlian's looks 6", {{CON_QRACE, "arlian"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, a bioandroid's looks 3", {{CON_QRACE, "bioandroid"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a bioandroid's looks 6", {{CON_QRACE, "bioandroid"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, a kai's looks 3", {{CON_QRACE, "kai"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a kai's looks 6", {{CON_QRACE, "kai"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, a demon's looks 3", {{CON_QRACE, "demon"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a demon's looks 6", {{CON_QRACE, "demon"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, a konatsu's looks 3", {{CON_QRACE, "konatsu"}, {CON_HAIRL, "3"}, {CON_HAIRC, "3"}, {CON_HAIRS, "3"}, {CON_SKIN, "3"}, {CON_EYE, "3"}, {CON_DISTFEA, "3"}, {CON_HW, "3"}, {CON_AURA, "3"}}),
      made("chargen, a konatsu's looks 6", {{CON_QRACE, "konatsu"}, {CON_HAIRL, "6"}, {CON_HAIRC, "6"}, {CON_HAIRS, "6"}, {CON_SKIN, "6"}, {CON_EYE, "6"}, {CON_DISTFEA, "6"}, {CON_HW, "6"}, {CON_AURA, "6"}}),
      made("chargen, the race help", {{CON_QRACE, "t"}, {CON_RACE_HELP, "human"}, {CON_RACE_HELP, "nonsense"}, {CON_RACE_HELP, "t"}, {CON_QRACE, "r"}, {CON_QRACE, ""}, {CON_QRACE, "nonsense"}, {CON_QRACE, "saiyan"}}),
      made("chargen, the sensei help", {{CON_QCLASS, "t"}, {CON_CLASS_HELP, "roshi"}, {CON_CLASS_HELP, "nonsense"}, {CON_CLASS_HELP, "t"}, {CON_QCLASS, "r"}, {CON_QCLASS, "nonsense"}, {CON_QCLASS, "dabura"}, {CON_QCLASS, "krane"}}),
      made("chargen, of piccolo", {{CON_QCLASS, "piccolo"}}),
      made("chargen, of krane", {{CON_QCLASS, "krane"}}),
      made("chargen, of nail", {{CON_QCLASS, "nail"}}),
      made("chargen, of bardock", {{CON_QCLASS, "bardock"}}),
      made("chargen, of ginyu", {{CON_QCLASS, "ginyu"}}),
      made("chargen, of frieza", {{CON_QCLASS, "frieza"}}),
      made("chargen, of tapion", {{CON_QCLASS, "tapion"}}),
      made("chargen, of kibito", {{CON_QCLASS, "kibito"}}),
      made("chargen, forgetting the skills", {{CON_SKILLS, "maybe"}, {CON_SKILLS, "forget"}}),
      made("chargen, a mutant forgetting", {{CON_QRACE, "mutant"}, {CON_SKILLS, "forget"}, {CON_GENOME, ""}, {CON_GENOME, "99"}, {CON_GENOME, "9"}}),
      made("chargen, a mutant of 1", {{CON_QRACE, "mutant"}, {CON_GENOME, "1"}}),
      made("chargen, a mutant of 2", {{CON_QRACE, "mutant"}, {CON_GENOME, "2"}}),
      made("chargen, a mutant of 3", {{CON_QRACE, "mutant"}, {CON_GENOME, "3"}}),
      made("chargen, a mutant of 4", {{CON_QRACE, "mutant"}, {CON_GENOME, "4"}}),
      made("chargen, a mutant of 5", {{CON_QRACE, "mutant"}, {CON_GENOME, "5"}}),
      made("chargen, a mutant of 6", {{CON_QRACE, "mutant"}, {CON_GENOME, "6"}}),
      made("chargen, a mutant of 7", {{CON_QRACE, "mutant"}, {CON_GENOME, "7"}}),
      made("chargen, a mutant of 8", {{CON_QRACE, "mutant"}, {CON_GENOME, "8"}}),
      made("chargen, a mutant of 9", {{CON_QRACE, "mutant"}, {CON_GENOME, "9"}}),
      made("chargen, a mutant of 10", {{CON_QRACE, "mutant"}, {CON_GENOME, "10"}}),
      made("chargen, a bioandroid of 1", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "1"}, {CON_GENOME, "1"}, {CON_GENOME, "2"}}),
      made("chargen, a bioandroid of 2", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "2"}, {CON_GENOME, "2"}, {CON_GENOME, "3"}}),
      made("chargen, a bioandroid of 3", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "3"}, {CON_GENOME, "3"}, {CON_GENOME, "4"}}),
      made("chargen, a bioandroid of 4", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "4"}, {CON_GENOME, "4"}, {CON_GENOME, "5"}}),
      made("chargen, a bioandroid of 5", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "5"}, {CON_GENOME, "5"}, {CON_GENOME, "6"}}),
      made("chargen, a bioandroid of 6", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "6"}, {CON_GENOME, "6"}, {CON_GENOME, "7"}}),
      made("chargen, a bioandroid of 7", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "7"}, {CON_GENOME, "7"}, {CON_GENOME, "8"}}),
      made("chargen, a bioandroid of 8", {{CON_QRACE, "bioandroid"}, {CON_GENOME, "8"}, {CON_GENOME, "8"}, {CON_GENOME, "1"}}),
      made("chargen, a bioandroid choosing oddly", {{CON_QRACE, "bioandroid"}, {CON_GENOME, ""}, {CON_GENOME, "0"}, {CON_GENOME, "9"}}),
      made("chargen, an android answering nothing", {{CON_QRACE, "android"}, {CON_RACIAL, ""}, {CON_QCLASS, "android 16"}, {CON_ANDROID, ""}}),
      made("chargen, an android answering 4", {{CON_QRACE, "android"}, {CON_RACIAL, "4"}, {CON_QCLASS, "android 16"}, {CON_ANDROID, "4"}}),
      made("chargen, an android answering x", {{CON_QRACE, "android"}, {CON_RACIAL, "x"}, {CON_QCLASS, "android 16"}, {CON_ANDROID, "x"}}),
      made("chargen, a halfbreed answering oddly", {{CON_QRACE, "halfbreed"}, {CON_RACIAL, "3"}, {CON_RACIAL, ""}}),
      made("chargen, trait 0 against 28", {{CON_BONUS, "n"}, {CON_BONUS, "1"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "14"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 2 against 40", {{CON_BONUS, "n"}, {CON_BONUS, "3"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "26"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 6 against 39", {{CON_BONUS, "n"}, {CON_BONUS, "7"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "25"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 8 against 50", {{CON_BONUS, "n"}, {CON_BONUS, "9"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "36"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 9 against 43", {{CON_BONUS, "n"}, {CON_BONUS, "10"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "29"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 10 against 44", {{CON_BONUS, "n"}, {CON_BONUS, "11"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "30"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 11 against 45", {{CON_BONUS, "n"}, {CON_BONUS, "12"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "31"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 12 against 46", {{CON_BONUS, "n"}, {CON_BONUS, "13"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "32"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 13 against 47", {{CON_BONUS, "n"}, {CON_BONUS, "14"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "33"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 14 against 48", {{CON_BONUS, "n"}, {CON_BONUS, "15"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "34"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 15 against 33", {{CON_BONUS, "n"}, {CON_BONUS, "16"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "19"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 17 against 34", {{CON_BONUS, "n"}, {CON_BONUS, "18"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "20"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 18 against 26", {{CON_BONUS, "n"}, {CON_BONUS, "19"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 19 against 29", {{CON_BONUS, "n"}, {CON_BONUS, "20"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "15"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 20 against 27", {{CON_BONUS, "n"}, {CON_BONUS, "21"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 21 against 30", {{CON_BONUS, "n"}, {CON_BONUS, "22"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "16"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 22 against 31", {{CON_BONUS, "n"}, {CON_BONUS, "23"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "17"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 23 against 32", {{CON_BONUS, "n"}, {CON_BONUS, "24"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "18"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 24 against 35", {{CON_BONUS, "n"}, {CON_BONUS, "25"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "21"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 25 against 51", {{CON_BONUS, "n"}, {CON_BONUS, "26"}, {CON_BONUS, "x"}, {CON_NEGATIVE, "37"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}}),
      made("chargen, trait 26 against 18", {{CON_BONUS, "n"}, {CON_BONUS, "19"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "12"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 27 against 20", {{CON_BONUS, "n"}, {CON_BONUS, "21"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "13"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 29 against 19", {{CON_BONUS, "n"}, {CON_BONUS, "20"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "15"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 30 against 21", {{CON_BONUS, "n"}, {CON_BONUS, "22"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "16"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 31 against 22", {{CON_BONUS, "n"}, {CON_BONUS, "23"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "17"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 32 against 23", {{CON_BONUS, "n"}, {CON_BONUS, "24"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "18"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 33 against 15", {{CON_BONUS, "n"}, {CON_BONUS, "16"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "19"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 34 against 17", {{CON_BONUS, "n"}, {CON_BONUS, "18"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "20"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 35 against 24", {{CON_BONUS, "n"}, {CON_BONUS, "25"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "21"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 39 against 6", {{CON_BONUS, "n"}, {CON_BONUS, "7"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "25"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 40 against 2", {{CON_BONUS, "n"}, {CON_BONUS, "3"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "26"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 43 against 9", {{CON_BONUS, "n"}, {CON_BONUS, "10"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "29"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 44 against 10", {{CON_BONUS, "n"}, {CON_BONUS, "11"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "30"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 45 against 11", {{CON_BONUS, "n"}, {CON_BONUS, "12"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "31"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 46 against 12", {{CON_BONUS, "n"}, {CON_BONUS, "13"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "32"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 47 against 13", {{CON_BONUS, "n"}, {CON_BONUS, "14"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "33"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 48 against 14", {{CON_BONUS, "n"}, {CON_BONUS, "15"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "34"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 50 against 8", {{CON_BONUS, "n"}, {CON_BONUS, "9"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "36"}, {CON_NEGATIVE, "x"}}),
      made("chargen, trait 51 against 25", {{CON_BONUS, "n"}, {CON_BONUS, "26"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_NEGATIVE, "37"}, {CON_NEGATIVE, "x"}}),
      made("chargen, an android's barred traits", {{CON_QRACE, "android"}, {CON_QCLASS, "android 16"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_BONUS, "4"}, {CON_BONUS, "17"}, {CON_BONUS, "x"}}),
      made("chargen, a demon's barred trait", {{CON_QRACE, "demon"}, {CON_QCLASS, "dabura"}, {CON_BONUS, "n"}, {CON_NEGATIVE, "22"}, {CON_NEGATIVE, "b"}, {CON_BONUS, "18"}, {CON_BONUS, "x"}}),
      {"chargen, a human of roshi", then(NEW_ACCOUNT, {"1", "Testchar", "human", "m", "1", "1", "1", "1", "1", "1", "1", "1", "roshi", "1", "1", "1", "1", "1", "1", "1", "1", "1", "1", "1", "keep", "x", "", "", "", "1", ""})},
  };

  unsigned long seed_for(const std::string &what)
  {
    unsigned long h = 5381;
    for (char c : what)
      h = h * 33 + (unsigned char)c;
    return h;
  }

  /* One client: the game's end of the connection and the client's */
  struct client {
    struct descriptor_data *d{};
    int peer = -1;
  };

  /* What the game has sent the client */
  std::string received(GameWorld *g, client &c)
  {
    std::string got;
    char buffer[32768];
    ssize_t length;
    if (!c.d || c.peer < 0)
      return got;
    if (*c.d->output && game_send_output(g, c.d) < 0)
      return got;
    while ((length = read(c.peer, buffer, sizeof(buffer))) > 0)
      got.append(buffer, length);
    return got;
  }

  bool still_connected(GameWorld *g, const client &c)
  {
    for (descriptor_data *d : each_desc(g))
      if (d == c.d)
        return true;
    return false;
  }

  void connect(GameWorld *g, client &c)
  {
    c.d = th_connect(g, &c.peer);
    file_print(stdout, "@connect\n{}\n", received(g, c));
  }

  void leave(GameWorld *g, client &c)
  {
    if (c.peer >= 0)
      close(c.peer);
    c.peer = -1;
    th_pulses(g, 1); /* the game notices the hang-up */
    c.d = nullptr;
    file_print(stdout, "@leave\n");
  }

  /* A hash of a file's text, to show what was saved without the whole of it */
  unsigned long hash_of(const std::string &text)
  {
    unsigned long h = 14695981039346656037UL;
    for (char c : text)
      h = (h ^ (unsigned char)c) * 1099511628211UL;
    return h;
  }

  /* Every file under dir, with its length and hash, in name order */
  void list_files(const std::string &dir)
  {
    DIR *d = opendir(dir.c_str());
    if (!d)
      return;
    std::vector<std::string> names;
    while (struct dirent *e = readdir(d))
      if (e->d_name[0] != '.')
        names.push_back(e->d_name);
    closedir(d);
    std::sort(names.begin(), names.end());
    for (const std::string &name : names) {
      std::string path = dir + "/" + name;
      DIR *sub         = opendir(path.c_str());
      if (sub) {
        closedir(sub);
        list_files(path);
        continue;
      }
      std::string text = th_file_text(path.c_str());
      file_print(stdout, "== file {}: {} bytes, hash {:x}\n", path, text.size(), hash_of(text));
    }
  }

  void audit_problem(void *, const char *problem) { file_print(stdout, "AUDIT: {}\n", problem); }

  /* The next answer for this state: the journey's own in turn, then the defaults in turn, then "1" */
  const char *answer_for(const journey &j, int state, const std::vector<size_t> &used)
  {
    size_t n = used[state];
    for (const answers *list : {&j.by_state, &DEFAULTS})
      for (const auto &[st, text] : *list)
        if (st == state && n-- == 0)
          return text;
    return "1";
  }

  void type(GameWorld *g, client &c, const char *line)
  {
    std::string typed = std::string(line) + "\r\n";
    bool open         = th_send(g, c.d, c.peer, typed.data(), typed.size());
    file_print(stdout, "> {}\n{}", line, open ? received(g, c) : std::string("(closed)\n"));
    if (open && still_connected(g, c))
      file_print(stdout, "\n[state {:d}]\n", desc_get_connected(c.d));
  }

  /* Answers by state, until in the game, gone, or the same answer has gone round too long */
  void drive(GameWorld *g, client &c, const journey &j)
  {
    std::vector<size_t> used(256);
    int last = -1, repeats = 0;
    for (int step = 0; step < 200 && c.d && still_connected(g, c); step++) {
      int state = desc_get_connected(c.d);
      if (state == CON_PLAYING)
        return;
      repeats = state == last ? repeats + 1 : 0;
      last    = state;
      if (repeats > 40) {
        file_print(stdout, "(stuck in state {:d})\n", state);
        return;
      }
      const char *line = answer_for(j, state, used);
      used[state]++;
      type(g, c, line);
    }
  }

  void play(GameWorld *g, const journey &j, unsigned long seed)
  {
    circle_srandom(g, seed);
    if (j.setup)
      j.setup(g);
    client c;
    connect(g, c);
    auto lines = [&](const std::vector<const char *> &list) {
    for (const char *line : list) {
      if (!strcmp(line, "@connect")) {
        connect(g, c);
        continue;
      }
      if (!strcmp(line, "@leave")) {
        leave(g, c);
        continue;
      }
      if (!strncmp(line, "@pulse ", 7)) {
        th_pulses(g, atoi(line + 7));
        file_print(stdout, "{}\n", line);
        continue;
      }
      if (!c.d || !still_connected(g, c)) {
        file_print(stdout, "> {}\n(closed)\n", line);
        continue;
      }
      type(g, c, line);
    }
    };
    lines(j.lines);
    if (j.drive)
      drive(g, c, j);
    lines(j.after);
    file_print(stdout, "rng {:d}\n", g->rng_seed);
    list_files("user");
    list_files("plrfiles");
    if (world_audit(g, audit_problem, nullptr))
      th_exit(3);
  }

  void run(GameWorld *g, const journey &j)
  {
    char report[64];
    int status;
    snprintf(report, sizeof(report), "case-%ld.txt", (long)getpid());
    cases++;
    fflush(nullptr);
    std::string own = str_format("{}/dbat-login-{:d}-{:d}", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", getpid(), cases);
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(2);
    }
    if (pid == 0) {
      /* each journey plays in its own copy of the world, so the files one saves aren't there for the next */
      std::string copy = str_format("mkdir -p '{}/data' && cp -r . '{}/data'", own, own);
      if (system(copy.c_str()) != 0 || chdir((own + "/data").c_str()) < 0)
        th_exit(2);
      int fd = open(report, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      alarm(CASE_SECONDS);
      play(g, j, seed_for(j.name));
      th_exit(0);
    }
    waitpid(pid, &status, 0);
    std::string report_path = own + "/data/" + report;
    std::string text        = th_file_text(report_path.c_str());
    std::string cleanup     = str_format("rm -rf '{}'", own);
    if (system(cleanup.c_str()) != 0)
      perror("rm");
    int outcome = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
    if (transcript) {
      file_print(transcript, "######## {} | {:d}\n{}", j.name, outcome, text);
      fflush(transcript);
    }
    if (outcome) {
      failures++;
      file_print(stdout, "FAIL ({:d}): {}\n{}", outcome, j.name, text);
    }
  }
} // namespace

int main(int argc, char **argv)
{
  int shard = 0, shards = 1, arg = 1;

  while (arg < argc && !strncmp(argv[arg], "--", 2)) {
    if (!strcmp(argv[arg], "--transcript") && arg + 1 < argc) {
      if (!(transcript = fopen(argv[arg + 1], "w"))) {
        perror("--transcript");
        return 2;
      }
      arg += 2;
    } else if (!strcmp(argv[arg], "--shard") && arg + 1 < argc && sscanf(argv[arg + 1], "%d/%d", &shard, &shards) == 2 && shards > 0 && shard >= 0 &&
               shard < shards) {
      arg += 2;
    } else {
      return 2;
    }
  }
  const char *only = argc > arg ? argv[arg] : nullptr;

  GameWorld *g = th_boot_at(TRANSCRIPT_TIME);
  int i        = 0;
  for (const journey &j : JOURNEYS) {
    if (i++ % shards != shard || (only && strncmp(j.name, only, strlen(only))))
      continue;
    run(g, j);
  }
  if (transcript)
    fclose(transcript);
  file_print(stdout, "{:d} cases, {:d} failed\n", cases, failures);
  th_shutdown(g);
  return failures != 0;
}
