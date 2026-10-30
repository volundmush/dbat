#pragma once
/* Positions */
constexpr int POS_DEAD       = 0; /* dead			*/
constexpr int POS_MORTALLYW  = 1; /* mortally wounded	*/
constexpr int POS_INCAP      = 2; /* incapacitated	*/
constexpr int POS_STUNNED    = 3; /* stunned		*/
constexpr int POS_SLEEPING   = 4; /* sleeping		*/
constexpr int POS_RESTING    = 5; /* resting		*/
constexpr int POS_SITTING    = 6; /* sitting		*/
constexpr int POS_FIGHTING   = 7; /* fighting		*/
constexpr int POS_STANDING   = 8; /* standing		*/

constexpr int NUM_POSITIONS  = 9;

extern const char *position_types[NUM_POSITIONS+1];