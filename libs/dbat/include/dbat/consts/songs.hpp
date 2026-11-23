#pragma once
#include "dbat/name_table.hpp"
#include <string_view>

/* Ocarina Songs */
constexpr int SONG_SAFETY           = 1;
constexpr int SONG_SHIELDING        = 2;
constexpr int SONG_SHADOW_STITCH    = 3;
constexpr int SONG_TELEPORT_EARTH   = 4;
constexpr int SONG_TELEPORT_KONACK  = 5;
constexpr int SONG_TELEPORT_ARLIA   = 6;
constexpr int SONG_TELEPORT_NAMEK   = 7;
constexpr int SONG_TELEPORT_VEGETA  = 8;
constexpr int SONG_TELEPORT_FRIGID  = 9;
constexpr int SONG_TELEPORT_AETHER  = 10;
constexpr int SONG_TELEPORT_KANASSA = 11;

constexpr int NUM_SONG_NAMES = 13; /* by the song played; quirk: 0 and 12 are "\n" */
extern const name_table song_types; /* NUM_SONG_NAMES of them */

