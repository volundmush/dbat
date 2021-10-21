//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_ACT_H
#define CIRCLE_ACT_H

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "clan.h"
#include "combat.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "boards.h"
#include "feats.h"
#include "clan.h"
#include "maputils.h"
#include "vehicles.h"
#include "guild.h"
#include "spells.h"
#include "oasis.h"
#include "feats.h"
#include "assemblies.h"

/* act.attack.c */

// Commands
ACMD(do_get);
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
extern struct *obj_selling;
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



/* act.other.c  */

// functions
void log_imm_action(char *messg, ...);

/* act.social.c */

// functions
void boot_social_messages(void);
void free_social_messages(void);
void free_action(struct social_messg *mess);
void free_command_list(void);

// commands
ACMD(do_action);
ACMD(do_insult);
ACMD(do_gmote);

/* act.wizard.c */


// functions
void search_replace(char *string, const char *find, const char *replace);

#endif //CIRCLE_ACT_H
