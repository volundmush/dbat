/*
 * Players made of dice: a few bots wander the fixture world typing commands, checking the world audit
 * after every action. A run is a list of actions, written to a journal as it goes, and replaying that
 * journal repeats the run exactly. So any failure can be rerun, shrunk (tools/shrink.py), and kept in
 * tests/corpus/ as a regression test.
 *
 * test_bot [first seed] [seeds] [steps]        each seed in its own process, and replays one journal
 *                                              to check that a run really repeats
 * test_bot --seed N [steps] [--journal FILE]   one seed here, showing the full report
 * test_bot --replay FILE                       one journal here, showing the full report
 * test_bot --corpus DIR                        every journal in DIR, each in its own process
 * test_bot --world DIR ...                     any of those, playing a copy of the world in DIR
 *
 * Journal lines:
 *   seed N                    seeds the game's generator; comes first
 *   spawn LABEL LEVEL DICE RACE SENSEI   logs a new player in, named LABEL; DICE rolls their kit
 *   login LABEL NAME          logs in a copy of a player with a save file
 *   connect LABEL             a client connects, the real way, and starts at the login menu
 *   type LABEL TEXT           LABEL's connection types TEXT; ignored if LABEL has gone
 *   clock SECONDS             the clock jumps forward
 *   leave LABEL               LABEL's connection hangs up
 *   pulse N                   N pulses of the game
 * Any of them can start with @N, which seeds the game's generator for that line alone. The bot gives one
 * to every line, so that removing a line doesn't change the rolls of all the lines after it, which is
 * what lets a journal shrink.
 */
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "harness.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/fight.h"
#include "dbat/players.h"
#include "dbat/room_utils.h"
#include "dbat/shop.h"
#include "dbat/object_utils.h"
#include "dbat/index.h"
#include "dbat/dg_scripts.h"
#include "dbat/zones.h"
#include "dbat/handler.h"
#include "dbat/descriptor.h"
#include "dbat/interpreter.h"
#include "dbat/random.h"
#include "dbat/utils.h"
#include "dbat/world_audit.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

constexpr int NUM_BOTS        = 4;
constexpr int NUM_SENSEIS     = 14; /* the ones players can have: the rest of the table is unused */
constexpr int MAX_CONNECTIONS = 64;
constexpr int STEP_SECONDS    = 10;
constexpr int AUDIT_FAILED    = 3;
constexpr time_t START_TIME   = 1800000000;
constexpr int MAX_SIGNATURES  = 100;
constexpr size_t MAX_LINE     = MAX_INPUT_LENGTH + 64;

/*
 * Bugs nobody has fixed yet, by a line of their report. They still get reported, but don't fail the
 * test, so a new failure stands out. tests/corpus/open/ is the place for a short journal of each: when
 * one is fixed, remove its line here and move its journal up into tests/corpus/.
 */
static const char *const known_failures[] = {
    "(none at the moment)",
};

static const char *const never_typed[] = {
    "copyover", /* replaces the process */
    "shutdown", /* ends the game on purpose */
};

/* What players mostly do, so that the bots get somewhere instead of only trying the odd commands. */
static const char *const everyday[] = {
    "north", "east", "south", "west", "look", "get", "drop", "give", "put", "wear", "remove", "wield",
    "kill", "punch", "flee", "follow", "sit", "stand", "sleep", "wake", "rest", "say", "tell", "open",
    "close", "eat", "buy", "sell", "list", "inventory", "equipment", "score", "deposit", "withdraw",
    "balance", "group", "steal", "sacrifice", "junk", "save", "quit",
};

static const char *const words[] = {
    "dummy", "keeper", "stranger", "chest", "sword", "bread", "atm", "bot0", "bot1", "bot2", "bot3", "self",
    "all", "all.sword", "2.dummy", "north", "east", "south", "west", "1", "2", "100", "300", "301", "302",
    "303", "3034", "-1", "0", "on", "off", "xyzzy", "%s%n", "$n", "thing", "thing5", "thing9", "thing15", "thing17", "thing24",
    "thing27", "thing28", "thing30", "thing31", "thing32", "critter1", "critter5", "critter10", "vanisher", "guildmaster",
    "hello", "dance", "up", "down", "here", "me", "@x", "$N",
};

