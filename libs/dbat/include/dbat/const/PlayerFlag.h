#pragma once
#include <cstdint>

enum class PlayerFlag : uint8_t
{
    player_killer = 0,        // Player is a player-killer
    player_thief = 1,         // Player is a player-thief
    frozen = 2,               // Player is frozen
    writing = 4,              // Player writing (board/mail/olc)
    mailing = 5,              // Player is writing mail
    site_ok = 7,              // Player has been site-cleared
    no_shout = 8,             // Player not allowed to shout/goss
    no_title = 9,             // Player not allowed to set title
    loadroom = 11,            // Player uses nonstandard loadroom
    no_wizlist = 12,          // Player shouldn't be on wizlist
    no_delete = 13,           // Player shouldn't be deleted
    wiz_invisible_start = 14, // Player should enter game wizinvis
    not_dead_yet = 16,        // (R) Player being extracted.
    piloting = 31,            // Player is sitting in the pilots chair
    skillp = 32,              // Player made a good choice in CC
    charging = 34,            // Player is charging
    knocked_out = 45,         // Knocked OUT
    immortal = 51,            // The player is immortal
    eyes_closed = 52,         // The player has their eyes closed
    disguised = 53,           // The player is disguised
    bandaged = 54,            // The player has been bandaged
    healing_tank = 56,        // Is inside a healing tank
    halfbreed_fury = 57,      // Is in fury mode
    ginyu_fighting_pose = 58, // Ginyu Pose Effect
    absorbed = 60,
    killed_by_player = 62,
    two_hand_wielding = 63,
    self_destruct_1 = 64,
    self_destruct_2 = 65,
    spiral = 66,
    biography_approved = 67,
    repair_learn = 69,
    forgetting_skill = 70,
    transmission = 71,
    fishing = 72,
    majin_goop_state = 73,
    multi_hit = 74,
    aura_light = 75,
    room_display = 76,
    stolen = 77,
    tail_hide = 78,     // Hides tail for S & HB
    no_regrow_tail = 79 // Halt Growth for S & HB
};

/* Player flags: used by Character.act */
constexpr PlayerFlag PLR_KILLER = PlayerFlag::player_killer;      /* Player is a player-killer        */
constexpr PlayerFlag PLR_THIEF = PlayerFlag::player_thief;       /* Player is a player-thief         */
constexpr PlayerFlag PLR_FROZEN = PlayerFlag::frozen;            /* Player is frozen                 */
constexpr PlayerFlag PLR_WRITING = PlayerFlag::writing;          /* Player writing (board/mail/olc)  */
constexpr PlayerFlag PLR_MAILING = PlayerFlag::mailing;          /* Player is writing mail           */
constexpr PlayerFlag PLR_NOSHOUT = PlayerFlag::no_shout;        /* Player not allowed to shout/goss */
constexpr PlayerFlag PLR_NOTITLE = PlayerFlag::no_title;        /* Player not allowed to set title  */
constexpr PlayerFlag PLR_DELETED = PlayerFlag::no_delete;       /* Player deleted - space reusable  */
constexpr PlayerFlag PLR_LOADROOM = PlayerFlag::loadroom;      /* Player uses nonstandard loadroom */
constexpr PlayerFlag PLR_NOWIZLIST = PlayerFlag::no_wizlist;   /* Player shouldn't be on wizlist  	*/
constexpr PlayerFlag PLR_NODELETE = PlayerFlag::no_delete;     /* Player shouldn't be deleted     	*/
constexpr PlayerFlag PLR_INVSTART = PlayerFlag::wiz_invisible_start;      /* Player should enter game wizinvis*/

constexpr PlayerFlag PLR_NOTDEADYET = PlayerFlag::not_dead_yet; /* (R) Player being extracted.     	*/
constexpr PlayerFlag PLR_PILOTING = PlayerFlag::piloting;      /* Player is sitting in the pilots chair */
constexpr PlayerFlag PLR_SKILLP = PlayerFlag::skillp;          /* Player made a good choice in CC  */
constexpr PlayerFlag PLR_CHARGE = PlayerFlag::charging;        /* Player is charging               */
constexpr PlayerFlag PLR_KNOCKED = PlayerFlag::knocked_out;    /* Knocked OUT                      */

constexpr PlayerFlag PLR_IMMORTAL = PlayerFlag::immortal;      /* The player is immortal           */
constexpr PlayerFlag PLR_EYEC = PlayerFlag::eyes_closed;      /* The player has their eyes closed */
constexpr PlayerFlag PLR_DISGUISED = PlayerFlag::disguised;  /* The player is disguised          */
constexpr PlayerFlag PLR_BANDAGED = PlayerFlag::bandaged;   /* THe player has been bandaged     */
constexpr PlayerFlag PLR_HEALT = PlayerFlag::healing_tank;      /* Is inside a healing tank         */
constexpr PlayerFlag PLR_FURY = PlayerFlag::halfbreed_fury;       /* Is in fury mode                  */
constexpr PlayerFlag PLR_POSE = PlayerFlag::ginyu_fighting_pose;       /* Ginyu Pose Effect                */
constexpr PlayerFlag PLR_ABSORBED = PlayerFlag::absorbed;
constexpr PlayerFlag PLR_PDEATH = PlayerFlag::killed_by_player;
constexpr PlayerFlag PLR_THANDW = PlayerFlag::two_hand_wielding;
constexpr PlayerFlag PLR_SELFD = PlayerFlag::self_destruct_1;
constexpr PlayerFlag PLR_SELFD2 = PlayerFlag::self_destruct_2;
constexpr PlayerFlag PLR_SPIRAL = PlayerFlag::spiral;
constexpr PlayerFlag PLR_BIOGR = PlayerFlag::biography_approved;
constexpr PlayerFlag PLR_REPLEARN = PlayerFlag::repair_learn;
constexpr PlayerFlag PLR_FORGET = PlayerFlag::forgetting_skill;
constexpr PlayerFlag PLR_TRANSMISSION = PlayerFlag::transmission;
constexpr PlayerFlag PLR_FISHING = PlayerFlag::fishing;
constexpr PlayerFlag PLR_GOOP = PlayerFlag::majin_goop_state;
constexpr PlayerFlag PLR_MULTIHIT = PlayerFlag::multi_hit;
constexpr PlayerFlag PLR_AURALIGHT = PlayerFlag::aura_light;
constexpr PlayerFlag PLR_RDISPLAY = PlayerFlag::room_display;
constexpr PlayerFlag PLR_STOLEN = PlayerFlag::stolen;
constexpr PlayerFlag PLR_TAILHIDE = PlayerFlag::tail_hide; /* Hides tail for S & HB            */
constexpr PlayerFlag PLR_NOGROW = PlayerFlag::no_regrow_tail;   /* Halt Growth for S & HB           */
