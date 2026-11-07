#pragma once
#include <ctime>

#include "dbat/consts/types.hpp"
#include "dbat/registry.hpp"
#include "dbat/rooms.hpp"
#include "dbat/guild.hpp"
#include "dbat/characters.hpp"
#include "dbat/index.hpp"
#include "dbat/objects.hpp"
#include "dbat/shop.hpp"
#include "dbat/ban.hpp"
#include "dbat/config.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/feats.hpp"
#include "dbat/house.hpp"
#include "dbat/maputils.hpp"
#include "dbat/oasis.hpp"
#include "dbat/spells.hpp"
#include "dbat/weather.hpp"
#include "dbat/zones.hpp"

/* The state of one running game. */
struct GameWorld {
  /* Entities and their prototypes */
  vnum_table<room_data, room_vnum> rooms; /* all rooms, by vnum */
  struct char_data *characters{}; /* All NPCs and PCs in the game      */
  struct obj_data *objects{};     /* All objects in the game           */
  struct char_data *affect_list{};
  struct char_data *combat_list{};
  struct char_data *next_combat_list{};
  vnum_table<index_data, mob_vnum> mob_index; /* mob prototypes, by vnum */
  vnum_table<char_data, mob_vnum> mob_proto;
  long max_mob_id{};
  vnum_table<index_data, obj_vnum> obj_index; /* object prototypes, by vnum */
  vnum_table<obj_data, obj_vnum> obj_proto;
  long max_obj_id{};
  struct obj_unique_hash_elem **obj_unique_hash_pools{};
  int mob_specials_used{};
  int number_of_assassins{};
  int xap_objs{};

  /* Zones */
  vnum_table<zone_data, zone_vnum> zones; /* in vnum order */
  struct reset_q_type reset_q{};

  /* Shops and guilds */
  vnum_table<shop_data, shop_vnum> shops;    /* in vnum order */
  vnum_table<guild_data, guild_vnum> guilds; /* in vnum order */
  int cmd_say{}, cmd_tell{}, cmd_emote{}, cmd_slap{}, cmd_puke{};

  /* Scripts */
  vnum_table<index_data, trig_vnum> trig_index; /* each entry holds its trigger as proto */
  struct trig_data *trigger_list{};
  struct trig_data *extracted_triggers{}; /* waiting for free_extracted_triggers() */
  struct dg_run_ctx dg_ctx{};
  struct lookup_table_t lookup_table[BUCKET_COUNT]{};
  long lookup_table_entries{};
  int dg_owner_purged{};
  int dg_eval_depth{}; /* how deeply nested the expression being evaluated is */
  int dg_act_check{};
  int script_depth{}; /* How many scripts are running inside each other */
  bool warned_mob_lookup{};
  bool warned_obj_lookup{};
  struct queue *event_q{};

  /* Players */
  struct player_index_element *player_table{};
  int top_of_p_table{};
  int top_of_p_file{};
  long top_idnum{};

  /* Connections and the server */
  struct descriptor_data *descriptors{};
  struct txt_block *bufpool{};
  int buf_largecount{}, buf_overflows{}, buf_switches{};
  int max_players{};
  int mother_desc{};
  uint16_t port{};
  bool fCopyOver{};
  int launch_argc{}; /* how and where this process started, for a copyover to do the same */
  char **launch_argv{};
  char *launch_dir{};
  int last_desc{};
  int circle_shutdown{}, circle_reboot{}, circle_restrict{};
  int copyover_timer{};
  time_t boot_time{};
  unsigned long pulse{};
  int mins_since_crashsave{};
  char *last_act_message{};

  /* Run modes */
  int mini_mud{}, no_mail{}, no_rent_check{}, no_specials{}, scheck{}, converting{};

  /* Time, weather and randomness */
  struct time_info_data time_info{};
  struct weather_data weather_info{};
  unsigned long rng_seed{};
  time_t pinned_time{}; /* When non-zero, what game_now() returns. Only tests set it. */

  /* Configuration, loaded at boot and edited with cedit */
  struct config_data config_info{};
  float exp_multiplier{};
  room_vnum r_mortal_start_room{}, r_immort_start_room{}, r_frozen_start_room{};
  char level_version[READ_SIZE]{};
  int level_vernum{};
  int save_classes[SAVING_WILL + 1][NUM_CLASSES]{};
  int basehit_classes[NUM_CLASSES]{};