/* Editors are entered with these, by Implementors, and bots stay in them for a while. */
static const char *const builder_commands[] = {
    "redit 300", "redit 310", "redit 399", "oedit 300", "oedit 311", "oedit 398", "medit 300", "medit 310",
    "medit 398", "zedit 3", "zedit new 5", "sedit 300", "sedit 398", "gedit 300", "gedit 398", "trigedit 300",
    "trigedit 340", "trigedit 398", "aedit nod", "aedit newsocial", "hedit fixture", "hedit newhelp", "cedit",
    "tedit news", "tedit motd", "assedit 300", "hsedit 300", "iedit sword", "iedit chest", "rlist", "olist",
    "mlist", "slist", "glist", "tlist", "zlist",
};

static const char *const editor_exits[] = {"q", "y", "n", "/s", "/q", "/c", "/h", "x"};

/* For the login menus. Some of these match each other often enough to get all the way in. */
static const char *const login_answers[] = {
    "Botuser", "Botuser", "Otheruser", "y", "y", "n", "Pa55word1", "Pa55word1", "Pa55word1", "bot@example.com", "1", "1", "2", "3", "4", "5",
    "Zarbot", "Qorbot", "Yes", "No", "", "", "m", "b", "x", "new", "delete", "cancel",
};

#define PICK(list) ((list)[bot_dice(sizeof(list) / sizeof(*(list)))])

/*
 * The bots' own dice, so that choosing what to do takes nothing from the game's generator. Only
 * choosing uses them, never doing, which is why a journal repeats without them.
 */
static unsigned long dice_state;

static unsigned long roll(unsigned long *state, unsigned long sides)
{
  *state = *state * 6364136223846793005UL + 1442695040888963407UL;
  return (*state >> 33) % sides;
}

static unsigned long bot_dice(unsigned long sides)
{
  return roll(&dice_state, sides);
}

static bool is_never_typed(const char *command)
{
  for (size_t i = 0; i < sizeof(never_typed) / sizeof(*never_typed); i++)
    if (!strcmp(command, never_typed[i]))
      return true;
  return false;
}

/* Who is connected, by the label the journal knows them by. */
struct connection {
  char label[16];
  struct descriptor_data *d;
  int peer; /* the client's end of its socket, or -1 for a player made by spawn */
};

static struct connection connections[MAX_CONNECTIONS];
static int num_connections = 0;

/* Forgets connections the game has closed, which it can do at any moment. */
static void forget_closed_connections(const GameWorld *g)
{
  int kept = 0;

  for (int i = 0; i < num_connections; i++) {
    for (const struct descriptor_data *d = g->descriptors; d; d = d->next)
      if (d == connections[i].d) {
        connections[kept++] = connections[i];
        goto next;
      }
    if (connections[i].peer >= 0)
      close(connections[i].peer);
  next:;
  }
  num_connections = kept;
}

static struct connection *find_connection(const char *label)
{
  for (int i = 0; i < num_connections; i++)
    if (!strcmp(connections[i].label, label))
      return &connections[i];
  return nullptr;
}

/* A low-level character fails most commands at the first check, so give them the means to get further. */
static void equip_bot(GameWorld *g, struct char_data *ch, unsigned long kit)
{
  char_set_class_level(ch, 1 + roll(&kit, 100));
  char_set_gold(ch, 100000);
  char_set_basepl(ch, 1000000);
  char_set_baseki(ch, 1000000);
  char_set_basest(ch, 1000000);
  char_set_sex(ch, roll(&kit, 3));
  for (int skill = 0; skill < SKILL_TABLE_SIZE; skill++)
    if (roll(&kit, 2))
      char_set_skills_base(ch, skill, 1 + roll(&kit, 100));
  for (unsigned long n = roll(&kit, 4); n > 0; n--)
    char_aff_set(ch, roll(&kit, NUM_AFF_FLAGS), true);
  for (unsigned long n = roll(&kit, 4); n > 0; n--)
    char_prf_set(g, ch, roll(&kit, NUM_PRF_FLAGS), true);
  restore(g, ch, false);
  for (unsigned long n = roll(&kit, 8) + 2; n > 0; n--)
    obj_to_char(g, read_object(g, roll(&kit, g->top_obj + 1), REAL), ch);
  char_from_room(g, ch);
  char_to_room(g, ch, roll(&kit, g->top_room + 1));
}

/* A running fingerprint of all the output, to compare two runs. */
static unsigned long fingerprint = 14695981039346656037UL;

