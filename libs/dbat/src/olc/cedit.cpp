/************************************************************************
 * OasisOLC - Game configuration / cedit.c                   v2.0	*
 * Copyright 2002-2003 Kip Potter   (kip_potter@hotmail.com)            *
 * A graphical in-game game configuration utility for OasisOLC.         *
 ************************************************************************/
#include <cerrno>
#include <climits>
#include <cstddef>
#include <type_traits>

#include "dbat/utils.hpp"
#include "dbat/consts/create.hpp"
#include "dbat/config.hpp"
#include "dbat/cedit.hpp"
#include "dbat/comm.hpp"
#include "dbat/oasis.hpp"
#include "dbat/spec_assign.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/genolc.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

/*
 * Every configuration option is one row of cedit_options[], in the order the config file
 * lists them. The row drives the editor's menu line and prompt, load_config()'s parsing
 * (config_load_option()) and save_config()'s output.
 */
enum cedit_type {
  CT_BOOL,   /* int, toggled from the menu */
  CT_INT,    /* int */
  CT_ROOM,   /* room_vnum that must exist */
  CT_METHOD, /* int, an index into creation_methods[] */
  CT_PORT,   /* uint16_t */
  CT_FLOAT,  /* float */
  CT_MSG,    /* char *, one line stored with a trailing "\r\n" */
  CT_STR,    /* char *, one line, may be NULL */
  CT_TEXT,   /* char *, several lines ending in '~' in the file, may be NULL */
};

constexpr int CO_NOBLANK      = (1 << 0); /* no blank line after it in the file */
constexpr int CO_OLD_ADMLEVEL = (1 << 1); /* the file may hold an old-style level (level_cap and up) */
constexpr int CO_RANGE        = (1 << 2); /* min/max apply */

struct cedit_option {
  const char *key;     /* config file key */
  const char *comment; /* comment lines written before it in the file, or NULL */
  enum cedit_type type;
  size_t offset;       /* in struct config_data */
  const char *section; /* starts this section of the file */
  unsigned flags;
  int min, max;       /* with CO_RANGE */
  char menu, choice;  /* the submenu (its main menu key) and the key in it; 0 if not edited */
  const char *label;  /* menu text, padded, before the ": " */
  const char *suffix; /* menu text after the value */
  const char *prompt;
  const char *note; /* sent after it is toggled */
};

/* The field's offset, refusing to compile unless the field has type T. */
template <typename Field, typename T> consteval size_t checked_offset(size_t offset)
{
  static_assert(std::is_same_v<Field, T>, "cedit option has the wrong type for its field");
  return offset;
}
#define FIELD(f, T) checked_offset<decltype(((struct config_data *)nullptr)->f), T>(offsetof(struct config_data, f))

#define BOOL(f)       .type = CT_BOOL, .offset = FIELD(f, int)
#define INT(f)        .type = CT_INT, .offset = FIELD(f, int)
#define ROOM(f)       .type = CT_ROOM, .offset = FIELD(f, room_vnum)
#define METHOD(f)     .type = CT_METHOD, .offset = FIELD(f, int)
#define PORT(f)       .type = CT_PORT, .offset = FIELD(f, uint16_t)
#define FLOAT(f)      .type = CT_FLOAT, .offset = FIELD(f, float)
#define MSG(f)        .type = CT_MSG, .offset = FIELD(f, char *)
#define STR(f)        .type = CT_STR, .offset = FIELD(f, char *)
#define TEXT(f)       .type = CT_TEXT, .offset = FIELD(f, char *)
#define RANGE(lo, hi) .flags = CO_RANGE, .min = (lo), .max = (hi)
#define MENU(m, c, l) .menu = (m), .choice = (c), .label = (l)

