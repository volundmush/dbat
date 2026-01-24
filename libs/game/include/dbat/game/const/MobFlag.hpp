#pragma once
#include <cstdint>


enum class MobFlag : uint8_t
{
    special_proc = 0,        // Mob has a callable spec-proc
    sentinel = 1,            // Mob should not move
    no_scavenger = 2,        // Mob won't pick up items from rooms
    aware = 4,               // Mob can't be backstabbed
    aggressive = 5,          // Mob auto-attacks everybody nearby
    stay_zone = 6,           // Mob shouldn't wander out of zone
    wimpy = 7,               // Mob flees if severely injured
    aggressive_evil = 8,     // Auto-attack any evil PC's
    aggressive_good = 9,     // Auto-attack any good PC's
    aggressive_neutral = 10, // Auto-attack any neutral PC's
    memory = 11,             // remember attackers if attacked
    helper = 12,             // attack PCs fighting other NPCs
    no_charm = 13,           // Mob can't be charmed
    no_summon = 14,          // Mob can't be summoned
    no_sleep = 15,           // Mob can't be slept
    autobalance = 16,        // Mob stats autobalance
    no_blind = 17,           // Mob can't be blinded
    no_kill = 18,            // Mob can't be killed
    not_dead_yet = 19,       // (R) Mob being extracted.
    mountable = 20,          // Mob is mountable.
    justdesc = 26,           // Mob doesn't use auto desc
    husk = 27,               // Is an extracted Husk
    dummy = 29,              // This mob will not fight back
    no_poison = 32,          // No poison
    know_kaioken = 33,       // Knows kaioken
};

/* Mobile flags: used by Character.act */
constexpr int MOB_SPEC = 0;          /* Mob has a callable spec-proc   	*/
constexpr int MOB_SENTINEL = 1;      /* Mob should not move            	*/
constexpr int MOB_NOSCAVENGER = 2;   /* Mob won't pick up items from rooms*/
constexpr int MOB_AWARE = 4;         /* Mob can't be backstabbed          */
constexpr int MOB_AGGRESSIVE = 5;    /* Mob auto-attacks everybody nearby	*/
constexpr int MOB_STAY_ZONE = 6;     /* Mob shouldn't wander out of zone  */
constexpr int MOB_WIMPY = 7;         /* Mob flees if severely injured  	*/
constexpr int MOB_AGGR_EVIL = 8;     /* Auto-attack any evil PC's		*/
constexpr int MOB_AGGR_GOOD = 9;     /* Auto-attack any good PC's      	*/
constexpr int MOB_AGGR_NEUTRAL = 10; /* Auto-attack any neutral PC's   	*/
constexpr int MOB_MEMORY = 11;       /* remember attackers if attacked    */
constexpr int MOB_HELPER = 12;       /* attack PCs fighting other NPCs    */
constexpr int MOB_NOCHARM = 13;      /* Mob can't be charmed         	*/
constexpr int MOB_NOSUMMON = 14;     /* Mob can't be summoned             */
constexpr int MOB_NOSLEEP = 15;      /* Mob can't be slept           	*/
constexpr int MOB_AUTOBALANCE = 16;  /* Mob stats autobalance		*/
constexpr int MOB_NOBLIND = 17;      /* Mob can't be blinded         	*/
constexpr int MOB_NOKILL = 18;       /* Mob can't be killed               */
constexpr int MOB_NOTDEADYET = 19;   /* (R) Mob being extracted.          */
constexpr int MOB_MOUNTABLE = 20;    /* Mob is mountable.			*/
constexpr int MOB_JUSTDESC = 26;     /* Mob doesn't use auto desc         */
constexpr int MOB_HUSK = 27;         /* Is an extracted Husk              */
constexpr int MOB_DUMMY = 29;        /* This mob will not fight back      */
constexpr int MOB_NOPOISON = 32;     /* No poison                         */
constexpr int MOB_KNOWKAIO = 33;     /* Knows kaioken                     */