static void fingerprint_bytes(const char *bytes, size_t length)
{
  for (size_t i = 0; i < length; i++)
    fingerprint = (fingerprint ^ (unsigned char)bytes[i]) * 1099511628211UL;
}

/* A connection's output goes through the game's real output code, and the client reads it. */
static void send_real_output(GameWorld *g, struct connection *c)
{
  static char received[32768];
  ssize_t length;

  if (*c->d->output && game_send_output(g, c->d) >= 0)
    while ((length = read(c->peer, received, sizeof(received))) > 0)
      fingerprint_bytes(received, length);
}

static void take_all_output(GameWorld *g)
{
  forget_closed_connections(g);
  for (int i = 0; i < num_connections; i++)
    if (connections[i].peer >= 0)
      send_real_output(g, &connections[i]);

  for (struct descriptor_data *d = g->descriptors; d; d = d->next) {
    fingerprint_bytes(d->output, strlen(d->output));
    th_take_output_at(g, d);
  }
}

static void print_audit_problem(void *, const char *problem)
{
  printf("AUDIT: %s\n", problem);
}

/*
 * The game reports much of what it notices going wrong in its log. Each kind of message is printed once
 * per run for tools/hunt.py to collect. With DBAT_SYSERR set to one of those messages, it ends the run
 * there instead, so that tools/shrink.py can cut a journal down to whatever causes it.
 */
static void simplify_message(char *text)
{
  char *out = text;

  for (const char *in = text; *in; in++) {
    if (in[0] == '0' && in[1] == 'x') { /* an address */
      in += 2;
      while (isxdigit((unsigned char)in[1]))
        in++;
      *out++ = '#';
    } else if (isdigit((unsigned char)*in)) {
      while (isdigit((unsigned char)in[1]))
        in++;
      *out++ = '#';
    } else
      *out++ = *in;
  }
  *out = '\0';
}

static void check_log(void)
{
  static FILE *log_reader;
  static char seen[128][160];
  static int num_seen;
  const char *fatal = getenv("DBAT_SYSERR");
  char text[1024];

  /* What goes wrong during boot is mostly what a tiny world lacks, so start from the end of that. */
  if (!log_reader) {
    if (!(log_reader = fopen("test.log", "r")))
      return;
    fseek(log_reader, 0, SEEK_END);
  }
  while (fgets(text, sizeof(text), log_reader)) {
    char *message = strstr(text, "SYSERR");
    bool is_new = true;

    if (!message)
      continue;
    if (strstr(message, "Creating new guild")) /* not an error, despite how it's logged */
      continue;
    message[strcspn(message, "\n")] = '\0';
    simplify_message(message);
    message[159] = '\0';
    for (int i = 0; i < num_seen && is_new; i++)
      is_new = strcmp(seen[i], message) != 0;
    if (is_new && num_seen < 128)
      snprintf(seen[num_seen++], sizeof(seen[0]), "%s", message);
    if (is_new)
      printf("LOGGED: %s\n", message);
    if (fatal && !strcmp(fatal, message)) {
      printf("AUDIT: logged: %s\n", message);
      th_exit(AUDIT_FAILED);
    }
  }
  clearerr(log_reader);
}

static time_t clock_jumps = 0;

static bool player_in_game(const GameWorld *g, const char *name)
{
  for (const struct char_data *ch = g->characters; ch; ch = char_get_next(ch))
    if (!char_is_npc(ch) && !strcasecmp(char_get_name(ch), name))
      return true;
  return false;
}

static void add_connection(const char *label, struct descriptor_data *d, int peer)
{
  snprintf(connections[num_connections].label, sizeof(connections[num_connections].label), "%s", label);
  connections[num_connections].d      = d;
  connections[num_connections++].peer = peer;
}

