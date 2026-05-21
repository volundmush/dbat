#pragma once
#include "consts/types.h"
#include "flags.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SECT(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].sector_type : SECT_INSIDE)
#define ROOM_DAMAGE(room)   (world[(room)].dmg)
#define ROOM_EFFECT(room)   (world[(room)].geffect)
#define ROOM_GRAVITY(room)  (world[(room)].gravity)
#define SUNKEN(room)    (ROOM_EFFECT(room) < 0 || SECT(room) == SECT_UNDERWATER)

#define IS_DARK(room)	room_is_dark((room))
#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) \
	(VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)

/* Minor Planet Defines */
#define PLANET_ZENITH(room) ((GET_ROOM_VNUM(room) >= 3400 && GET_ROOM_VNUM(room) <= 3599) || (GET_ROOM_VNUM(room) >= 62900 && GET_ROOM_VNUM(room) <= 62999) || \
				(GET_ROOM_VNUM(room) == 19600))
    
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
#define ROOM_FLAGGED(loc, flag) (IS_SET_AR(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))

#define W_EXIT(room, num)     (world[(room)].dir_option[(num)])
#define R_EXIT(room, num)     ((room)->dir_option[(num)])

#ifdef __cplusplus
}
#endif
