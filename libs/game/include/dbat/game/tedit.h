#pragma once
#include "dbat/db/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

ACMD(do_tedit);
void tedit_string_cleanup(struct descriptor_data *d, int terminator);
void news_string_cleanup(struct descriptor_data *d, int terminator);


#ifdef __cplusplus
}
#endif
