#ifndef CIRCLE_ACT_H
#define CIRCLE_ACT_H
//
// Created by volund on 10/20/21.
//

#include "typestubs.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "dg_comm.h"
#include "clan.h"
#include "combat.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "boards.h"
#include "players.h"
#include "fight.h"
#include "feats.h"
#include "clan.h"
#include "maputils.h"
#include "vehicles.h"
#include "guild.h"
#include "spells.h"
#include "oasis.h"
#include "feats.h"
#include "assemblies.h"
#include "obj_edit.h"
#include "combat.h"
#include "weather.h"
#include "shop.h"
#include "limits.h"
#include "graph.h"
#include "spell_parser.h"

/* act.attack.c */

// Commands
ACMD(do_spike);
ACMD(do_selfd);
ACMD(do_spiral);
ACMD(do_breaker);
ACMD(do_throw);
ACMD(do_razor);
ACMD(do_koteiru);
ACMD(do_hspiral);
ACMD(do_seishou);
ACMD(do_bash);
ACMD(do_head);
ACMD(do_nova);
ACMD(do_malice);
ACMD(do_zen);
ACMD(do_sunder);
ACMD(do_combine);
ACMD(do_energize);
ACMD(do_lightgrenade);
ACMD(do_strike);
ACMD(do_ram);
ACMD(do_breath);


/* act.comm.c */


// commands
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_respond);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);
ACMD(do_voice);
ACMD(do_languages);
ACMD(do_osay);

/* act.informative.c */
// defines
#define MAX_PORTAL_TYPES        6
/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG		0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2

#define HIST_LENGTH 100

// global variables
extern int *cmd_sort_info;

// functions
int readIntro(struct char_data *ch, struct char_data *vict);
void introCreate(struct char_data *ch);
int check_disabled(const struct command_info *command);
void sort_commands(void);
char *find_exdesc(char *word, struct extra_descr_data *list);
void add_history(struct char_data *ch, char *str, int type);
void introCreate(struct char_data *ch);
void introWrite(struct char_data *ch, struct char_data *vict, char *name);
void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief);
int perf_skill(int skill);
int search_help(const char *argument, int level);

// commands
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_status);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
ACMD(do_commands);
ACMD(do_exits);
ACMD(do_autoexit);
ACMD(do_history);
ACMD(do_map);
ACMD(do_rptrans);
ACMD(do_finger);
ACMD(do_perf);
ACMD(do_nickname);
ACMD(do_table);
ACMD(do_play);
ACMD(do_post);
ACMD(do_hand);
ACMD(do_shuffle);
ACMD(do_draw);
ACMD(do_kyodaika);
ACMD(do_mimic);
ACMD(do_rpbank);
ACMD(do_rdisplay);
ACMD(do_evolve);
ACMD(do_rpbanktrans);
ACMD(do_showoff);
ACMD(do_intro);
ACMD(do_scan);
ACMD(do_toplist);
ACMD(do_whois);

/* act.item.c */
// global variables
extern struct obj_data *obj_selling;
extern struct char_data *ch_selling, *ch_buying;

// functions
int check_saveroom_count(struct char_data *ch, struct obj_data *cont);
void dball_load(void);
int check_insidebag(struct obj_data *cont, double mult);
int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void weight_change_object(struct obj_data *obj, int weight);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_remove(struct char_data *ch, int pos);
cl_sint64 max_carry_weight(struct char_data *ch);
void stop_auction(int type, struct char_data * ch);
void check_auction(void);

// commands
ACMD(do_split);
ACMD(do_auction);
ACMD(do_bid);
ACMD(do_assemble);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_twohand);
ACMD(do_deploy);
ACMD(do_pack);
ACMD(do_garden);
ACMD(do_refuel);
ACMD(do_sac);

/* act.misc.c */
void handle_multi_merge(struct char_data *form);
void handle_songs(void);
void fish_update(void);
void disp_rpp_store(struct char_data *ch);
void handle_rpp_store(struct char_data *ch, int choice);
void rpp_feature(struct char_data *ch, const char *arg);
void ash_burn(struct char_data *ch);

