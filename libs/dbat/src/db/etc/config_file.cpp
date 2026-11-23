/*
 * The game's settings: config.cpp's defaults, then lib/etc/config over them, a "tag = value" line each (what each
 * tag sets is cedit's config_load_option()).
 */
#include <algorithm>
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/cedit.hpp"

/* External variables from config.c */

static void load_default_config(GameWorld *g)
{
  /****************************************************************************/
  /** This function is called only once, at boot-time.                       **/
  /** - We assume config_info is empty                          -- Welcor    **/
  /****************************************************************************/
  /****************************************************************************/
  /** Game play options.                                                     **/
  /****************************************************************************/
  CONFIG_PK_ALLOWED             = pk_allowed;
  CONFIG_PT_ALLOWED             = pt_allowed;
  CONFIG_LEVEL_CAN_SHOUT        = level_can_shout;
  CONFIG_HOLLER_MOVE_COST       = holler_move_cost;
  CONFIG_TUNNEL_SIZE            = tunnel_size;
  CONFIG_MAX_EXP_GAIN           = max_exp_gain;
  CONFIG_MAX_EXP_LOSS           = max_exp_loss;
  CONFIG_MAX_NPC_CORPSE_TIME    = max_npc_corpse_time;
  CONFIG_MAX_PC_CORPSE_TIME     = max_pc_corpse_time;
  CONFIG_IDLE_VOID              = idle_void;
  CONFIG_IDLE_RENT_TIME         = idle_rent_time;
  CONFIG_IDLE_MAX_LEVEL         = idle_max_level;
  CONFIG_DTS_ARE_DUMPS          = dts_are_dumps;
  CONFIG_LOAD_INVENTORY         = load_into_inventory;
  CONFIG_OK                     = OK;
  CONFIG_NOPERSON               = NOPERSON;
  CONFIG_NOEFFECT               = NOEFFECT;
  CONFIG_TRACK_T_DOORS          = track_through_doors;
  CONFIG_LEVEL_CAP              = level_cap;
  CONFIG_STACK_MOBS             = show_mob_stacking;
  CONFIG_STACK_OBJS             = show_obj_stacking;
  CONFIG_MOB_FIGHTING           = mob_fighting;
  CONFIG_DISP_CLOSED_DOORS      = disp_closed_doors;
  CONFIG_REROLL_PLAYER_CREATION = reroll_status;
  CONFIG_INITIAL_POINTS_POOL    = initial_points;
  CONFIG_ENABLE_COMPRESSION     = enable_compression;
  CONFIG_ENABLE_LANGUAGES       = enable_languages;
  CONFIG_ALL_ITEMS_UNIQUE       = all_items_unique;
  CONFIG_EXP_MULTIPLIER         = g->exp_multiplier;
  /****************************************************************************/
  /** Rent / crashsave options.                                              **/
  /****************************************************************************/
  CONFIG_FREE_RENT     = free_rent;
  CONFIG_MAX_OBJ_SAVE  = max_obj_save;
  CONFIG_MIN_RENT_COST = min_rent_cost;
  CONFIG_AUTO_SAVE     = auto_save;
  CONFIG_AUTOSAVE_TIME = autosave_time;
  CONFIG_CRASH_TIMEOUT = crash_file_timeout;
  CONFIG_RENT_TIMEOUT  = rent_file_timeout;

  /****************************************************************************/
  /** Room numbers.                                                          **/
  /****************************************************************************/
  CONFIG_MORTAL_START   = mortal_start_room;
  CONFIG_IMMORTAL_START = immort_start_room;
  CONFIG_FROZEN_START   = frozen_start_room;
  CONFIG_DON_ROOM_1     = donation_room_1;
  CONFIG_DON_ROOM_2     = donation_room_2;
  CONFIG_DON_ROOM_3     = donation_room_3;

  /****************************************************************************/
  /** Game operation options.                                                **/
  /****************************************************************************/
  CONFIG_DFLT_PORT = DFLT_PORT;

  CONFIG_DFLT_IP  = DFLT_IP;
  CONFIG_DFLT_DIR = DFLT_DIR;
  CONFIG_LOGNAME  = LOGNAME;

  CONFIG_MAX_PLAYING    = max_playing;
  CONFIG_MAX_FILESIZE   = max_filesize;
  CONFIG_MAX_BAD_PWS    = max_bad_pws;
  CONFIG_SITEOK_ALL     = siteok_everyone;
  CONFIG_NS_IS_SLOW     = nameserver_is_slow;
  CONFIG_NEW_SOCIALS    = use_new_socials;
  CONFIG_OLC_SAVE       = auto_save_olc;
  CONFIG_MENU           = MENU;
  CONFIG_WELC_MESSG     = WELC_MESSG;
  CONFIG_START_MESSG    = START_MESSG;
  CONFIG_EXP_MULTIPLIER = 1.0;

  /****************************************************************************/
  /** Autowiz options.                                                       **/
  /****************************************************************************/
  CONFIG_USE_AUTOWIZ     = use_autowiz;
  CONFIG_MIN_WIZLIST_LEV = min_wizlist_lev;

  /****************************************************************************/
  /** Character advancement options.                                         **/
  /****************************************************************************/
  CONFIG_ALLOW_MULTICLASS = allow_multiclass;
  CONFIG_ALLOW_PRESTIGE   = allow_prestige;

  /****************************************************************************/
  /** ticks menu                                                             **/
  /****************************************************************************/
  CONFIG_PULSE_VIOLENCE = pulse_violence;
  CONFIG_PULSE_MOBILE   = pulse_mobile;
  CONFIG_PULSE_ZONE     = pulse_zone;
  CONFIG_PULSE_CURRENT  = pulse_current;
  CONFIG_PULSE_SANITY   = pulse_sanity;
  CONFIG_PULSE_IDLEPWD  = pulse_idlepwd;
  CONFIG_PULSE_AUTOSAVE = pulse_autosave;
  CONFIG_PULSE_USAGE    = pulse_usage;
  CONFIG_PULSE_TIMESAVE = pulse_timesave;

  /****************************************************************************/
  /** Character Creation Method                                              **/
  /****************************************************************************/
  CONFIG_CREATION_METHOD = method;
}

/* "tag = value": the tag is what is before the '=', its spaces taken out (quirk: "a b" is "ab"); the value is
   after it and the spaces and '='s that follow */
static std::pair<std::string, std::string_view> split_option(std::string_view line)
{
  size_t equals = std::min(line.find('='), line.size());
  std::string tag;
  for (char c : line.substr(0, equals))
    if (c != ' ')
      tag += c;
  std::string_view value = line.substr(equals);
  while (value.starts_with('=') || value.starts_with(' '))
    value.remove_prefix(1);
  return {std::move(tag), value};
}

void load_config(GameWorld *g)
{
  load_default_config(g);

  io::loaded<io::text_reader> file = io::text_reader::open(CONFIG_CONFFILE);
  if (!file)
    file = io::text_reader::open(str_format("{}/{}", DFLT_DIR, CONFIG_CONFFILE));
  if (!file) {
    mud_log("SYSERR: Game Config File: {}: can't be opened", CONFIG_CONFFILE);
    return;
  }

  while (std::optional<std::string_view> line = file->line()) {
    auto [tag, value] = split_option(*line);
    if (io::loaded<void> read = config_load_option(g, tag, value, *file); !read) {
      mud_log("SYSERR: {}", read.error().text());
      exit(1);
    }
  }
  if (CONFIG_DFLT_DIR.empty()) /* an empty DFLT_DIR line */
    CONFIG_DFLT_DIR = DFLT_DIR;
}
