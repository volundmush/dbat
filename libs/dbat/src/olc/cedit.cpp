/************************************************************************
 * OasisOLC - Game configuration / cedit.c                   v2.0	*
 * Copyright 2002-2003 Kip Potter   (kip_potter@hotmail.com)            *
 * A graphical in-game game configuration utility for OasisOLC.         *
 ************************************************************************/
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <variant>

#include "dbat/args.hpp"
#include "dbat/utils.hpp"
#include "dbat/consts/create.hpp"
#include "dbat/config.hpp"
#include "dbat/cedit.hpp"
#include "dbat/comm.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/modify.hpp"
#include "dbat/spec_assign.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/genolc.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/olc/commands.hpp"

/*
 * Every configuration option is one row of cedit_options[], in the order the config file
 * lists them. The row drives the editor's menu line and prompt, load_config()'s parsing
 * (config_load_option()) and save_config()'s output.
 */
enum cedit_type {
  CT_BOOL,   /* bool, toggled from the menu */
  CT_INT,    /* int */
  CT_ROOM,   /* room_vnum that must exist */
  CT_METHOD, /* int, an index into creation_methods[] */
  CT_PORT,   /* uint16_t */
  CT_FLOAT,  /* float */
  CT_MSG,    /* std::string, one line stored with a trailing "\r\n" */
  CT_STR,    /* std::string, one line, may be empty */
  CT_TEXT,   /* std::string, several lines ending in '~' in the file, may be empty */
};

/* Where an option is in a config_data, as the type it has there */
template <class T> using config_field = T &(*)(struct config_data &);
using any_config_field                = std::variant<config_field<bool>, config_field<int>, config_field<uint16_t>, config_field<float>, config_field<std::string>>;

struct cedit_option {
  std::string_view key{};     /* config file key */
  std::string_view comment{}; /* comment lines written before it in the file, if any */
  enum cedit_type type{};
  any_config_field field; /* of the type its cedit_type says */
  std::string_view section{};  /* starts this section of the file */
  bool noblank{};        /* no blank line after it in the file */
  bool old_admlevel{};   /* the file may hold an old-style level (level_cap and up) */
  bool range{};          /* min/max apply */
  int min{}, max{};      /* with range */
  char menu{}, choice{}; /* the submenu (its main menu key) and the key in it; 0 if not edited */
  std::string_view label{};   /* menu text, padded, before the ": " */
  std::string_view suffix{};  /* menu text after the value */
  std::string_view prompt{};
  std::string_view note{}; /* sent after it is toggled */

  /* The option in c, as the type it is */
  template <class T> T &in(struct config_data &c) const { return std::get<config_field<T>>(field)(c); }
  bool is_string() const { return std::holds_alternative<config_field<std::string>>(field); }
};

/* Finds the field in a config_data; it won't compile unless the field has type T */
#define FIELD(f, T) static_cast<config_field<T>>([](struct config_data &c) -> T & { return c.f; })