static const struct cedit_option cedit_options[] = {
    /* Game play options: menu G, apart from the two creation options (menu N) */
    {.key = "pk_allowed", .comment = "* Is player killing allowed on the mud?\n", BOOL(play.pk_allowed),
     .section = "Game Play Options", MENU('G', 'A', "Player Killing Allowed  ")},
    {.key = "pt_allowed", .comment = "* Is player thieving allowed on the mud?\n", BOOL(play.pt_allowed),
     MENU('G', 'B', "Player Thieving Allowed ")},
    {.key = "level_can_shout", .comment = "* What is the minimum level a player can shout/gossip/etc?\n", INT(play.level_can_shout),
     MENU('G', 'C', "Minimum Level To Shout  "), .prompt = "Enter the minimum level a player must be to shout, gossip, etc : "},
    {.key = "holler_move_cost", .comment = "* How many movement points does shouting cost the player?\n", INT(play.holler_move_cost),
     MENU('G', 'D', "Holler Move Cost        "), .prompt = "Enter the amount it costs (in move points) to holler : "},
    {.key = "tunnel_size", .comment = "* How many players can fit in a tunnel?\n", INT(play.tunnel_size),
     MENU('G', 'E', "Tunnel Size             "), .prompt = "Enter the maximum number of people allowed in a tunnel : "},
    {.key = "max_exp_gain", .comment = "* Maximum experience gainable per kill?\n", INT(play.max_exp_gain),
     MENU('G', 'F', "Maximum Experience Gain "), .prompt = "Enter the maximum gain of experience per kill for players : "},
    {.key = "max_exp_loss", .comment = "* Maximum experience loseable per death?\n", INT(play.max_exp_loss),
     MENU('G', 'G', "Maximum Experience Loss "), .prompt = "Enter the maximum loss of experience per death for players : "},
    {.key = "max_npc_corpse_time", .comment = "* Number of tics before NPC corpses decompose.\n", INT(play.max_npc_corpse_time),
     MENU('G', 'H', "Max Time for NPC Corpse "), .prompt = "Enter the number of tics before NPC corpses decompose : "},
    {.key = "max_pc_corpse_time", .comment = "* Number of tics before PC corpses decompose.\n", INT(play.max_pc_corpse_time),
     MENU('G', 'I', "Max Time for PC Corpse  "), .prompt = "Enter the number of tics before PC corpses decompose : "},
    {.key = "idle_void", .comment = "* Number of tics before a PC is sent to the void.\n", INT(play.idle_void),
     MENU('G', 'J', "Tics before PC sent to void "), .prompt = "Enter the number of tics before PC's are sent to the void (idle) : "},
    {.key = "idle_rent_time", .comment = "* Number of tics before a PC is autorented.\n", INT(play.idle_rent_time),
     MENU('G', 'K', "Tics before PC is autosaved "),
     .prompt = "Enter the number of tics before PC's are automatically rented and forced to quit : "},
    {.key = "idle_max_level", .comment = "* Admin level and above of players whom are immune to idle penalties.\n", INT(play.idle_max_level),
     .flags  = CO_OLD_ADMLEVEL, MENU('G', 'L', "Level Immune To IDLE        "),
     .prompt = "Enter the level a player must be to become immune to IDLE : "},
    {.key = "dts_are_dumps", .comment = "* Should the items in death traps be junked automatically?\n", BOOL(play.dts_are_dumps),
     MENU('G', 'M', "Death Traps Junk Items      ")},
    {.key = "load_into_inventory", .comment = "* When an immortal loads an object, should it load into their inventory?\n",
     BOOL(play.load_into_inventory), MENU('G', 'N', "Objects Load Into Inventory ")},
    {.key = "track_through_doors", .comment = "* Should PC's be able to track through hidden or closed doors?\n", BOOL(play.track_through_doors),
     MENU('G', 'O', "Track Through Doors         ")},
    {.key = "level_cap", .comment = "* What is the level that cannot be attained?\n", INT(play.level_cap),
     MENU('G', 'R', "Unattainable Level          "), .prompt = "Enter the number a character cannot level to : "},
    {.key = "stack_mobs", .comment = "* Stack mobiles when showing contents of rooms?\n", BOOL(play.stack_mobs),
     MENU('G', '1', "Stack Mobiles in room descs ")},
    {.key = "stack_objs", .comment = "* Stack objects when showing contents of rooms?\n", BOOL(play.stack_objs),
     MENU('G', '2', "Stack Objects in room descs ")},
    {.key = "mob_fighting", .comment = "* Allow aggressive mobs to attack other mobs?\n", BOOL(play.mob_fighting),
     MENU('G', '3', "Allow mobs to fight mobs    ")},
    {.key = "disp_closed_doors", .comment = "* Should closed doors be shown on autoexit / exit?\n", BOOL(play.disp_closed_doors),
     MENU('G', 'P', "Display Closed Doors        ")},
    {.key = "reroll_stats", .comment = "* Should players be able to reroll stats at creation?\n", BOOL(play.reroll_player),
     MENU('N', 'B', "Players can reroll stats on creation ")},
    {.key = "initial_points", .comment = "* How many points in players initial points pool if using point pool creation?\n",
     INT(play.initial_points), RANGE(0, INT_MAX),
     MENU('N', 'C', "Number of points in initial points pool for pool generation methods "), .suffix = " ",
     .prompt = "Enter size of initial points pool. (0 or greater). "},
    {.key = "compression", .comment = "* Should compression be used if the client supports it?\n", BOOL(play.enable_compression),
     MENU('G', '8', "Allow MCCP2 stream compression (recommended)")},
    {.key = "enable_languages", .comment = "* Should spoken languages be used?\n", BOOL(play.enable_languages),
     MENU('G', '9', "Allow spoken languages ")},
    {.key = "all_items_unique", .comment = "* Should all items be treated as unique?\n", BOOL(play.all_items_unique),
     MENU('G', 'S', "Treat all Objects as Unique ")},
    {.key = "exp_multiplier", .comment = "* Amount of in game experience multiplier.\n", FLOAT(play.exp_multiplier),
     MENU('G', 'T', "Experience multiplier       "),
     .prompt = "Enter the multiplier a player will recieve to experience gained : "},
    {.key = "ok", .comment = "* Text sent to players when OK is all that is needed.\n", MSG(play.OK),
     MENU('G', '4', "OK Message Text         "), .prompt = "Enter the OK message : "},
    {.key = "noperson", .comment = "* Text sent to players when noone is available.\n", MSG(play.NOPERSON),
     MENU('G', '5', "NOPERSON Message Text   "), .prompt = "Enter the NOPERSON message : "},
    {.key = "noeffect", .comment = "* Text sent to players when an effect fails.\n", MSG(play.NOEFFECT), .flags = CO_NOBLANK,
     MENU('G', '6', "NOEFFECT Message Text   "), .prompt = "Enter the NOEFFECT message : "},

    /* Rent and crashsave options: menu C */
    {.key = "free_rent",
     .comment = "* Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,\n"
     "* your objects are saved at no cost, as in Merc-type MUDs.)\n",
     BOOL(csd.free_rent), .section = "Rent/Crashsave Options", MENU('C', 'A', "Free Rent          ")},
    {.key = "max_obj_save", .comment = "* Maximum number of items players are allowed to rent.\n", INT(csd.max_obj_save),
     MENU('C', 'B', "Max Objects Saved  "), .prompt = "Enter the maximum number of items players can rent : "},
    {.key = "min_rent_cost", .comment = "* Receptionist's surcharge on top of item costs.\n", INT(csd.min_rent_cost),
     MENU('C', 'C', "Minimum Rent Cost  "), .prompt = "Enter the surcharge on top of item costs : "},
    {.key = "auto_save", .comment = "* Should the game automatically save people?\n", BOOL(csd.auto_save),
     MENU('C', 'D', "Auto Save          ")},
    {.key = "autosave_time", .comment = "* If auto_save = 1, how often (in minutes) should the game save people's objects?\n",
     INT(csd.autosave_time), MENU('C', 'E', "Auto Save Time     "), .suffix = " minute(s)",
     .prompt = "Enter how often (in minutes) should the MUD save players : "},
    {.key = "crash_file_timeout", .comment = "* Lifetime of crashfiles and force-rent (idlesave) files in days.\n",
     INT(csd.crash_file_timeout), MENU('C', 'F', "Crash File Timeout "), .suffix = " day(s)",
     .prompt = "Enter the lifetime of crash and idlesave files (days) : "},
    {.key = "rent_file_timeout", .comment = "* Lifetime of normal rent files in days.\n", INT(csd.rent_file_timeout),
     MENU('C', 'G', "Rent File Timeout  "), .suffix = " day(s)",
     .prompt = "Enter the lifetime of normal rent files (days) : "},

    /* Room numbers: menu R */
    {.key = "mortal_start_room", .comment = "* The virtual number of the room that mortals should enter at.\n",
     ROOM(room_nums.mortal_start_room), .section = "Room Numbers", MENU('R', 'A', "Mortal Start Room   "),
     .prompt = "Enter the room's vnum where mortals should load into : "},
    {.key = "immort_start_room", .comment = "* The virtual number of the room that immorts should enter at.\n",
     ROOM(room_nums.immort_start_room), MENU('R', 'B', "Immortal Start Room "),
     .prompt = "Enter the room's vnum where immortals should load into : "},
    {.key = "frozen_start_room", .comment = "* The virtual number of the room that frozen people should enter at.\n",
     ROOM(room_nums.frozen_start_room), MENU('R', 'C', "Frozen Start Room   "),
     .prompt = "Enter the room's vnum where frozen people should load into : "},
    {.key = "donation_room_1",
     .comment = "* The virtual numbers of the donation rooms.  Note: Add donation rooms\n"
     "* sequentially (1 & 2 before 3). If you don't, you might not be able to\n"
     "* donate. Use -1 for 'no such room'.\n",
     ROOM(room_nums.donation_room_1), .flags = CO_NOBLANK, MENU('R', '1', "Donation Room #1    "),
     .prompt = "Enter the vnum for donation room #1 : "},
    {.key = "donation_room_2", .comment = nullptr, ROOM(room_nums.donation_room_2), .flags = CO_NOBLANK,
     MENU('R', '2', "Donation Room #2    "), .prompt = "Enter the vnum for donation room #2 : "},
    {.key = "donation_room_3", .comment = nullptr, ROOM(room_nums.donation_room_3),
     MENU('R', '3', "Donation Room #3    "), .prompt = "Enter the vnum for donation room #3 : "},

    /* Game operation options: menu O */
    {.key = "DFLT_PORT",
     .comment = "* This is the default port on which the game should run if no port is\n"
     "* given on the command-line.  NOTE WELL: If you're using the\n"
     "* 'autorun' script, the port number there will override this setting.\n"
     "* Change the PORT= line in autorun instead of (or in addition to)\n"
     "* changing this.\n",
     PORT(operation.DFLT_PORT), .section = "Game Operation Options", RANGE(1, UINT16_MAX),
     MENU('O', 'A', "Default Port "), .prompt = "Enter the default port number : "},
    {.key = "DFLT_IP", .comment = "* IP address to which the MUD should bind.\n", STR(operation.DFLT_IP),
     MENU('O', 'B', "Default IP   "), .prompt = "Enter the default IP Address : "},
    {.key = "DFLT_DIR", .comment = "* default directory to use as data directory.\n", STR(operation.DFLT_DIR),
     MENU('O', 'C', "Default Directory   "), .prompt = "Enter the default directory : "},
    {.key = "LOGNAME", .comment = "* What file to log messages to (ex: 'log/syslog').\n", STR(operation.LOGNAME),
     MENU('O', 'D', "Logfile Name        "), .prompt = "Enter the name of the logfile : "},
    {.key = "max_playing", .comment = "* Maximum number of players allowed before game starts to turn people away.\n",
     INT(operation.max_playing), MENU('O', 'E', "Max Players         "), .prompt = "Enter the maximum number of players : "},
    {.key = "max_filesize", .comment = "* Maximum size of bug, typo, and idea files in bytes (to prevent bombing).\n",
     INT(operation.max_filesize), MENU('O', 'F', "Max Filesize        "), .prompt = "Enter the maximum size of the logs : "},
    {.key = "max_bad_pws", .comment = "* Maximum number of password attempts before disconnection.\n", INT(operation.max_bad_pws),
     MENU('O', 'G', "Max Bad Pws         "), .prompt = "Enter the maximum number of password attempts : "},
    {.key = "siteok_everyone", .comment = "* Is the site ok for everyone except those that are banned?\n", BOOL(operation.siteok_everyone),
     MENU('O', 'H', "Site Ok Everyone    ")},
    {.key = "use_new_socials",
     .comment = "* If you want to use the original social file format\n"
     "* and disable Aedit, set to 0, otherwise, 1.\n",
     BOOL(operation.use_new_socials), MENU('O', 'J', "Use new socials file"),
     .note = "Please note that using the stock social file will disable AEDIT.\r\n"},
    {.key = "nameserver_is_slow", .comment = "* Unused: sites are always recorded as numeric addresses.\n",
     BOOL(operation.nameserver_is_slow), MENU('O', 'I', "Name Server Is Slow ")},
    {.key = "auto_save_olc", .comment = "* Should OLC autosave to disk (1) or save internally (0).\n", BOOL(operation.auto_save_olc),
     MENU('O', 'K', "OLC autosave to disk")},
    {.key = "MENU", .comment = "* The entrance/exit menu.\n", TEXT(operation.MENU),
     MENU('O', 'L', "Main Menu           "), .prompt = "Enter the new MENU :\r\n\r\n"},
    {.key = "WELC_MESSG", .comment = "* The welcome message.\n", TEXT(operation.WELC_MESSG),
     MENU('O', 'M', "Welcome Message     "), .prompt = "Enter the new welcome message :\r\n\r\n"},
    {.key = "START_MESSG", .comment = "* NEWBIE start message.\n", TEXT(operation.START_MESSG),
     MENU('O', 'N', "Start Message       "), .prompt = "Enter the new newbie start message :\r\n\r\n"},

    /* Autowiz options: menu A */
    {.key = "use_autowiz",
     .comment = "* Should the game automatically create a new wizlist/immlist every time\n"
     "* someone immorts, or is promoted to a higher (or lower) god level?\n",
     BOOL(autowiz.use_autowiz), .section = "Autowiz Options", MENU('A', 'A', "Use the autowiz        ")},
    {.key = "min_wizlist_lev", .comment = "* If yes, what is the lowest level which should be on the wizlist?\n",
     INT(autowiz.min_wizlist_lev), .flags = CO_RANGE | CO_OLD_ADMLEVEL, .min = 0, .max = ADMLVL_IMPL,
     MENU('A', 'B', "Minimum wizlist level  "),
     .prompt = "Enter the minimum level for players to appear on the wizlist : "},

    /* Character advancement options: menu D */
    {.key = "allow_multiclass", .comment = "* Should characters be allowed to advance in multiple classes?\n",
     BOOL(advance.allow_multiclass), .section = "Character Advancement Options",
     MENU('D', 'A', "Allow multiclass       ")},
    {.key = "allow_prestige", .comment = "* Should characters be allowed to advance in prestige classes?\n",
     BOOL(advance.allow_prestige), MENU('D', 'B', "Allow Prestige Classes ")},

    /*
     * Game speed options: menu T. heartbeat() divides by the zone, mobile, autosave, idle
     * password, sanity and timesave pulses (and by idle password * 10 / 15), so those can't be 0.
     */
    {.key = "pulse_viol", .comment = "*Speed of Violence system. 0 - 10 (10) being the slowest.\n", INT(ticks.pulse_violence),
     .section = "Game Speed Options", RANGE(0, 10), MENU('T', 'A', "Pulse Violence Time  "),
     .prompt = "Enter the Speed of the violence system (1 - 10) 10 = Slowest : "},
    {.key = "pulse_mobile", .comment = "*Speed of Mobile actions. 0 - 20 (20) being the slowest.\n", INT(ticks.pulse_mobile),
     RANGE(1, 20), MENU('T', 'B', "Mobile Action Time   "),
     .prompt = "Enter the Speed of the Mobile Actions (1 - 20) 20 = Slowest : "},
    {.key = "pulse_zone", .comment = "*Zone updates. 0 - 20 (20) being the slowest.\n", INT(ticks.pulse_zone),
     RANGE(1, 20), MENU('T', 'C', "Zone Time            "), .prompt = "Enter Zone update time : "},
    {.key = "pulse_autosave", .comment = "*Autosave. 0 - 100 (100) being the slowest.\n", INT(ticks.pulse_autosave),
     RANGE(1, 100), MENU('T', 'D', "Autosave Time        "), .prompt = "Enter time for autosaving : "},
    {.key = "pulse_idlepwd", .comment = "*Idling passwords. 0 - 30 (30) being the slowest.\n", INT(ticks.pulse_idlepwd),
     RANGE(2, 30), MENU('T', 'E', "Idle Password Time   "),
     .prompt = "Enter the time to kill connection waiting for password : "},
    {.key = "pulse_sanity", .comment = "*Sanity check time.. 0 - 50 (50) being the slowest.\n", INT(ticks.pulse_sanity),
     RANGE(1, 50), MENU('T', 'F', "Sanity Check Time    "), .prompt = "Enter the sanity check time : "},
    {.key = "pulse_usage", .comment = "*Usage time.  0 - 10 (10) being the slowest in minutes.\n", INT(ticks.pulse_usage),
     RANGE(0, 10), MENU('T', 'G', "Usage Check Time     "), .prompt = "Enter the usage check time : "},
    {.key = "pulse_timesave", .comment = "*Timesaving. 0 - 50 (50) being the slowest in minutes.\n", INT(ticks.pulse_timesave),
     RANGE(1, 50), MENU('T', 'H', "Timesave Time        "), .prompt = "Enter timesave : "},
    {.key = "pulse_current", .comment = "*Current update time. 0 - 30 (30) being the slowest.\n", INT(ticks.pulse_current),
     RANGE(0, 30), MENU('T', 'I', "Currents Update Time "), .prompt = "Enter Current update time : "},
    {.key = "method", .comment = "*Character creation method.\n", METHOD(creation.method), RANGE(0, NUM_CREATION_METHODS - 1),
     MENU('N', 'A', "Character Creation Method ")},
};

