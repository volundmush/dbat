// Added pragma once to avoid multiple inclusion redefinition errors.
#pragma once
#include "defs.h"

template<typename T>
bool OBJWEAR_FLAGGED(T *obj, int flag) {
    return obj->wear_flags.get(static_cast<WearFlag>(flag));
}

template<typename T>
int64_t GET_OBJ_VAL(T* obj, const std::string& val) {
    return obj->getBaseStat(val);
}
template<typename T>
int64_t SET_OBJ_VAL(T* obj, const std::string& val, int newval) {
    return obj->setBaseStat(val, newval);
}
template<typename T>
int64_t MOD_OBJ_VAL(T* obj, const std::string& val, int mod) {
    return obj->modBaseStat(val, mod);
}

extern bool OBJAFF_FLAGGED(Object *obj, int flag);
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), VAL_CONTAINER_FLAGS), (flag)))

extern bool OBJ_FLAGGED(Object *obj, int flag);
