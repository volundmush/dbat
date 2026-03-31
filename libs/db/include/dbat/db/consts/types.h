#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define CIRCLE_UNSIGNED_INDEX 0	/* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE	uint16_t
#define NOTHING	((IDXTYPE)~0)
#else
#define IDXTYPE	int
#define NOTHING	-1	/* nil reference for objects	*/
#endif

#define NOWHERE NOTHING
#define NOBODY NOTHING
#define NOFLAG NOTHING

#define I64T "ld"
#define SZT "ld"
#define TMT "ld"

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;
typedef IDXTYPE obj_vnum;
typedef IDXTYPE mob_vnum;
typedef IDXTYPE zone_vnum;
typedef IDXTYPE shop_vnum;
typedef IDXTYPE trig_vnum;
typedef IDXTYPE guild_vnum;

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;
typedef IDXTYPE obj_rnum;
typedef IDXTYPE mob_rnum;
typedef IDXTYPE zone_rnum;
typedef IDXTYPE shop_rnum;
typedef IDXTYPE trig_rnum;
typedef IDXTYPE guild_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 */
typedef uint32_t bitvector_t;

#define FALSE false
#define TRUE true

struct char_data;
struct obj_data;
struct room_data;
struct zone_data;
struct descriptor_data;
struct obj_affected_type;
struct affected_type;
struct ban_list_element;
struct board_msg;
struct board_memory;
struct board_info;
struct command_info;
struct alias_data;
struct time_data;
struct abil_data;
struct player_special_data;
struct guild_data;
struct shop_data;
struct shop_buy_data;
struct extra_descr_data;
struct help_index_element;
struct player_index_element;
struct social_messg;
struct weather_data;
struct time_info_data;
struct reset_com;

typedef struct char_data char_data;
typedef struct obj_data obj_data;
typedef struct room_data room_data;
typedef struct zone_data zone_data;
typedef struct descriptor_data descriptor_data;
typedef struct obj_affected_type obj_affected_type;
typedef struct affected_type affected_type;
typedef struct ban_list_element ban_list_element;
typedef struct board_msg board_msg;
typedef struct board_memory board_memory;
typedef struct board_info board_info;
typedef struct command_info command_info;
typedef struct alias_data alias_data;
typedef struct time_data time_data;
typedef struct abil_data abil_data;
typedef struct player_special_data player_special_data;
typedef struct guild_data guild_data;
typedef struct shop_data shop_data;
typedef struct shop_buy_data shop_buy_data;
typedef struct extra_descr_data extra_descr_data;
typedef struct help_index_element help_index_element;
typedef struct player_index_element player_index_element;
typedef struct social_messg social_messg;
typedef struct weather_data weather_data;
typedef struct time_info_data time_info_data;
typedef struct reset_com reset_com;


#define ACMD(name) void (name)(struct char_data *ch, char *argument, int cmd, int subcmd)
#define SPECIAL(name) int (name)(struct char_data *ch, void *me, int cmd, char *argument)