constexpr int NUM_CEDIT_OPTIONS = countof(cedit_options);

static bool is_string_option(const struct cedit_option *o)
{
  return o->type == CT_MSG || o->type == CT_STR || o->type == CT_TEXT;
}

static void *option_field(struct config_data *c, const struct cedit_option *o)
{
  return (char *)c + o->offset;
}

static void cedit_disp_menu(GameWorld *g, struct descriptor_data *d);
static void cedit_disp_submenu(GameWorld *g, struct descriptor_data *d, char menu);
static void cedit_setup(GameWorld *g, struct descriptor_data *d);
static void reassign_rooms(GameWorld *g);

/******************************************************************************/
/** Routines                                                                 **/
/******************************************************************************/
ACMD(do_oasis_cedit)
{
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH];

  one_argument(argument, buf1);

  if (char_get_admlevel(ch) < ADMLVL_GRGOD) {
    send_to_char(g, ch, "You can't modify the game configuration.\r\n");
    return;
  }

  d = char_get_desc(ch);

  if (!*buf1) {
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZONE(d) = nullptr;
    cedit_setup(g, d);
    desc_set_connected(d, CON_CEDIT);
    act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_WRITING, true);

    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true,
           "OLC: %s starts editing the game configuration.", char_get_name(ch));
    return;
  }
  if (strcasecmp("save", buf1) != 0) {
    send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
    return;
  }

  send_to_char(g, ch, "Saving the game configuration.\r\n");
  mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true,
         "OLC: %s saves the game configuration.", char_get_name(ch));

  save_config(g, NOWHERE);
}

