#include "Room.hpp"


extern bool WHERE_FLAGGED(room_vnum loc, WhereFlag flag);
extern bool WHERE_FLAGGED(Room *loc, WhereFlag flag);
extern bool ROOM_FLAGGED(room_vnum loc, RoomFlag flag);
extern bool ROOM_FLAGGED(Room *loc, RoomFlag flag);

#define SECT(room)    (VALID_ROOM_RNUM(room) ? get_room(room)->sector_type : SECT_INSIDE)
#define ROOM_DAMAGE(room)   get_room(room)->getDamage()
#define ROOM_EFFECT(room)   get_room(room)->geffect
#define ROOM_GRAVITY(room)  get_room(room)->getGravity()
#define SUNKEN(room)    (ROOM_EFFECT(room) < 0 || SECT(room) == SECT_UNDERWATER)

#define IS_DARK(room)    room_is_dark(room)
#define IS_LIGHT(room)  !IS_DARK(room)

#define VALID_ROOM_RNUM(rnum)    (Room::registry.contains(rnum) > 0 && rnum != NOWHERE)
#define GET_ROOM_VNUM(rnum) (VALID_ROOM_RNUM(rnum) ? (rnum) : NOWHERE)
#define GET_ROOM_SPEC(room) \
    (VALID_ROOM_RNUM(room) ? get_room(room)->func : nullptr)

    /* Minor Planet Defines */
#define PLANET_ZENITH(room) ((GET_ROOM_VNUM(room) >= 3400 && GET_ROOM_VNUM(room) <= 3599) || (GET_ROOM_VNUM(room) >= 62900 && GET_ROOM_VNUM(room) <= 62999) || \
                (GET_ROOM_VNUM(room) == 19600))

#define ROOM_FLAGS(loc)    Room::registry.at(loc).room_flags

extern const char* sense_location_name(room_vnum roomnum);

extern Room *get_room(room_vnum vn);



extern void repairRoomDamage(uint64_t heartPulse, double deltaTime);

extern room_rnum real_room(room_vnum vnum);