/* Does one journal line, whether the bot just chose it or a journal is being replayed. */
static void do_action(GameWorld *g, char *line)
{
  char *rest;

  alarm(STEP_SECONDS);
  forget_closed_connections(g);

  if (*line == '@') {
    circle_srandom(g, strtoul(line + 1, &rest, 10) + 1); /* a seed of 0 would stick on one value */
    line = rest + (*rest == ' ');
  }

  if (!strncmp(line, "seed ", 5)) {
    circle_srandom(g, strtoul(line + 5, nullptr, 10));
  } else if (!strncmp(line, "spawn ", 6)) {
    char label[16];
    int level, race, sensei;
    unsigned long kit;

    if (sscanf(line + 6, "%15s %d %lu %d %d", label, &level, &kit, &race, &sensei) == 5 && num_connections < MAX_CONNECTIONS) {
      struct char_data *ch = th_player_as(g, label, level, race, sensei);

      equip_bot(g, ch, kit);
      add_connection(label, char_get_desc(ch), -1);
    }
  } else if (!strncmp(line, "login ", 6)) {
    char label[16], name[64];
    struct char_data *ch;

    /* the game itself won't load a player who's already in it */
    if (sscanf(line + 6, "%15s %63s", label, name) == 2 && num_connections < MAX_CONNECTIONS && !player_in_game(g, name) &&
        (ch = th_existing_player(g, name)))
      add_connection(label, char_get_desc(ch), -1);
  } else if (!strncmp(line, "connect ", 8)) {
    int peer;
    struct descriptor_data *d;

    if (num_connections < MAX_CONNECTIONS && (d = th_connect(g, &peer)))
      add_connection(line + 8, d, peer);
  } else if (!strncmp(line, "type ", 5)) {
    struct connection *c;

    rest = strchr(line + 5, ' ');
    if (rest) {
      *rest++ = '\0';
      if ((c = find_connection(line + 5))) {
        if (c->peer < 0)
          th_type_at(g, c->d, rest);
        else {
          size_t length = strlen(rest);

          rest[length] = '\n'; /* there's room: the line was read from a larger buffer */
          th_send(g, c->d, c->peer, rest, length + 1);
          rest[length] = '\0';
        }
      }
    }
  } else if (!strncmp(line, "leave ", 6)) {
    struct connection *c = find_connection(line + 6);

    if (c)
      close_socket(g, c->d); /* what the game does when a connection hangs up */
  } else if (!strncmp(line, "clock ", 6)) {
    clock_jumps += strtol(line + 6, nullptr, 10);
    g->pinned_time = START_TIME + clock_jumps + (time_t)(g->pulse / PASSES_PER_SEC);
  } else if (!strncmp(line, "pulse ", 6)) {
    th_pulses(g, atoi(line + 6));
    g->pinned_time = START_TIME + clock_jumps + (time_t)(g->pulse / PASSES_PER_SEC);
  }

  take_all_output(g);
  check_log();
  if (world_audit(g, print_audit_problem, nullptr))
    th_exit(AUDIT_FAILED);
}

/* The first word of a keyword list, which is how a player would name the thing. */
static const char *first_keyword(const char *keywords)
{
  static char word[64];

  snprintf(word, sizeof(word), "%s", keywords ? keywords : "something");
  word[strcspn(word, " ")] = '\0';
  return word;
}

/* Something this bot could sensibly mean: what's around it, what it holds, or a number from the world. */
static const char *pick_word(GameWorld *g, const struct char_data *ch)
{
  static char number[16];

  switch (ch ? bot_dice(6) : 2 + bot_dice(4)) {
    case 0: {
      int count = 0, chosen;
      struct char_data *other;

      if (char_get_in_room(ch) == NOWHERE)
        break;
      for (other = rroom_get_people(g, char_get_in_room(ch)); other; other = char_get_next_in_room(other))
        count++;
      chosen = bot_dice(count ? count : 1);
      for (other = rroom_get_people(g, char_get_in_room(ch)); other; other = char_get_next_in_room(other))
        if (chosen-- == 0)
          return first_keyword(char_is_npc(other) ? char_get_alias(other) : char_get_name(other));
      break;
    }
    case 1: {
      int count = 0, chosen;
      struct obj_data *obj;
      struct obj_data *list = char_get_in_room(ch) != NOWHERE && bot_dice(2) ? rroom_get_contents(g, char_get_in_room(ch)) : char_get_inventory(ch);

      for (obj = list; obj; obj = obj_get_next_content(obj))
        count++;
      chosen = bot_dice(count ? count : 1);
      for (obj = list; obj; obj = obj_get_next_content(obj))
        if (chosen-- == 0)
          return first_keyword(obj_get_name(obj));
      break;
    }
    case 2:
      snprintf(number, sizeof(number), "%d", rroom_get_vnum(g, bot_dice(g->top_room + 1)));
      return number;
    case 3:
      snprintf(number, sizeof(number), "%d", g->obj_index[bot_dice(g->top_obj + 1)].vnum);
      return number;
    case 4:
      snprintf(number, sizeof(number), "%d", g->mob_index[bot_dice(g->top_mob + 1)].vnum);
      return number;
    default:
      break;
  }
  return PICK(words);
}