/*-------------------------------------------------------------------*/

/* The editor works on a copy of config_info, with its own copies of the strings. */
static void cedit_setup(GameWorld *g, struct descriptor_data *d)
{
  CREATE(OLC_CONFIG(d), struct config_data, 1);
  *OLC_CONFIG(d)          = g->config_info;
  OLC_CONFIG(d)->CONFFILE = nullptr;

  for (int i = 0; i < NUM_CEDIT_OPTIONS; i++) {
    const struct cedit_option *o = &cedit_options[i];
    char **s;

    if (!is_string_option(o))
      continue;
    s  = (char **)option_field(OLC_CONFIG(d), o);
    *s = o->type == CT_MSG ? str_udup(*s) : (*s ? strdup(*s) : nullptr);
  }

  cedit_disp_menu(g, d);
}

/******************************************************************************/

/* Installs the edited copy as config_info. The copy's strings move over with it. */
static void cedit_save_internally(GameWorld *g, struct descriptor_data *d)
{
  struct config_data *c = OLC_CONFIG(d);
  /* see if we need to reassign spec procs on rooms */
  int reassign = (CONFIG_DTS_ARE_DUMPS != c->play.dts_are_dumps);

  for (int i = 0; i < NUM_CEDIT_OPTIONS; i++)
    if (is_string_option(&cedit_options[i]))
      free(*(char **)option_field(&g->config_info, &cedit_options[i]));

  c->CONFFILE = g->config_info.CONFFILE;
  g->config_info = *c;

  for (int i = 0; i < NUM_CEDIT_OPTIONS; i++)
    if (is_string_option(&cedit_options[i]))
      *(char **)option_field(c, &cedit_options[i]) = nullptr;

  /* if we changed the dts to/from dumps, reassign - Welcor */
  if (reassign)
    reassign_rooms(g);

  add_to_save_list(g, NOWHERE, SL_CFG);
}

