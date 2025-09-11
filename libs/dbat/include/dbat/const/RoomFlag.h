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
constexpr RoomFlag ROOM_DARK = RoomFlag::dark;         /* Dark			*/
//constexpr RoomFlag ROOM_DEATH = RoomFlag::death_trap;        /* Death trap		*/
constexpr RoomFlag ROOM_NOMOB = RoomFlag::no_mobiles;        /* MOBs not allowed		*/
constexpr RoomFlag ROOM_INDOORS = RoomFlag::indoors;      /* Indoors			*/
constexpr RoomFlag ROOM_PEACEFUL = RoomFlag::peaceful;     /* Violence not allowed	*/
constexpr RoomFlag ROOM_SOUNDPROOF = RoomFlag::soundproof;   /* Shouts, gossip blocked	*/
constexpr RoomFlag ROOM_NOTRACK = RoomFlag::no_track;      /* Track won't go through	*/
constexpr RoomFlag ROOM_NOINSTANT = RoomFlag::no_instant_transmission;    /* IT not allowed		*/
constexpr RoomFlag ROOM_TUNNEL = RoomFlag::tunnel;       /* room for only 1 pers	*/
constexpr RoomFlag ROOM_PRIVATE = RoomFlag::private_room;      /* Can't teleport in		*/
constexpr RoomFlag ROOM_GODROOM = RoomFlag::god_room;     /* LVL_GOD+ only allowed	*/
constexpr RoomFlag ROOM_HOUSE = RoomFlag::house;       /* (R) Room is a house	*/
constexpr RoomFlag ROOM_ATRIUM = RoomFlag::atrium;      /* (R) The door to a house	*/
constexpr RoomFlag ROOM_OLC = RoomFlag::olc;         /* (R) Modifyable/!compress	*/
constexpr RoomFlag ROOM_VEHICLE = RoomFlag::vehicle;     /* Requires a vehicle to pass       */
constexpr RoomFlag ROOM_UNDERGROUND = RoomFlag::underground; /* Room is below ground      */
constexpr RoomFlag ROOM_HELL = RoomFlag::punishment_hell;        /* Room is Punishment Hell*/
constexpr RoomFlag ROOM_REGEN = RoomFlag::regen;       /* Better regen */
constexpr RoomFlag ROOM_CBANK = RoomFlag::clan_bank;       /* This room is a clan bank */
constexpr RoomFlag ROOM_SHIP = RoomFlag::ship;        /* This room is a private ship room */
constexpr RoomFlag ROOM_AURA = RoomFlag::healing_aura;        /* This room has an aura around it  */
constexpr RoomFlag ROOM_BEDROOM = RoomFlag::bedroom;     /* +25% regen                       */
constexpr RoomFlag ROOM_WORKOUT = RoomFlag::workout;     /* Workout Room                     */
constexpr RoomFlag ROOM_GARDEN1 = RoomFlag::garden_1;     /* 8 plant garden                   */
constexpr RoomFlag ROOM_GARDEN2 = RoomFlag::garden_2;     /* 20 plant garden                  */
constexpr RoomFlag ROOM_FERTILE1 = RoomFlag::fertile_1;
constexpr RoomFlag ROOM_FERTILE2 = RoomFlag::fertile_2;
constexpr RoomFlag ROOM_FISHING = RoomFlag::fishing;
constexpr RoomFlag ROOM_FISHFRESH = RoomFlag::fishfresh;
constexpr RoomFlag ROOM_CANREMODEL = RoomFlag::can_remodel;

