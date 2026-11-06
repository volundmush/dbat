#pragma once
#include "dbat/consts/types.h"
#include "db.h"

#ifdef __cplusplus
extern "C" {
#endif

constexpr int STRING_TERMINATOR = '~';

#define CONFIG_GENOLC_MOBPROG 0

bool genolc_checkstring(struct descriptor_data *d, char *arg);

/* Shop and guild messages are builder text used as a printf format: one %s for the customer's name, plus one %d
 * for the price when allow_int is set (shop buy/sell). Returns NULL if msg is safe to use that way, else why not. */
const char *npc_msg_format_problem(const char *msg, bool allow_int);

bool remove_from_save_list(GameWorld *g, zone_vnum, int type);
bool add_to_save_list(GameWorld *g, zone_vnum, int type);
bool in_save_list(GameWorld *g, zone_vnum, int type);
void strip_cr(char *);
bool save_all(GameWorld *g);
char *str_udup(const char *);
void olc_list_columns(GameWorld *g, struct descriptor_data *d, const char *const *names, int count, int first_number, const char *gap, int columns);
void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from);
void free_ex_descriptions(struct extra_descr_data *head);
int sprintascii(char *out, bitvector_t bits);
void free_save_list(GameWorld *g);

struct save_list_data {
  zone_vnum zone;
  int type;
  struct save_list_data *next;
};

/* save_list_data.type */
constexpr int SL_MOB = 0;
constexpr int SL_OBJ = 1;
constexpr int SL_SHP = 2;
constexpr int SL_WLD = 3;
constexpr int SL_ZON = 4;
constexpr int SL_CFG = 5;
constexpr int SL_GLD = 6;
constexpr int SL_MAX = SL_GLD;
constexpr int SL_ACT = SL_MAX + 1; /* must be above MAX */
constexpr int SL_HLP = SL_MAX + 2; /* must be above MAX */

#define LIMIT(var, low, high) MIN(high, MAX(var, low))

room_vnum genolc_zone_bottom(GameWorld *g, zone_rnum rznum);
room_vnum genolc_zonep_bottom(struct zone_data *zone);
void create_world_index(GameWorld *g, zone_vnum znum, const char *type);

#ifdef __cplusplus
}
#endif