/******************************************************************************/

/* Called by load_config() for each "tag = value" line of the config file. */
void config_load_option(GameWorld *g, const char *tag, char *line, FILE *fl)
{
  static const char *const obsolete[] = {"auto_level", "immort_level_ok"};
  const struct cedit_option *o        = nullptr;
  char **s;
  int num;

  for (int i = 0; i < NUM_CEDIT_OPTIONS && !o; i++)
    if (!strcasecmp(tag, cedit_options[i].key))
      o = &cedit_options[i];

  if (!o) {
    for (size_t i = 0; i < countof(obsolete); i++)
      if (!strcasecmp(tag, obsolete[i]))
        mud_log("Ignoring obsolete config option %s", obsolete[i]);
    return;
  }

  s = (char **)option_field(&g->config_info, o);
  switch (o->type) {
    case CT_BOOL:
    case CT_INT:
    case CT_ROOM:
    case CT_METHOD:
      num = atoi(line);
      if ((o->flags & CO_OLD_ADMLEVEL) && num >= CONFIG_LEVEL_CAP)
        num += 1 - CONFIG_LEVEL_CAP;
      *(int *)option_field(&g->config_info, o) = num;
      break;
    case CT_PORT:
      *(uint16_t *)option_field(&g->config_info, o) = atoi(line);
      break;
    case CT_FLOAT:
      *(float *)option_field(&g->config_info, o) = atof(line);
      break;
    case CT_MSG:
      free(*s);
      CREATE(*s, char, strlen(line) + 3);
      sprintf(*s, "%s\r\n", line);
      break;
    case CT_STR:
      free(*s);
      *s = *line ? strdup(line) : nullptr;
      break;
    case CT_TEXT: {
      char error[MAX_INPUT_LENGTH];

      free(*s);
      snprintf(error, sizeof(error), "Reading %s in load_config()", o->key);
      *s = fread_string(fl, error);
      break;
    }
  }
}

