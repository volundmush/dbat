#include <span>
#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/attacks.hpp"

/* Combine Attack Names */
static constexpr auto attack_names_comp_entries = std::to_array<std::string_view>({
    "Kamehameha",          /* 0 Roshi */
    "Galik Gun",           /* 1 Bardock */
    "Masenko",             /* 2 Piccolo */
    "Deathbeam",           /* 3 Frieza */
    "Honoo",               /* 4 Dabura */
    "Twin Slash",          /* 5 Tapion */
    "Hell Flash",          /* 6 Sixteen */
    "Psychic Blast",       /* 7 Kibito */
    "Crusher Ball",        /* 8 Ginyu */
    "Water Spikes",        /* 9 Tsuna */
    "Tribeam",             /* 10 Krane */
    "Star Breaker",        /* 11 Jinto */
    "Seishou Enko",        /* 12 Arlian */
    "Renzokou Energy Dan", /* 13 Nail */
    });
static_assert(std::size(attack_names_comp_entries) == NUM_ATTACK_NAMES);
constinit const name_table attack_names_comp(attack_names_comp_entries);

/* Combine Attack Names */
static constexpr auto attack_names_entries = std::to_array<std::string_view>({
    "kamehameha",          /* 0 Roshi */
    "galik gun",           /* 1 Bardock */
    "masenko",             /* 2 Piccolo */
    "deathbeam",           /* 3 Frieza */
    "honoo",               /* 4 Dabura */
    "twin slash",          /* 5 Tapion */
    "hell flash",          /* 6 Sixteen */
    "psychic blast",       /* 7 Kibito */
    "crusher ball",        /* 8 Ginyu */
    "water spikes",        /* 9 Tsuna */
    "tribeam",             /* 10 Krane */
    "star breaker",        /* 11 Jinto */
    "seishou enko",        /* 12 Arlian */
    "renzokou energy dan", /* 13 Nail */
    });
static_assert(std::size(attack_names_entries) == NUM_ATTACK_NAMES);
constinit const name_table attack_names(attack_names_entries);

/* Combine Attack Skills */
static constexpr auto attack_skill_list = std::to_array<int>({464, 444, 441, 472, 476, 474, 488, 475, 485, 510, 482, 533, 528, 440});
const std::span<const int> attack_skills = attack_skill_list;