/* Mostly short, sometimes as long as the game allows: buffers only overflow on long input. */
static size_t append_word(GameWorld *g, const struct char_data *ch, char *text, size_t used, size_t size)
{
  static const size_t lengths[] = {256, 1024, 4096, MAX_INPUT_LENGTH - 8};

  used += snprintf(text + used, size - used, " ");
  if (used < size && !bot_dice(40)) {
    size_t length = lengths[bot_dice(sizeof(lengths) / sizeof(*lengths))];
    char fill = bot_dice(2) ? 'a' : '%';

    while (length-- > 0 && used + 1 < size)
      text[used++] = fill;
    text[used] = '\0';
    return used;
  }
  return used + snprintf(text + used, size - used, "%s", pick_word(g, ch));
}

static void compose_for_editor(GameWorld *g, char *text, size_t size)
{
  if (!bot_dice(3)) {
    size_t used = snprintf(text, size, "%s", pick_word(g, nullptr));

    if (!bot_dice(4))
      append_word(g, nullptr, text, used, size);
  } else if (!bot_dice(40))
    snprintf(text, size, "%s", PICK(editor_exits));
  else if (bot_dice(2))
    snprintf(text, size, "%lu", bot_dice(40));
  else
    snprintf(text, size, "%c", (char)('a' + bot_dice(26)));
}

static void compose_for_login(char *text, size_t size)
{
  if (bot_dice(3))
    snprintf(text, size, "%s", PICK(login_answers));
  else if (bot_dice(2))
    snprintf(text, size, "%lu", bot_dice(30));
  else
    snprintf(text, size, "%c", (char)('a' + bot_dice(26)));
}

/* Opens an editor on something that exists, or something new. Only Implementors will get anywhere. */
static void compose_builder_command(GameWorld *g, char *text, size_t size)
{
  static const char *const with_room[] = {"redit", "goto", "rstat"};
  static const char *const with_object[] = {"oedit", "ostat", "load obj", "vnum obj"};
  static const char *const with_mob[] = {"medit", "mstat", "load mob", "vnum mob"};

  switch (bot_dice(7)) {
    case 0:
      snprintf(text, size, "%s %d", PICK(with_room), rroom_get_vnum(g, bot_dice(g->top_room + 1)));
      break;
    case 1:
      snprintf(text, size, "%s %d", PICK(with_object), g->obj_index[bot_dice(g->top_obj + 1)].vnum);
      break;
    case 2:
      snprintf(text, size, "%s %d", PICK(with_mob), g->mob_index[bot_dice(g->top_mob + 1)].vnum);
      break;
    case 3:
      snprintf(text, size, "zedit %d", g->zones[bot_dice(g->top_zone + 1)].number);
      break;
    case 4:
      if (g->top_shop >= 0) {
        snprintf(text, size, "sedit %d", rshop_get_vnum(g, bot_dice(g->top_shop + 1)));
        break;
      }
      [[fallthrough]];
    case 5:
      if (g->top_of_trigt > 0) {
        snprintf(text, size, "trigedit %d", g->trig_index[bot_dice(g->top_of_trigt)]->vnum);
        break;
      }
      [[fallthrough]];
    default:
      snprintf(text, size, "%s", PICK(builder_commands));
  }
}

static void compose_typing(GameWorld *g, const struct descriptor_data *d, char *text, size_t size)
{
  const char *command;
  size_t used;
  int num_commands = 0;

  if (desc_get_connected(d) != CON_PLAYING || d->str) {
    if (!d->character || !char_get_name(d->character) || desc_get_connected(d) == CON_MENU)
      compose_for_login(text, size);
    else
      compose_for_editor(g, text, size);
    return;
  }

  switch (bot_dice(10)) {
    case 0:
    case 1:
    case 2:
    case 3:
      command = PICK(everyday);
      break;
    case 4:
      compose_builder_command(g, text, size);
      return;
    default:
      while (g->complete_cmd_info[num_commands].command[0] != '\n')
        num_commands++;
      do {
        command = g->complete_cmd_info[bot_dice(num_commands - 1) + 1].command;
      } while (!strcmp(command, "RESERVED"));
  }
  if (is_never_typed(command))
    command = "look";

  used = snprintf(text, size, "%s", command);
  for (unsigned long n = bot_dice(4); n > 0 && used < size; n--)
    used = append_word(g, d->character, text, used, size);
}