/* Writes text to the config file without its carriage returns. */
static void put_no_cr(const char *text, FILE *fl)
{
  for (; *text; text++)
    if (*text != '\r')
      fputc(*text, fl);
}

bool save_config(GameWorld *g, IDXTYPE nowhere)
{
  struct atomic_file af;
  FILE *fl;

  if (!(fl = atomic_open(&af, CONFIG_CONFFILE, false))) {
    perror("SYSERR: save_config");
    return (false);
  }

  fprintf(fl,
          "* This file is autogenerated by OasisOLC (CEdit).\n"
          "* Please note the following information about this file's format.\n"
          "*\n"
          "* - If variable is a yes/no or true/false based variable, use 1's and 0's\n"
          "*   where YES or TRUE = 1 and NO or FALSE = 0.\n"
          "* - Variable names in this file are case-insensitive.  Variable values\n"
          "*   are not case-insensitive.\n"
          "* -----------------------------------------------------------------------\n"
          "* Lines starting with * are comments, and are not parsed.\n"
          "* -----------------------------------------------------------------------\n\n");

  for (int i = 0; i < NUM_CEDIT_OPTIONS; i++) {
    const struct cedit_option *o = &cedit_options[i];
    void *f                      = option_field(&g->config_info, o);

    if (o->section)
      fprintf(fl, "%s* [ %s ]\n", i ? "\n\n\n" : "", o->section);
    if (is_string_option(o) && !*(char **)f)
      continue;
    if (o->comment)
      fputs(o->comment, fl);

    switch (o->type) {
      case CT_BOOL:
      case CT_INT:
      case CT_ROOM:
      case CT_METHOD:
        fprintf(fl, "%s = %d\n", o->key, *(int *)f);
        break;
      case CT_PORT:
        fprintf(fl, "%s = %d\n", o->key, *(uint16_t *)f);
        break;
      case CT_FLOAT:
        fprintf(fl, "%s = %.2f\n", o->key, *(float *)f);
        break;
      case CT_MSG:
      case CT_STR:
        fprintf(fl, "%s = ", o->key);
        put_no_cr(*(char **)f, fl);
        fputc('\n', fl);
        break;
      case CT_TEXT:
        fprintf(fl, "%s = \n", o->key);
        put_no_cr(*(char **)f, fl);
        fputs("~\n", fl);
        break;
    }
    if (!(o->flags & CO_NOBLANK))
      fputc('\n', fl);
  }

  if (!atomic_commit(&af))
    return (false); /* still listed as unsaved */

  if (in_save_list(g, NOWHERE, SL_CFG))
    remove_from_save_list(g, NOWHERE, SL_CFG);

  return (true);
}

/**************************************************************************
 Menu functions
 **************************************************************************/

/*
 * the main menu
 */
