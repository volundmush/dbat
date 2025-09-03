#pragma once
#include <cstdint>

enum class RoomFlag : std::uint8_t
{
    dark = 0, // Dark
    // death_trap = 1,            // Death trap
    no_mobiles = 2,              // MOBs not allowed
    indoors = 3,                 // Indoors
    peaceful = 4,                // Violence not allowed
    soundproof = 5,              // Shouts, gossip blocked
    no_track = 6,                // Track won't go through
    no_instant_transmission = 7, // IT not allowed
    tunnel = 8,                  // room for only 1 pers
    private_room = 9,            // Can't teleport in
    god_room = 10,               // LVL_GOD+ only allowed
    house = 11,                  // (R) Room is a house
    // house_crash = 12,     // (R) House needs saving
    atrium = 13, // (R) The door to a house
    olc = 14,    // (R) Modifyable/!compress
    // bfs_mark = 15,        // (R) breath-first srch mrk
    vehicle = 16,     // Requires a vehicle to pass
    underground = 17, // Room is below ground
    // timed_deathtrap = 19,        // Room has a timed death trap
    punishment_hell = 28, // Room is Punishment Hell
    regen = 29,           // Better regen
    clan_bank = 35,       // This room is a clan bank
    ship = 36,            // This room is a private ship room
    healing_aura = 40,    // This room has an aura around it
    bedroom = 57,         // +25% regen
    workout = 58,         // Workout Room
    garden_1 = 59,        // 8 plant garden
    garden_2 = 60,        // 20 plant garden
    fertile_1 = 61,       // (unspecified)
    fertile_2 = 62,       // (unspecified)
    fishing = 63,         // (unspecified)
    fishfresh = 64,       // (unspecified)
    can_remodel = 65,     // (unspecified)
    // save = 67,            // room saves contents
};



/* Room flags: used in Room.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
constexpr int ROOM_DARK = 0;         /* Dark			*/
constexpr int ROOM_DEATH = 1;        /* Death trap		*/
constexpr int ROOM_NOMOB = 2;        /* MOBs not allowed		*/
constexpr int ROOM_INDOORS = 3;      /* Indoors			*/
constexpr int ROOM_PEACEFUL = 4;     /* Violence not allowed	*/
constexpr int ROOM_SOUNDPROOF = 5;   /* Shouts, gossip blocked	*/
constexpr int ROOM_NOTRACK = 6;      /* Track won't go through	*/
constexpr int ROOM_NOINSTANT = 7;    /* IT not allowed		*/
constexpr int ROOM_TUNNEL = 8;       /* room for only 1 pers	*/
constexpr int ROOM_PRIVATE = 9;      /* Can't teleport in		*/
constexpr int ROOM_GODROOM = 10;     /* LVL_GOD+ only allowed	*/
constexpr int ROOM_HOUSE = 11;       /* (R) Room is a house	*/
constexpr int ROOM_ATRIUM = 13;      /* (R) The door to a house	*/
constexpr int ROOM_OLC = 14;         /* (R) Modifyable/!compress	*/
constexpr int ROOM_VEHICLE = 16;     /* Requires a vehicle to pass       */
constexpr int ROOM_UNDERGROUND = 17; /* Room is below ground      */
constexpr int ROOM_HELL = 28;        /* Room is Punishment Hell*/
constexpr int ROOM_REGEN = 29;       /* Better regen */
constexpr int ROOM_CBANK = 35;       /* This room is a clan bank */
constexpr int ROOM_SHIP = 36;        /* This room is a private ship room */
constexpr int ROOM_AURA = 40;        /* This room has an aura around it  */
constexpr int ROOM_BEDROOM = 57;     /* +25% regen                       */
constexpr int ROOM_WORKOUT = 58;     /* Workout Room                     */
constexpr int ROOM_GARDEN1 = 59;     /* 8 plant garden                   */
constexpr int ROOM_GARDEN2 = 60;     /* 20 plant garden                  */
constexpr int ROOM_FERTILE1 = 61;
constexpr int ROOM_FERTILE2 = 62;
constexpr int ROOM_FISHING = 63;
constexpr int ROOM_FISHFRESH = 64;
constexpr int ROOM_CANREMODEL = 65;

