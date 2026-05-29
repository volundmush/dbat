#pragma once
#include "consts/types.h"
#include "flags.h"

#ifdef __cplusplus
extern "C" {
#endif


#define IS_DARK(room)	room_is_dark((room))
#define IS_LIGHT(room)  (!room_is_dark(room))

#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))

/* Minor Planet Defines */
    
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
#define ROOM_FLAGGED(loc, flag) (IS_SET_AR(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))

#define R_EXIT(room, num)     ((room)->dir_option[(num)])

#ifdef __cplusplus
}
#endif