/* Only a connection still at the login menus has a nameless character, or none in the game. */
static bool at_login_menu(const struct descriptor_data *d)
{
  return !d->character || !char_get_name(d->character);
}

/* Only a real world has players worth logging in as: the fixture's are other bots. */
static bool use_real_players = false;
static FILE *journal;
static char line[MAX_LINE + 2];

static void journal_and_do(GameWorld *g)
{
  static char with_dice[MAX_LINE + 34];

  snprintf(with_dice, MAX_LINE, "@%lu %s", bot_dice(1000000000), line);
  fprintf(journal, "%s\n", with_dice);
  fflush(journal);
  do_action(g, with_dice);
}

/* Chooses what happens next. */
static void bot_step(GameWorld *g, int *next_name)
{
  struct connection *c;

  forget_closed_connections(g);
  while (num_connections < NUM_BOTS) {
    int number = (*next_name)++;

    /* in a world with real players, often be one of them: nothing else is as varied */
    if (use_real_players && g->top_of_p_table >= 0 && bot_dice(2)) {
      const struct player_index_element *player = &g->player_table[bot_dice(g->top_of_p_table + 1)];

      if (player->name && *player->name) {
        snprintf(line, sizeof(line), "login Real%d %s", number, player->name);
        journal_and_do(g);
        forget_closed_connections(g);
        continue;
      }
    }

    /* the first of each batch is an Implementor: they reach far more of the game */
    snprintf(line, sizeof(line), "spawn Bot%d %d %lu %lu %lu", number, number % NUM_BOTS == 0 ? ADMLVL_IMPL : ADMLVL_NONE,
             bot_dice(1000000), bot_dice(NUM_RACES), bot_dice(NUM_SENSEIS));
    journal_and_do(g);
    forget_closed_connections(g);
  }
  if (!bot_dice(60) && num_connections < NUM_BOTS * 3) {
    snprintf(line, sizeof(line), "connect Client%d", (*next_name)++);
    journal_and_do(g);
    forget_closed_connections(g);
  }

  c = &connections[bot_dice(num_connections)];
  if (at_login_menu(c->d) && (c->peer < 0 || !bot_dice(50))) {
    /* someone gives up at the menus; a spawned bot only gets here after quitting */
    snprintf(line, sizeof(line), "leave %s", c->label);
  } else if (!bot_dice(200)) {
    snprintf(line, sizeof(line), "leave %s", c->label); /* a connection drops, at any moment at all */
  } else {
    static char text[MAX_LINE - 32];

    compose_typing(g, c->d, text, sizeof(text));
    snprintf(line, sizeof(line), "type %s %s", c->label, text);
  }
  journal_and_do(g);

  if (!bot_dice(100)) {
    static const int jumps[] = {3600, 3600 * 6, 86400, 86400 * 7, 86400 * 30};

    snprintf(line, sizeof(line), "clock %d", jumps[bot_dice(sizeof(jumps) / sizeof(*jumps))]);
    journal_and_do(g);
  }

  snprintf(line, sizeof(line), "pulse %lu", bot_dice(10) + 1);
  journal_and_do(g);
}

static void finish(GameWorld *g)
{
  alarm(0);
  printf("fingerprint %lx\n", fingerprint);
  th_shutdown(g);
}

static void play_seed(unsigned long seed, int steps, FILE *journal_file)
{
  GameWorld *g = th_boot_at(START_TIME);
  int next_name = 0;

  journal   = journal_file;
  dice_state = seed;
  snprintf(line, sizeof(line), "seed %lu", seed);
  journal_and_do(g);
  for (int step = 0; step < steps; step++)
    bot_step(g, &next_name);
  finish(g);
}

static void play_journal(const char *path)
{
  FILE *fl = fopen(path, "r");
  GameWorld *g;

  if (!fl) {
    perror(path);
    exit(2);
  }
  g = th_boot_at(START_TIME);
  while (fgets(line, MAX_LINE, fl)) {
    line[strcspn(line, "\r\n")] = '\0';
    if (*line && *line != '#')
      do_action(g, line);
  }
  fclose(fl);
  finish(g);
}

struct signature {
  char text[300];
  char example[600];
  int count;
  bool known;
};

