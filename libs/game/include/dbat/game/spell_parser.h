#pragma once
#include "dbat/db/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SINFO spell_info[spellnum]
extern const char *unused_spellname;

// Commands
ACMD(do_cast);


#ifdef __cplusplus
}
#endif