#define BOOL(f)       .type = CT_BOOL, .field = FIELD(f, bool)
#define INT(f)        .type = CT_INT, .field = FIELD(f, int)
#define ROOM(f)       .type = CT_ROOM, .field = FIELD(f, room_vnum)
#define METHOD(f)     .type = CT_METHOD, .field = FIELD(f, int)
#define PORT(f)       .type = CT_PORT, .field = FIELD(f, uint16_t)
#define FLOAT(f)      .type = CT_FLOAT, .field = FIELD(f, float)
#define MSG(f)        .type = CT_MSG, .field = FIELD(f, std::string)
#define STR(f)        .type = CT_STR, .field = FIELD(f, std::string)
#define TEXT(f)       .type = CT_TEXT, .field = FIELD(f, std::string)
#define RANGE(lo, hi) .range = true, .min = (lo), .max = (hi)
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
     .old_admlevel = true, MENU('G', 'L', "Level Immune To IDLE        "),
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
    {.key = "noeffect", .comment = "* Text sent to players when an effect fails.\n", MSG(play.NOEFFECT), .noblank = true,
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
     ROOM(room_nums.donation_room_1), .noblank = true, MENU('R', '1', "Donation Room #1    "),
     .prompt = "Enter the vnum for donation room #1 : "},
    {.key = "donation_room_2", .comment = {}, ROOM(room_nums.donation_room_2), .noblank = true,
     MENU('R', '2', "Donation Room #2    "), .prompt = "Enter the vnum for donation room #2 : "},
    {.key = "donation_room_3", .comment = {}, ROOM(room_nums.donation_room_3),
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
     INT(autowiz.min_wizlist_lev), .old_admlevel = true, .range = true, .min = 0, .max = ADMLVL_IMPL,
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

namespace {
  /* The option the config file calls tag, or null */
  const struct cedit_option *option_keyed(std::string_view tag)
  {
    auto found = std::ranges::find_if(cedit_options, [&](const cedit_option &o) { return str_eq_nocase(tag, o.key); });
    return found == std::end(cedit_options) ? nullptr : found;
  }

  /* text without its carriage returns */
  std::string without_cr(std::string text)
  {
    std::erase(text, '\r');
    return text;
  }

  void reassign_rooms(GameWorld *g)
  {
    /* remove old funcs */
    for (room_vnum i : g->rooms.vnums())
      room_set_func(g, i, nullptr);

    /* reassign spec_procs */
    assign_rooms(g);
  }
} // namespace

/* Called by load_config() for each "tag = value" line of the config file. */
io::loaded<void> config_load_option(GameWorld *g, std::string_view tag, std::string_view value, io::text_reader &in)
{
  const struct cedit_option *o = option_keyed(tag);
  if (!o) {
    for (std::string_view old : {"auto_level", "immort_level_ok"})
      if (str_eq_nocase(tag, old))
        mud_log("Ignoring obsolete config option {}", old);
    return {};
  }

  struct config_data &c = g->config_info;
  switch (o->type) {
    case CT_BOOL: o->in<bool>(c) = io::c_atol(value) != 0; break; /* as atoi(), and the game only asked whether it was 0 */
    case CT_INT:
    case CT_ROOM:
    case CT_METHOD: {
      int num = static_cast<int>(io::c_atol(value)); /* as atoi() */
      if (o->old_admlevel && num >= CONFIG_LEVEL_CAP)
        num += 1 - CONFIG_LEVEL_CAP;
      o->in<int>(c) = num;
      break;
    }
    case CT_PORT: o->in<uint16_t>(c) = static_cast<uint16_t>(io::c_atol(value)); break;
    case CT_FLOAT: o->in<float>(c) = static_cast<float>(std::strtod(std::string(value).c_str(), nullptr)); break;
    case CT_MSG: o->in<std::string>(c) = str_format("{}\r\n", value); break;
    case CT_STR: o->in<std::string>(c) = value; break;
    case CT_TEXT: {
      io::loaded<std::string> text = in.tilde_string(str_format("config option {}", o->key));
      if (!text)
        return std::unexpected(text.error());
      o->in<std::string>(c) = std::move(*text);
      break;
    }
  }
  return {};
}

bool save_config(GameWorld *g, struct zone_data *)
{
  io::text_writer out(CONFIG_CONFFILE);
  out.write("* This file is autogenerated by OasisOLC (CEdit).\n"
            "* Please note the following information about this file's format.\n"
            "*\n"
            "* - If variable is a yes/no or true/false based variable, use 1's and 0's\n"
            "*   where YES or TRUE = 1 and NO or FALSE = 0.\n"
            "* - Variable names in this file are case-insensitive.  Variable values\n"
            "*   are not case-insensitive.\n"
            "* -----------------------------------------------------------------------\n"
            "* Lines starting with * are comments, and are not parsed.\n"
            "* -----------------------------------------------------------------------\n\n");

  struct config_data &c = g->config_info;
  bool first            = true;
  for (const cedit_option &o : cedit_options) {
    if (!o.section.empty())
      out.print("{}* [ {} ]\n", first ? "" : "\n\n\n", o.section);
    first = false;
    if (o.is_string() && o.in<std::string>(c).empty())
      continue;
    if (!o.comment.empty())
      out.write(o.comment);

    switch (o.type) {
      case CT_BOOL: out.print("{} = {:d}\n", o.key, int(o.in<bool>(c))); break;
      case CT_INT:
      case CT_ROOM:
      case CT_METHOD: out.print("{} = {:d}\n", o.key, o.in<int>(c)); break;
      case CT_PORT: out.print("{} = {:d}\n", o.key, o.in<uint16_t>(c)); break;
      case CT_FLOAT: out.print("{} = {:.2f}\n", o.key, o.in<float>(c)); break;
      case CT_MSG:
      case CT_STR: out.print("{} = {}\n", o.key, without_cr(o.in<std::string>(c))); break;
      case CT_TEXT: out.print("{} = \n{}~\n", o.key, without_cr(o.in<std::string>(c))); break;
    }
    if (!o.noblank)
      out.write("\n");
  }

  if (!out.commit())
    return (false); /* still listed as unsaved */

  if (in_save_list(g, NOWHERE, SL_CFG))
    remove_from_save_list(g, NOWHERE, SL_CFG);

  return (true);
}

namespace {
  /* The configuration editor: a copy of the configuration, put in its place when saved */
  struct config_editor final : olc_session {
    enum class mode { main_menu, confirm_save, submenu, option };

    mode asking = mode::main_menu;
    struct config_data config;
    char submenu{};                       /* the submenu shown or last shown, by its main menu key */
    const struct cedit_option *option{}; /* the option being asked for */

    explicit config_editor(const struct config_data &from) : config(from) { config.CONFFILE.clear(); }

    void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) override;
    void text_done(GameWorld *g, struct descriptor_data *d, int) override
    {
      if (asking == mode::option)
        show_submenu(g, d, option->menu);
    }
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override
    {
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} stops editing the game configuration", char_get_name(d->character));
    }

    void menu(GameWorld *g, struct descriptor_data *d);
    void show_option(GameWorld *g, struct descriptor_data *d, const struct cedit_option &o);
    void show_submenu(GameWorld *g, struct descriptor_data *d, char which);
    void ask(GameWorld *g, struct descriptor_data *d, const struct cedit_option &o);
    bool set(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void submenu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line);
    void install(GameWorld *g);
  };

  /* Puts the edited copy in place of the configuration */
  void config_editor::install(GameWorld *g)
  {
    /* see if we need to reassign spec procs on rooms */
    const bool reassign = CONFIG_DTS_ARE_DUMPS != config.play.dts_are_dumps;

    config.CONFFILE = g->config_info.CONFFILE;
    g->config_info  = config;

    /* if we changed the dts to/from dumps, reassign - Welcor */
    if (reassign)
      reassign_rooms(g);

    add_to_save_list(g, NOWHERE, SL_CFG);
  }

  void config_editor::menu(GameWorld *g, struct descriptor_data *d)
  {
    clear_screen(g, d);
    write_to_output(g, d,
                    "OasisOLC MUD Configuration Editor\r\n@WA@B) @CAutowiz Options\r\n@WC@B) @CCrashsave/Rent Options\r\n@WD@B) @CaDvancement Options\r\n@WG@B) @CGame Play Options\r\n@WN@B) @CNew Character Creation Options\r\n@WO@B) @COperation Options\r\n@WR@B) @CRoom Numbers\r\n@WT@B) @CGame Tick Options\r\n@WQ@B) @CQuit\r\n@WEnter your choice : @n");
    asking = mode::main_menu;
  }

  /* One option's line in its submenu */
  void config_editor::show_option(GameWorld *g, struct descriptor_data *d, const struct cedit_option &o)
  {
    std::string value;
    switch (o.type) {
      case CT_BOOL:
        if (o.menu == 'O') /* the operation options always showed YES/NO */
          value = YESNO(o.in<bool>(config));
        else
          value = o.in<bool>(config) ? "Yes" : "No";
        break;
      case CT_INT:
      case CT_ROOM: value = str_format("{:d}", o.in<int>(config)); break;
      case CT_METHOD: {
        const int v = o.in<int>(config);
        value       = str_format("{:d} \r\n   {}", v, v >= 0 && v < NUM_CREATION_METHODS ? creation_methods[v] : "<invalid>");
        break;
      }
      case CT_PORT: value = str_format("{:d}", o.in<uint16_t>(config)); break;
      case CT_FLOAT: value = str_format("{:.2f}", o.in<float>(config)); break;
      case CT_MSG:
      case CT_STR:
      case CT_TEXT: value = o.in<std::string>(config).empty() ? "<None>" : o.in<std::string>(config); break;
    }

    if (o.type == CT_TEXT)
      write_to_output(g, d, "@W{:c}@B) @C{}: \r\n@n{}@n\r\n", as_char(o.choice), o.label, value);
    else /* a message ends in its own newline */
      write_to_output(g, d, "@W{:c}@B) @C{}: @c{}{}{}", as_char(o.choice), o.label, value, o.suffix,
                      o.type == CT_MSG ? "" : "\r\n");
  }

  /* A submenu lists its options by key, letters first */
  void config_editor::show_submenu(GameWorld *g, struct descriptor_data *d, char which)
  {
    clear_screen(g, d);
    write_to_output(g, d, "\r\n\r\n");
    for (char key : std::string_view("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789"))
      for (const cedit_option &o : cedit_options)
        if (o.menu == which && o.choice == key)
          show_option(g, d, o);
    write_to_output(g, d, "@WQ@B) @CExit To The Main Menu\r\n@WEnter your choice : @n");
    asking  = mode::submenu;
    submenu = which;
  }

  /* Asks for a new value of option o */
  void config_editor::ask(GameWorld *g, struct descriptor_data *d, const struct cedit_option &o)
  {
    asking = mode::option;
    option = &o;

    if (o.type == CT_METHOD) {
      clear_screen(g, d);
      for (int m = 0; m < NUM_CREATION_METHODS; m++)
        write_to_output(g, d, "@W{:d}@B) @C{}@n\r\n", m, creation_methods[m]);
      write_to_output(g, d, "Choose character creation type: ");
    } else if (o.type == CT_TEXT) {
      std::string &text = o.in<std::string>(config);
      clear_screen(g, d);
      send_editor_help(g, d);
      write_to_output(g, d, "{}", o.prompt);
      if (!text.empty())
        write_to_output(g, d, "{}", text);
      string_write(d, &text, MAX_INPUT_LENGTH, 0);
    } else
      write_to_output(g, d, "{}", o.prompt);
  }

  /* An integer, with trailing spaces only */
  std::optional<int> whole_number(const std::string &typed)
  {
    std::string_view text = until_nul(typed); /* as strtol() read it */
    size_t at             = 0;
    while (at < text.size() && isspace(static_cast<unsigned char>(text[at])))
      at++;
    size_t start = at;
    if (at < text.size() && (text[at] == '+' || text[at] == '-'))
      at++;
    size_t digits = at;
    while (at < text.size() && isdigit(static_cast<unsigned char>(text[at])))
      at++;
    if (at == digits || !std::ranges::all_of(text.substr(at), [](char c) { return isspace(c) != 0; }))
      return std::nullopt;
    const long long read = io::c_atoll(text.substr(start, at - start)); /* held to a long long's range, so past an int */
    if (read < INT_MIN || read > INT_MAX)
      return std::nullopt;
    return static_cast<int>(read);
  }

  /* A float, with trailing spaces only */
  std::optional<float> real_number(const std::string &text)
  {
    char *end;
    const float read = strtof(text.c_str(), &end);
    while (isspace(*end))
      end++;
    if (end == text.c_str() || *end)
      return std::nullopt;
    return read;
  }

  /*
   * Stores the value typed for the option asked for. Bad input stores nothing: it gets a message and the prompt again,
   * and false is returned.
   */
  bool config_editor::set(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const struct cedit_option &o = *option;
    std::string text(line);

    switch (o.type) {
      case CT_MSG: /* the text as typed, then a newline */
        smash_tilde(text);
        o.in<std::string>(config) = text_or_undefined(text) + "\r\n";
        return true;

      case CT_STR:
        if (text.empty())
          break;
        o.in<std::string>(config) = text;
        return true;

      case CT_FLOAT:
        if (std::optional<float> read = real_number(text)) {
          o.in<float>(config) = *read;
          return true;
        }
        break;

      default: { /* the integer types */
        std::optional<int> read = whole_number(text);
        if (!read)
          break;
        if (o.range && (*read < o.min || *read > o.max)) {
          if (o.max == INT_MAX)
            write_to_output(g, d, "Please enter a number {:d} or higher.\r\n", o.min);
          else
            write_to_output(g, d, "Please enter a number between {:d} - {:d}.\r\n", o.min, o.max);
          ask(g, d, o);
          return false;
        }
        if (o.type == CT_ROOM && !room_of(g, *read)) {
          write_to_output(g, d, "That room doesn't exist!\r\n");
          ask(g, d, o);
          return false;
        }
        if (o.type == CT_PORT)
          o.in<uint16_t>(config) = *read;
        else
          o.in<int>(config) = *read;
        return true;
      }
    }

    write_to_output(g, d, "That is an invalid choice!\r\n");
    ask(g, d, o);
    return false;
  }

  void config_editor::submenu_choice(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    const char key = toupper(first_char(line));
    if (key == 'Q') {
      menu(g, d);
      return;
    }
    auto found = std::ranges::find_if(cedit_options, [&](const cedit_option &o) { return key && o.menu == submenu && o.choice == key; });
    if (found == std::end(cedit_options)) {
      write_to_output(g, d, "\r\nThat is an invalid choice!\r\n");
      show_submenu(g, d, submenu);
      return;
    }
    if (found->type != CT_BOOL) {
      ask(g, d, *found);
      return;
    }
    bool &flag = found->in<bool>(config);
    flag       = !flag;
    if (!found->note.empty())
      send_to_char(g, d->character, "{}", found->note);
    show_submenu(g, d, submenu);
  }

  void config_editor::save_answer(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    std::optional<bool> save = yes_or_no(line);
    if (!save) {
      write_to_output(g, d, "\r\nThat is an invalid choice!\r\n");
      write_to_output(g, d, "Do you wish to save your changes? : ");
      return;
    }
    if (!*save) {
      write_to_output(g, d, "Game configuration not saved to memory.\r\n");
      leave_olc(g, d);
      return;
    }
    install(g);
    mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, d->character)), true, "OLC: {} modifies the game configuration.",
           char_get_name(d->character));
    leave_olc(g, d);
    if (CONFIG_OLC_SAVE) {
      save_config(g, nullptr);
      write_to_output(g, d, "Game configuration saved to disk.\r\n");
    } else
      write_to_output(g, d, "Game configuration saved to memory.\r\n");
  }

  void config_editor::parse(GameWorld *g, struct descriptor_data *d, std::string_view line)
  {
    switch (asking) {
      case mode::confirm_save: save_answer(g, d, line); return;
      case mode::main_menu: {
        const char key = toupper(first_char(line));
        if (key == 'Q') {
          write_to_output(g, d, "Do you wish to save your changes? : ");
          asking = mode::confirm_save;
        } else if (key && std::string_view("ACDGNORT").contains(key))
          show_submenu(g, d, key);
        else {
          write_to_output(g, d, "That is an invalid choice!\r\n");
          menu(g, d);
        }
        return;
      }
      case mode::submenu: submenu_choice(g, d, line); return;
      case mode::option:
        if (set(g, d, line))
          show_submenu(g, d, option->menu);
        return;
    }
  }
} // namespace

ACMD(do_oasis_cedit)
{
  std::string_view rest = argument;
  std::string buf1      = take_arg(rest);

  if (char_get_admlevel(ch) < ADMLVL_GRGOD) {
    send_to_char(g, ch, "You can't modify the game configuration.\r\n");
    return;
  }

  if (buf1.empty()) {
    struct descriptor_data *d = char_get_desc(ch);
    open_session(g, d, std::make_unique<config_editor>(g->config_info), "");
    session_of<config_editor>(d).menu(g, d);
    begin_editing(g, d, CON_CEDIT);
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} starts editing the game configuration.", char_get_name(ch));
    return;
  }
  if (!str_eq_nocase("save", buf1)) {
    send_to_char(g, ch, "Yikes!  Stop that, someone will get hurt!\r\n");
    return;
  }

  send_to_char(g, ch, "Saving the game configuration.\r\n");
  mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves the game configuration.", char_get_name(ch));
  save_config(g, nullptr);
}