static void cedit_disp_menu(GameWorld *g, struct descriptor_data *d)
{
  clear_screen(g, d);

  /*
   * Menu header
   */
  write_to_output(g, d,
                  "OasisOLC MUD Configuration Editor\r\n"
                  "@WA@B) @CAutowiz Options\r\n"
                  "@WC@B) @CCrashsave/Rent Options\r\n"
                  "@WD@B) @CaDvancement Options\r\n"
                  "@WG@B) @CGame Play Options\r\n"
                  "@WN@B) @CNew Character Creation Options\r\n"
                  "@WO@B) @COperation Options\r\n"
                  "@WR@B) @CRoom Numbers\r\n"
                  "@WT@B) @CGame Tick Options\r\n"
                  "@WQ@B) @CQuit\r\n"
                  "@WEnter your choice : @n");

  OLC_MODE(d) = CEDIT_MAIN_MENU;
}

/* One option's line in its submenu. */
static void cedit_disp_option(GameWorld *g, struct descriptor_data *d, const struct cedit_option *o)
{
  void *f = option_field(OLC_CONFIG(d), o);
  char buf[MAX_INPUT_LENGTH];
  const char *value = buf;
  int v;

  switch (o->type) {
    case CT_BOOL:
      v = *(int *)f;
      if (o->menu == 'O') /* the operation options always showed YES/NO */
        value = YESNO(v);
      else
        value = v == 1 ? "Yes" : "No";
      break;
    case CT_INT:
    case CT_ROOM:
      snprintf(buf, sizeof(buf), "%d", *(int *)f);
      break;
    case CT_METHOD:
      v = *(int *)f;
      snprintf(buf, sizeof(buf), "%d \r\n   %s", v,
               v >= 0 && v < NUM_CREATION_METHODS ? creation_methods[v] : "<invalid>");
      break;
    case CT_PORT:
      snprintf(buf, sizeof(buf), "%d", *(uint16_t *)f);
      break;
    case CT_FLOAT:
      snprintf(buf, sizeof(buf), "%.2f", *(float *)f);
      break;
    case CT_MSG:
    case CT_STR:
    case CT_TEXT:
      value = *(char **)f ? *(char **)f : "<None>";
      break;
  }

  if (o->type == CT_TEXT)
    write_to_output(g, d, "@W%c@B) @C%s: \r\n@n%s@n\r\n", o->choice, o->label, value);
  else /* a message ends in its own newline */
    write_to_output(g, d, "@W%c@B) @C%s: @c%s%s%s", o->choice, o->label, value, o->suffix ? o->suffix : "",
                    o->type == CT_MSG ? "" : "\r\n");
}