// commands
ACMD(do_transform);
ACMD(do_follow);
ACMD(do_spoil);
ACMD(do_feed);
ACMD(do_beacon);
ACMD(do_dimizu);
ACMD(do_obstruct);
ACMD(do_warppool);
ACMD(do_fireshield);
ACMD(do_cook);
ACMD(do_adrenaline);
ACMD(do_ensnare);
ACMD(do_arena);
ACMD(do_bury);
ACMD(do_hayasa);
ACMD(do_instill);
ACMD(do_kanso);
ACMD(do_hydromancy);
ACMD(do_channel);
ACMD(do_shimmer);
ACMD(do_metamorph);
ACMD(do_amnisiac);
ACMD(do_healglow);
ACMD(do_resize);
ACMD(do_scry);
ACMD(do_runic);
ACMD(do_extract);
ACMD(do_fish);
ACMD(do_defend);
ACMD(do_lifeforce);
ACMD(do_liquefy);
ACMD(do_shell);
ACMD(do_moondust);
ACMD(do_preference);
ACMD(do_song);
ACMD(do_multiform);
ACMD(do_spiritcontrol);
ACMD(do_ashcloud);
ACMD(do_silk);
ACMD(do_tailhide);
ACMD(do_nogrow);
ACMD(do_restring);

/* act.movement.c */

// global variables
const char *cmd_door[NUM_DOOR_CMD];

// functions
void handle_teleport(struct char_data *ch, struct char_data *tar, int location);
void dismount_char(struct char_data *ch);
void mount_char(struct char_data *ch, struct char_data *mount);
int land_location(struct char_data *ch, char *arg);
void carry_drop(struct char_data *ch, int type);
int has_o2(struct char_data *ch);
int do_simple_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move(struct char_data *ch, int dir, int need_specials_check);

// commands
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_fly);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);
ACMD(do_flee);
ACMD(do_carry);
ACMD(do_land);

/* act.offensive.c */

// commands
ACMD(do_assist);
ACMD(do_kill);
ACMD(do_flee);
ACMD(do_charge);
ACMD(do_punch);
ACMD(do_kick);
ACMD(do_elbow);
ACMD(do_knee);
ACMD(do_powerup);
ACMD(do_roundhouse);
ACMD(do_tailwhip);
ACMD(do_uppercut);
ACMD(do_kiball);
ACMD(do_kiblast);
ACMD(do_beam);
ACMD(do_bite);
ACMD(do_heeldrop);
ACMD(do_attack);
ACMD(do_attack2);
ACMD(do_renzo);
ACMD(do_kamehameha);
ACMD(do_masenko);
ACMD(do_dodonpa);
ACMD(do_galikgun);
ACMD(do_deathbeam);
ACMD(do_eraser);
ACMD(do_tslash);
ACMD(do_psyblast);
ACMD(do_honoo);
ACMD(do_dualbeam);
ACMD(do_rogafufuken);
ACMD(do_baku);
ACMD(do_kienzan);
ACMD(do_tribeam);
ACMD(do_sbc);
ACMD(do_final);
ACMD(do_crusher);
ACMD(do_ddslash);
ACMD(do_pbarrage);
ACMD(do_hellflash);
ACMD(do_hellspear);
ACMD(do_kakusanha);
ACMD(do_scatter);
ACMD(do_bigbang);
ACMD(do_pslash);
ACMD(do_deathball);
ACMD(do_spiritball);
ACMD(do_genki);
ACMD(do_geno);
ACMD(do_kousengan);
ACMD(do_balefire);
ACMD(do_blessedhammer);
ACMD(do_shogekiha);
ACMD(do_tsuihidan);
ACMD(do_slam);
ACMD(do_rescue);


/* act.other.c  */

// variables
extern const room_vnum freeres[NUM_ALIGNS];

