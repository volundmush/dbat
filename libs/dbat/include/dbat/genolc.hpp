#pragma once
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/db.hpp"

constexpr int STRING_TERMINATOR = '~';

/* The save list's zones for the socials and the help, which have no zone of their own */
constexpr zone_vnum AEDIT_PERMISSION = 999; /* arbitrary number higher than max zone vnum*/
constexpr zone_vnum HEDIT_PERMISSION = 888; /* arbitrary number higher then max zone vnum*/


/* Shop and guild messages are builder text used as a printf format: one %s for the customer's name, plus one %d
 * for the price when allow_int is set (shop buy/sell). Returns NULL if msg is safe to use that way, else why not. */
const char *npc_msg_format_problem(std::string_view msg, bool allow_int);
/* msg, a message npc_msg_format_problem() passed, as printf made it: its %s the customer's name (a null one
 * "(null)"), its %d the amount, and %% a % */
std::string npc_message(std::string_view msg, const char *name, int amount = 0);

bool remove_from_save_list(GameWorld *g, zone_vnum, int type);
bool add_to_save_list(GameWorld *g, zone_vnum, int type);
bool in_save_list(GameWorld *g, zone_vnum, int type);
void strip_cr(std::string &text);
bool save_all(GameWorld *g);
/* text, or "undefined" for none, as OLC stores a name or description left empty */
std::string text_or_undefined(std::string_view text);
std::string text_or_undefined(const char *text);
/* Reads a number typed for a float field as sscanf("%f") did: the field is left alone unless text starts with one */
void scan_float(std::string_view text, float &field);
void olc_list_columns(GameWorld *g, struct descriptor_data *d, const char *const *names, int count, int first_number, const char *gap, int columns);
/* bits as the world files write them: a letter for each bit set, a-z then A-F, or "0" for none */
std::string flag_letters(bitvector_t bits);

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

room_vnum genolc_zone_bottom(GameWorld *g, struct zone_data *rznum);
room_vnum genolc_zonep_bottom(struct zone_data *zone);
void create_world_index(GameWorld *g, zone_vnum znum, std::string_view type);

