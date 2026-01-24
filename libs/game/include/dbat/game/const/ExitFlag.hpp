#pragma once
#include <cstdint>

enum class ExitFlag : std::uint8_t {
    isdoor = 0,
    closed = 1,
    locked = 2,
    pickproof = 3,
    secret = 4
};

/* Exit info: used in Room.dir_option.exit_info */
constexpr ExitFlag EX_ISDOOR = ExitFlag::isdoor;    /* Exit is a door		*/
constexpr ExitFlag EX_CLOSED = ExitFlag::closed;    /* The door is closed	*/
constexpr ExitFlag EX_LOCKED = ExitFlag::locked;    /* The door is locked	*/
constexpr ExitFlag EX_PICKPROOF = ExitFlag::pickproof; /* Lock can't be picked	*/
constexpr ExitFlag EX_SECRET = ExitFlag::secret;    /* The door is hidden        */