// functions
void log_imm_action(char *messg, ...);
void hint_system(struct char_data *ch, int num);
int dball_count(struct char_data *ch);
void log_custom(struct descriptor_data *d, struct obj_data *obj);
void wishSYS(void);
void bring_to_cap(struct char_data *ch);
void base_update(void);
void log_custom(struct descriptor_data *d, struct obj_data *obj);
void load_shadow_dragons();
void innate_remove(struct char_data * ch, struct innate_node * inn);
void innate_add(struct char_data * ch, int innate, int timer);
int is_innate(struct char_data *ch, int spellnum);
int is_innate_ready(struct char_data *ch, int spellnum);
void add_innate_timer(struct char_data *ch, int spellnum);
void add_innate_affects(struct char_data *ch);
void update_innate(struct char_data *ch);

// commands
ACMD(do_gen_comm);
ACMD(do_charge);
ACMD(do_wear);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_value);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_file);
ACMD(do_scribe);
ACMD(do_pagelength);
ACMD(do_scouter);
ACMD(do_snet);
ACMD(do_spar);
ACMD(do_pushup);
ACMD(do_situp);
ACMD(do_transform);
ACMD(do_summon);
ACMD(do_instant);
ACMD(do_barrier);
ACMD(do_heal);
ACMD(do_solar);
ACMD(do_eyec);
ACMD(do_zanzoken);
ACMD(do_eavesdrop);
ACMD(do_disguise);
ACMD(do_appraise);
ACMD(do_forgery);
ACMD(do_plant);
ACMD(do_kaioken);
ACMD(do_focus);
ACMD(do_regenerate);
ACMD(do_escape);
ACMD(do_absorb);
ACMD(do_ingest);
ACMD(do_upgrade);
ACMD(do_srepair);
ACMD(do_recharge);
ACMD(do_form);
ACMD(do_spit);
ACMD(do_majinize);
ACMD(do_potential);
ACMD(do_telepathy);
ACMD(do_fury);
ACMD(do_pose);
ACMD(do_implant);
ACMD(do_hass);
ACMD(do_suppress);
ACMD(do_drag);
ACMD(do_stop);
ACMD(do_future);
ACMD(do_candy);
ACMD(do_kura);
ACMD(do_taisha);
ACMD(do_paralyze);
ACMD(do_infuse);
ACMD(do_rip);
ACMD(do_train);
ACMD(do_trip);
ACMD(do_grapple);
ACMD(do_willpower);
ACMD(do_commune);
ACMD(do_rpp);
ACMD(do_meditate);
ACMD(do_aura);
ACMD(do_think);
ACMD(do_block);
ACMD(do_visible);
ACMD(do_compare);
ACMD(do_break);
ACMD(do_fix);
ACMD(do_resurrect);
ACMD(do_clan);
ACMD(do_aid);

/* act.social.c */

// functions
void boot_social_messages(void);
void free_social_messages(void);
void free_action(struct social_messg *mess);
void free_command_list(void);
char *fread_action(FILE *fl, int nr);

// commands
ACMD(do_action);
ACMD(do_insult);
ACMD(do_gmote);

/* act.wizard.c */

/* global variables */


// functions
void search_replace(char *string, const char *find, const char *replace);
void update_space(void);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
void perform_immort_vis(struct char_data *ch);
void snoop_check(struct char_data *ch);


// commands
ACMD(do_echo);
ACMD(do_send);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
ACMD(do_stat);
ACMD(do_shutdown);
ACMD(do_recall);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
ACMD(do_show);
ACMD(do_set);
ACMD(do_saveall);
ACMD(do_wizupdate);
ACMD(do_chown);
ACMD(do_zpurge);
ACMD(do_zcheck);
ACMD(do_checkloadstatus);
ACMD(do_spells);
ACMD(do_finddoor);
ACMD(do_interest);
ACMD(do_transobj);
ACMD(do_permission);
ACMD(do_reward);
ACMD(do_approve);
ACMD(do_newsedit);
ACMD(do_news);
ACMD(do_lag);
ACMD(do_rbank);
ACMD(do_hell);
ACMD(do_varstat);
ACMD(do_handout);
ACMD(do_ginfo);
ACMD(do_plist);
ACMD(do_peace);
ACMD(do_raise);
ACMD(do_boom);


#endif //CIRCLE_ACT_H
