#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SEARCH_WORKING (1 << 0)
#define SEARCH_GENUINE (1 << 1)
#define SEARCH_HOT (1 << 2)

struct obj_vnum_search_data {
    obj_vnum vnum;
    int flags;
    // char_data is an optional searcher, for visibility checks. If null, all objects are considered visible.
    struct char_data *ch; 
    struct obj_data *found;
};

struct obj_type_search_data {
    int type;
    int flags;
    struct char_data *ch;
    struct obj_data *found;
};

#ifdef __cplusplus
}
#endif