#include "Object.hpp"
#include "ObjectPrototype.hpp"

void auto_equip(Character *ch, Object *obj, int location);

#define VALID_OBJ_RNUM(obj)    obj_proto.contains(GET_OBJ_RNUM(obj))

#define GET_OBJ_LEVEL(obj)      obj->getBaseStat<int>("level")
#define GET_OBJ_PERM(obj)       obj->affect_flags
#define GET_OBJ_TYPE(obj)    obj->type_flag
#define GET_OBJ_COST(obj)    obj->getBaseStat<int>("cost")
#define GET_OBJ_RENT(obj)    obj->getBaseStat<int>("cost_per_day")
#define GET_OBJ_EXTRA(obj)    obj->item_flags
#define GET_OBJ_EXTRA_AR(obj, i)   obj->item_flags.get(i)
#define GET_OBJ_WEAR(obj)    obj->wear_flags
#define GET_OBJ_WEIGHT(obj)    obj->getBaseStat("weight")
#define GET_OBJ_TIMER(obj)    obj->getBaseStat<int>("timer")
#define SITTING(obj)            obj->sitting.lock().get()
#define GET_OBJ_POSTTYPE(obj)   obj->posttype
#define GET_OBJ_POSTED(obj)     obj->posted_to
#define GET_FELLOW_WALL(obj)    obj->fellow_wall
#define FOOB(obj)               obj->getBaseStat<int>("foob")
/* Below is used for "homing" ki attacks */
#define TARGET(obj)             obj->target
#define KICHARGE(obj)           obj->getBaseStat("kicharge")
#define KITYPE(obj)             obj->getBaseStat<int>("kitype")
#define USER(obj)               obj->user
#define KIDIST(obj)             obj->getBaseStat<int>("distance")
/* Above is used for "homing ki attacks */
#define SFREQ(obj)              obj->getBaseStat<int>("scoutfreq")
#define HCHARGE(obj)            GET_OBJ_VAL(obj, VAL_BED_HTANK_CHARGE)
#define GET_LAST_LOAD(obj)      obj->getBaseStat<time_t>("lload")
#define GET_OBJ_SIZE(obj)    static_cast<int>(obj->size)
#define GET_OBJ_RNUM(obj)    obj->getVnum()
#define GET_OBJ_VNUM(obj)    obj->getVnum()

inline SpecialFunc GET_OBJ_SPEC(Object* obj) {
    if(auto find = obj_proto.find(obj->getVnum()); find != obj_proto.end())
        return find->second->func;
    return nullptr;
}

#define GET_FUEL(obj)           GET_OBJ_VAL(obj, VAL_VEHICLE_FUEL)
#define GET_FUELCOUNT(obj)      GET_OBJ_VAL(obj, VAL_VEHICLE_FUELCOUNT)

#define IS_CORPSE(obj)        (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
                    GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) == 1)

#define CAN_WEAR(obj, part)    OBJWEAR_FLAGGED(obj, part)
#define GET_OBJ_MATERIAL(obj)   GET_OBJ_VAL(obj, VAL_ALL_MATERIAL)
#define GET_OBJ_SHORT(obj)    obj->getShortDescription()

extern bool wearable_obj(Object *obj);

extern void randomize_eq(Object *obj);

#define OBJS(obj, vict) (vict->canSee(obj) ? obj->getShortDescription()  : "something")

#define OBJN(obj, vict) (vict->canSee(obj) ? fname(obj->getName()) : "something")

extern int wield_type(int chsize, Object *weap);

Object *create_obj();

Object *read_object(obj_vnum nr, int type);

extern std::vector<std::weak_ptr<Object>> getAllObjects();

extern void auc_load(Object *obj);