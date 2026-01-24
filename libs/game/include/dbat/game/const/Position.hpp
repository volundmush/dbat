#pragma once
#include <cstdint>

enum class Position : std::uint8_t {
    Dead = 0,
    Mortally_Wounded = 1,
    Incapacitated = 2,
    Stunned = 3,
    Sleeping = 4,
    Resting = 5,
    Sitting = 6,
    Fighting = 7,
    Standing = 8
};


/* Positions */
constexpr Position POS_DEAD = Position::Dead;      /* dead			*/
constexpr Position POS_MORTALLYW = Position::Mortally_Wounded; /* mortally wounded	*/
constexpr Position POS_INCAP = Position::Incapacitated;     /* incapacitated	*/
constexpr Position POS_STUNNED = Position::Stunned;   /* stunned		*/
constexpr Position POS_SLEEPING = Position::Sleeping;  /* sleeping		*/
constexpr Position POS_RESTING = Position::Resting;   /* resting		*/
constexpr Position POS_SITTING = Position::Sitting;   /* sitting		*/
constexpr Position POS_FIGHTING = Position::Fighting;  /* fighting		*/
constexpr Position POS_STANDING = Position::Standing;  /* standing		*/