/* A submenu lists its options by key, letters first. */
static void cedit_disp_submenu(GameWorld *g, struct descriptor_data *d, char menu)
{
  static const char order[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";

  clear_screen(g, d);
  write_to_output(g, d, "\r\n\r\n");
  for (const char *key = order; *key; key++)
    for (int i = 0; i < NUM_CEDIT_OPTIONS; i++)
      if (cedit_options[i].menu == menu && cedit_options[i].choice == *key)
        cedit_disp_option(g, d, &cedit_options[i]);
  write_to_output(g, d, "@WQ@B) @CExit To The Main Menu\r\n"
                     "@WEnter your choice : @n");

  OLC_MODE(d) = CEDIT_SUBMENU;
  OLC_VAL(d)  = menu;
}

/* Asks for a new value of option i. */
static void cedit_prompt(GameWorld *g, struct descriptor_data *d, int i)
{
  const struct cedit_option *o = &cedit_options[i];
  char **text;

  OLC_MODE(d) = CEDIT_OPTION;
  OLC_VAL(d)  = i;

  if (o->type == CT_METHOD) {
    clear_screen(g, d);
    for (int m = 0; m < NUM_CREATION_METHODS; m++)
      write_to_output(g, d, "@W%d@B) @C%s@n\r\n", m, creation_methods[m]);
    write_to_output(g, d, "Choose character creation type: ");
  } else if (o->type == CT_TEXT) {
    text = (char **)option_field(OLC_CONFIG(d), o);
    clear_screen(g, d);
    send_editor_help(g, d);
    write_to_output(g, d, "%s", o->prompt);
    if (*text)
      write_to_output(g, d, "%s", *text);
    string_write(d, text, MAX_INPUT_LENGTH, 0, *text ? strdup(*text) : nullptr);
  } else
    write_to_output(g, d, "%s", o->prompt);
}

/* Reads an integer, allowing trailing spaces only. */
static bool parse_int(const char *arg, long *value)
{
  char *end;

  errno  = 0;
  *value = strtol(arg, &end, 10);
  while (isspace(*end))
    end++;
  return end != arg && !*end && !errno && *value >= INT_MIN && *value <= INT_MAX;
}

/* A message option (OK, NOPERSON, NOEFFECT): the text as typed, then a newline. */
static char *message_option(const char *text)
{
  char *msg;

  if (!*text)
    text = "undefined"; /* as str_udup() */
  CREATE(msg, char, strlen(text) + 3);
  sprintf(msg, "%s\r\n", text);
  return msg;
}

/*
 * Stores the value typed for option o. Bad input stores nothing: it gets a message and the
 * prompt again, and false is returned.
 */
static bool cedit_set_value(GameWorld *g, struct descriptor_data *d, int i, char *arg)
{
  const struct cedit_option *o = &cedit_options[i];
  void *f                      = option_field(OLC_CONFIG(d), o);
  char **s                     = (char **)f;
  long num;
  char *end;
  float fnum;

  switch (o->type) {
    case CT_MSG:
      genolc_checkstring(d, arg);
      free(*s);
      *s = message_option(arg);
      return true;

    case CT_STR:
      if (!*arg)
        break;
      free(*s);
      *s = strdup(arg);
      return true;

    case CT_FLOAT:
      fnum = strtof(arg, &end);
      while (isspace(*end))
        end++;
      if (end == arg || *end)
        break;
      *(float *)f = fnum;
      return true;

    default: /* the integer types */
      if (!parse_int(arg, &num))
        break;
      if ((o->flags & CO_RANGE) && (num < o->min || num > o->max)) {
        if (o->max == INT_MAX)
          write_to_output(g, d, "Please enter a number %d or higher.\r\n", o->min);
        else
          write_to_output(g, d, "Please enter a number between %d - %d.\r\n", o->min, o->max);
        cedit_prompt(g, d, i);
        return false;
      }
      if (o->type == CT_ROOM && real_room(g, num) == NOWHERE) {
        write_to_output(g, d, "That room doesn't exist!\r\n");
        cedit_prompt(g, d, i);
        return false;
      }
      if (o->type == CT_PORT)
        *(uint16_t *)f = num;
      else
        *(int *)f = num;
      return true;
  }

  write_to_output(g, d, "That is an invalid choice!\r\n");
  cedit_prompt(g, d, i);
  return false;
}

void cedit_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  switch (OLC_MODE(d)) {
    case CEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
        case 'y':
        case 'Y':
          cedit_save_internally(g, d);
          mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: %s modifies the game configuration.", char_get_name(d->character));
          cleanup_olc(g, d, CLEANUP_CONFIG);
          if (CONFIG_OLC_SAVE) {
            save_config(g, NOWHERE);
            write_to_output(g, d, "Game configuration saved to disk.\r\n");
          } else
            write_to_output(g, d, "Game configuration saved to memory.\r\n");
          return;
        case 'n':
        case 'N':
          write_to_output(g, d, "Game configuration not saved to memory.\r\n");
          cleanup_olc(g, d, CLEANUP_CONFIG);
          return;
        default:
          write_to_output(g, d, "\r\nThat is an invalid choice!\r\n");
          write_to_output(g, d, "Do you wish to save your changes? : ");
          return;
      }

    case CEDIT_MAIN_MENU:
      if (toupper(*arg) == 'Q') {
        write_to_output(g, d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = CEDIT_CONFIRM_SAVESTRING;
      } else if (*arg && strchr("ACDGNORT", toupper(*arg)))
        cedit_disp_submenu(g, d, toupper(*arg));
      else {
        write_to_output(g, d, "That is an invalid choice!\r\n");
        cedit_disp_menu(g, d);
      }
      return;

    case CEDIT_SUBMENU: {
      char menu = OLC_VAL(d);

      if (toupper(*arg) == 'Q') {
        cedit_disp_menu(g, d);
        return;
      }
      for (int i = 0; i < NUM_CEDIT_OPTIONS; i++) {
        const struct cedit_option *o = &cedit_options[i];
        int *flag;

        if (!*arg || o->menu != menu || o->choice != toupper(*arg))
          continue;
        if (o->type != CT_BOOL) {
          cedit_prompt(g, d, i);
          return;
        }
        flag  = (int *)option_field(OLC_CONFIG(d), o);
        *flag = *flag == 1 ? 0 : 1;
        if (o->note)
          send_to_char(g, d->character, "%s", o->note);
        cedit_disp_submenu(g, d, menu);
        return;
      }
      write_to_output(g, d, "\r\nThat is an invalid choice!\r\n");
      cedit_disp_submenu(g, d, menu);
      return;
    }

    case CEDIT_OPTION:
      if (cedit_set_value(g, d, OLC_VAL(d), arg))
        cedit_disp_submenu(g, d, cedit_options[OLC_VAL(d)].menu);
      return;

    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(g, d, CLEANUP_CONFIG);
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: cedit_parse(): Reached default case!");
      write_to_output(g, d, "Oops...\r\n");
      break;
  }
}

static void reassign_rooms(GameWorld *g)
{
  room_rnum i;

  /* remove old funcs */
  for (i = 0; i <= g->top_room; i++)
    rroom_set_func(g, i, nullptr);

  /* reassign spec_procs */
  assign_rooms(g);
}

/* The text editor has finished with a MENU, WELC_MESSG or START_MESSG. */
void cedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  if (OLC_MODE(d) == CEDIT_OPTION)
    cedit_disp_submenu(g, d, cedit_options[OLC_VAL(d)].menu);
}
