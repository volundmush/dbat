#pragma once
#include "consts/types.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "flags.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OBJAFF_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_PERM(obj), (flag)))
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), VAL_CONTAINER_FLAGS), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_WEAR(obj), (flag)))
#define OBJ_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_EXTRA(obj), (flag)))

#define VALID_OBJ_RNUM(obj)	(GET_OBJ_RNUM(obj) <= top_of_objt && \
				 GET_OBJ_RNUM(obj) != NOTHING)

#define GET_OBJ_LEVEL(obj)      ((obj)->level)
#define GET_OBJ_PERM(obj)       ((obj)->bitvector)
#define GET_OBJ_TYPE(obj)	((obj)->type_flag)
#define GET_OBJ_COST(obj)	((obj)->cost)
#define GET_OBJ_RENT(obj)	((obj)->cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->extra_flags)
#define GET_OBJ_EXTRA_AR(obj, i)   ((obj)->extra_flags[(i)])
#define GET_OBJ_WEAR(obj)	((obj)->wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->weight)
#define GET_OBJ_TIMER(obj)	((obj)->timer)
#define SITTING(obj)            ((obj)->sitting)
#define GET_OBJ_POSTTYPE(obj)   ((obj)->posttype)
#define GET_OBJ_POSTED(obj)     ((obj)->posted_to)
#define GET_FELLOW_WALL(obj)    ((obj)->fellow_wall)
#define GET_AUCTER(obj)         ((obj)->aucter)
#define GET_CURBID(obj)         ((obj)->curBidder)
#define GET_AUCTERN(obj)        ((obj)->auctname)
#define GET_AUCTIME(obj)        ((obj)->aucTime)
#define GET_BID(obj)            ((obj)->bid)
#define GET_STARTBID(obj)       ((obj)->startbid)
#define FOOB(obj)               ((obj)->foob)
/* Below is used for "homing" ki attacks */
#define TARGET(obj)             ((obj)->target)
#define KICHARGE(obj)           ((obj)->kicharge)
#define KITYPE(obj)             ((obj)->kitype)
#define USER(obj)               ((obj)->user)
#define KIDIST(obj)             ((obj)->distance)
/* Above is used for "homing ki attacks */
#define SFREQ(obj)              ((obj)->scoutfreq)
#define HCHARGE(obj)            ((obj)->healcharge)
#define GET_LAST_LOAD(obj)      ((obj)->lload)
#define GET_OBJ_SIZE(obj)	((obj)->size)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)
#define GET_OBJ_SPEC(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].func : NULL)
#define GET_FUEL(obj)           (GET_OBJ_VAL((obj), 2))
#define GET_FUELCOUNT(obj)      (GET_OBJ_VAL((obj), 3))

#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) == 1)

#define CAN_WEAR(obj, part)	OBJWEAR_FLAGGED((obj), (part))
#define GET_OBJ_MATERIAL(obj)   ((obj)->value[7])
#define GET_OBJ_SHORT(obj)	((obj)->short_description)


#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")
#define OBJ_LOADROOM(obj)     ((obj)->room_loaded)


#ifdef __cplusplus
}
#endif
