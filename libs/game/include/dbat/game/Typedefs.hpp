#pragma once
#include <cstdint>

/* Basic system dependencies *******************************************/
#define IDXTYPE	int
#define NOWHERE -1	/* nil reference for rooms	*/
#define NOTHING NOWHERE
#define NOBODY NOWHERE
#define NOFLAG NOWHERE
#define NOBZOB NOWHERE

#define I64T "ld"
#define SZT "ld"
#define TMT "ld"

/* Various virtual (human-reference) number types. */
typedef IDXTYPE vnum;
typedef vnum room_vnum;
typedef vnum obj_vnum;
typedef vnum mob_vnum;
typedef vnum zone_vnum;
typedef vnum shop_vnum;
typedef vnum trig_vnum;
typedef vnum guild_vnum;

/* Various real (array-reference) number types. */
typedef vnum room_rnum;
typedef vnum obj_rnum;
typedef vnum mob_rnum;
typedef vnum zone_rnum;
typedef vnum shop_rnum;
typedef vnum trig_rnum;
typedef vnum guild_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 */
typedef std::uint32_t bitvector_t;