  /* Commands, skills and feats */
  struct command_info *complete_cmd_info{};
  int *cmd_sort_info{};
  struct spell_info_type spell_info[SKILL_TABLE_SIZE]{};
  int spell_sort_info[SKILL_TABLE_SIZE]{};
  struct feat_info feat_list[NUM_FEATS_DEFINED]{};
  int feat_sort_info[MAX_FEATS]{};
  struct disabled_data *disabled_first{};
  struct social_messg *soc_mess_list{};
  int top_of_socialt{};

  /* Text loaded at boot */
  char *background{}, *credits{}, *handbook{}, *help{}, *ihelp{}, *immlist{}, *imotd{}, *info{};
  char *motd{}, *news{}, *policies{}, *wizlist{}, *GREETANSI{}, *GREETINGS{};
  struct help_index_element *help_table{};
  int top_of_helpt{};
  char *context_help_list[NUM_CONTEXTS]{};

  /* Boards, mail, houses and bans */
  struct board_info *bboards{};
  time_t BOARDNEWBUI{}, BOARDNEWCOD{}, BOARDNEWDUO{}, BOARDNEWIMM{}, BOARDNEWMORT{};
  struct mail_index_type_d *mail_index{};
  struct position_list_type_d *free_list{};
  long file_end_pos{};
  char *mail_buf{};
  long mail_buf_len{}, mail_buf_alloc{};
  struct house_control_rec house_control[MAX_HOUSES]{};
  int num_of_houses{};
  struct ban_list_element *ban_list{};
  char *invalid_list[MAX_INVALID_NAMES]{};
  int num_invalid{};
  struct assembly_data *g_pAssemblyTable{};
  long g_lNumAssemblies{};

  /* Dragon balls and wishes */
  int SHENRON{}, DRAGONC{}, DRAGONR{}, DRAGONZ{}, dballtime{}, WISHTIME{};
  int WISH[2]{};
  int32_t EDRAGON_uid{}; /* the summoned Shenron, by id */
  int SHADOW_DRAGON1{}, SHADOW_DRAGON2{}, SHADOW_DRAGON3{}, SHADOW_DRAGON4{}, SHADOW_DRAGON5{}, SHADOW_DRAGON6{}, SHADOW_DRAGON7{};

  /* Other game events and counters */
  int ERAPLAYERS{}, HEDITS{}, HIGHPCOUNT{}, LASTNEWS{}, LASTPAYTYPE{}, MAJIN_ALLOWED{}, MOON_UP{}, PCOUNT{};
  int SAIYAN_ALLOWED{}, SELFISHMETER{}, TOPCOUNTDOWN{}, TOPLOADED{}, TOP_OF_NEWS{};
  time_t INTERESTTIME{}, LASTINTEREST{}, LASTPAYOUT{}, NEWSUPDATE{}, PCOUNTDATE{}, PCOUNTDAY{};
  char *topname[25]{};
  int64_t toppoint[25]{};

  /* Auctions */
  int32_t ch_selling_uid{}, ch_buying_uid{}; /* by id, as entities refer to each other */
  int32_t obj_selling_uid{};
  int aucstat{}, curbid{};

  /* Pathfinding and map display */
  struct bfs_queue_struct *bfs_queue_head{}, *bfs_queue_tail{};
  room_vnum mapnums[MAP_ROWS + 1][MAP_COLS + 1]{};

  /* OLC and bookkeeping */
  struct save_list_data *save_list{};
  obj_vnum lRnum{};
  int extractions_pending{};

  /* Progress of the world file parsers and the zone clock */
  room_vnum parse_room_index{};
  struct zone_data *parse_room_zone{};
  mob_vnum parse_mob_index{};
  obj_vnum parse_obj_index{};
  int zone_update_timer{};
};

GameWorld *game_world_create(void);

/* Frees only the GameWorld. Its contents belong to destroy_db() or whoever else built them. */
void game_world_destroy(GameWorld *g);

/* Frees everything a booted game owns, except the GameWorld itself. */
void game_world_free_contents(GameWorld *g);

/* The current wall-clock time, as the game sees it. */
time_t game_now(const GameWorld *g);