static struct signature signatures[MAX_SIGNATURES];
static int num_signatures = 0;

static void record(const char *text, const char *example, bool known)
{
  for (int i = 0; i < num_signatures; i++)
    if (!strcmp(signatures[i].text, text)) {
      signatures[i].count++;
      return;
    }
  if (num_signatures == MAX_SIGNATURES)
    return;
  snprintf(signatures[num_signatures].text, sizeof(signatures[num_signatures].text), "%s", text);
  snprintf(signatures[num_signatures].example, sizeof(signatures[num_signatures].example), "%s", example);
  signatures[num_signatures].known   = known;
  signatures[num_signatures++].count = 1;
}

/* What the report says went wrong: the sanitizer's or audit's own words, and the game's first frame. */
static void read_report(const char *path, char *text, size_t size, unsigned long *fingerprint_found, bool *known)
{
  char report_line[1024], summary[220] = "", frame[60] = "";
  FILE *fl = fopen(path, "r");

  *known = false;
  while (fl && fgets(report_line, sizeof(report_line), fl)) {
    char *at;

    report_line[strcspn(report_line, "\n")] = '\0';
    for (size_t i = 0; i < sizeof(known_failures) / sizeof(*known_failures); i++)
      if (strstr(report_line, known_failures[i]))
        *known = true;
    sscanf(report_line, "fingerprint %lx", fingerprint_found);
    if (!*summary && !strncmp(report_line, "AUDIT: ", 7))
      snprintf(summary, sizeof(summary), "%s", report_line);
    if (!*summary && (at = strstr(report_line, "runtime error:")))
      snprintf(summary, sizeof(summary), "%s", at);
    if (!*summary && (at = strstr(report_line, "SUMMARY: ")))
      snprintf(summary, sizeof(summary), "%s", at + 9);
    /* the accessors tell us nothing: they're only where a bad pointer happened to get used */
    if (!*frame && strstr(report_line, "/libs/dbat/src/") && !strstr(report_line, "_utils.c") && (at = strstr(report_line, " in "))) {
      char *end = strchr(at + 4, ' ');

      if (end)
        *end = '\0';
      snprintf(frame, sizeof(frame), "%s", at + 4);
    }
  }
  if (fl)
    fclose(fl);
  snprintf(text, size, "%s [%s]", *summary ? summary : "no report", frame);
}

/*
 * Runs a seed, or a journal if path is given, in a child process. Returns its fingerprint, or 0 after
 * recording what went wrong. A seed's journal is left in dir.
 */
static unsigned long run_child(const char *dir, unsigned long seed, int steps, const char *path)
{
  char report[600], journal_path[600], text[300], example[600];
  unsigned long found = 0;
  int status;
  bool known;
  pid_t pid;

  mkdir(dir, 0755);
  snprintf(report, sizeof(report), "%s/report.txt", dir);
  snprintf(journal_path, sizeof(journal_path), "%s/journal.txt", dir);
  if (path)
    snprintf(example, sizeof(example), "test_bot --replay %s", path);
  else
    snprintf(example, sizeof(example), "test_bot --seed %lu %d", seed, steps);
  fflush(nullptr);

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(2);
  }
  if (pid == 0) {
    FILE *journal_file = path ? nullptr : fopen(journal_path, "w");

    setenv("TMPDIR", dir, 1);
    if ((!path && !journal_file) || !freopen(report, "w", stdout) || dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
      _exit(2);
    if (path)
      play_journal(path);
    else
      play_seed(seed, steps, journal_file);
    th_exit(0);
  }

  waitpid(pid, &status, 0);
  read_report(report, text, sizeof(text), &found, &known);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && found)
    return found;

  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
    snprintf(text, sizeof(text), "hang: one action ran for over %d seconds", STEP_SECONDS);
  else if (WIFSIGNALED(status) && !strncmp(text, "no report", 9))
    snprintf(text, sizeof(text), "killed by signal %d", WTERMSIG(status));
  record(text, example, known);
  return 0;
}

/* Without a sanitizer a crash leaves no report, so a known bug can't be told from a new one. */
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
constexpr bool HAS_SANITIZER = true;
#else
constexpr bool HAS_SANITIZER = false;
#endif

