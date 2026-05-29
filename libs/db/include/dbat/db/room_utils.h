#pragma once
#include "consts/types.h"
#include "flags.h"

#ifdef __cplusplus
extern "C" {
#endif


#define IS_DARK(room)	room_is_dark((room))
#define IS_LIGHT(room)  (!room_is_dark(room))

/* Minor Planet Defines */
    
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))

#define R_EXIT(room, num)     ((room)->dir_option[(num)])

#ifdef __cplusplus
}
#endif