static int print_failures(void)
{
  int new_failures = 0;

  for (int i = 0; i < num_signatures; i++) {
    printf("%4d %s %s\n      first seen with: %s\n", signatures[i].count, signatures[i].known ? "known" : "NEW  ",
           signatures[i].text, signatures[i].example);
    if (!signatures[i].known && (HAS_SANITIZER || strncmp(signatures[i].text, "killed by signal", 16)))
      new_failures++;
  }
  return new_failures;
}

static int run_seeds(const char *base, unsigned long first, int seeds, int steps)
{
  unsigned long repeat_fingerprint = 0;
  char repeat_journal[600] = "", repeat_dir[600];
  int finished = 0, failures = 0;

  for (int i = 0; i < seeds; i++) {
    char dir[512];
    unsigned long result;

    snprintf(dir, sizeof(dir), "%s/seed-%lu", base, first + i);
    result = run_child(dir, first + i, steps, nullptr);
    if (result)
      finished++;
    if (result && !repeat_fingerprint) {
      repeat_fingerprint = result;
      snprintf(repeat_journal, sizeof(repeat_journal), "%s/journal.txt", dir);
    }
  }
  /* a run that failed proves nothing here, so this uses the first that finished */
  snprintf(repeat_dir, sizeof(repeat_dir), "%s/replay", base);
  if (repeat_fingerprint && run_child(repeat_dir, 0, 0, repeat_journal) != repeat_fingerprint) {
    printf("FAIL: replaying %s didn't repeat the run exactly\n", repeat_journal);
    failures++;
  }
  failures += print_failures();
  printf("%d seeds of %d steps, %d finished, %d distinct failures, %d of them new\n", seeds, steps, finished, num_signatures, failures);
  return failures;
}

static int run_corpus(const char *base, const char *corpus)
{
  DIR *dir = opendir(corpus);
  struct dirent *entry;
  int files = 0, failures;

  if (!dir) {
    perror(corpus);
    return 1;
  }
  while ((entry = readdir(dir))) {
    char path[1024], child[1024];

    if (!strstr(entry->d_name, ".txt"))
      continue;
    snprintf(path, sizeof(path), "%s/%s", corpus, entry->d_name);
    snprintf(child, sizeof(child), "%s/corpus-%d", base, files++);
    run_child(child, 0, 0, path);
  }
  closedir(dir);
  failures = print_failures();
  printf("%d journals, %d distinct failures, %d of them new\n", files, num_signatures, failures);
  return failures;
}

int main(int argc, char **argv)
{
  const char *tmp = getenv("TMPDIR");
  char base[512], cleanup[600];
  int failures;

  /* The sanitizer only reads this at startup, and a stack trace is what says which code failed. */
  if (!getenv("UBSAN_OPTIONS")) {
    setenv("UBSAN_OPTIONS", "print_stacktrace=1", 1);
    execv("/proc/self/exe", argv);
  }

  if (argc > 2 && !strcmp(argv[1], "--world")) { /* a directory of real data, instead of the fixture */
    th_use_world(argv[2]);
    use_real_players = true;
    argv += 2;
    argc -= 2;
  }

  if (argc > 2 && !strcmp(argv[1], "--seed")) {
    FILE *journal_file = stdout;
    int steps = argc > 3 && *argv[3] != '-' ? atoi(argv[3]) : 2000;

    for (int i = 2; i + 1 < argc; i++)
      if (!strcmp(argv[i], "--journal") && !(journal_file = fopen(argv[i + 1], "w"))) {
        perror(argv[i + 1]);
        return 2;
      }
    play_seed(strtoul(argv[2], nullptr, 10), steps, journal_file);
    return 0;
  }
  if (argc > 2 && !strcmp(argv[1], "--replay")) {
    play_journal(argv[2]);
    return 0;
  }

  snprintf(base, sizeof(base), "%s/dbat-bot.XXXXXX", tmp ? tmp : "/tmp");
  if (!mkdtemp(base)) {
    perror("mkdtemp");
    return 2;
  }

  if (argc > 2 && !strcmp(argv[1], "--corpus"))
    failures = run_corpus(base, argv[2]);
  else
    failures = run_seeds(base, argc > 1 ? strtoul(argv[1], nullptr, 10) : 1, argc > 2 ? atoi(argv[2]) : 20, argc > 3 ? atoi(argv[3]) : 2000);

  snprintf(cleanup, sizeof(cleanup), "rm -rf '%s'", base);
  if (system(cleanup) != 0)
    printf("couldn't remove %s\n", base);
  return failures != 0;
}
