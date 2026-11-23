#pragma once
#include "dbat/name_table.hpp"
#include <string_view>

/* Character Creation Styles */
constexpr int WIELD_NONE    = 0;
constexpr int WIELD_LIGHT   = 1;
constexpr int WIELD_ONEHAND = 2;
constexpr int WIELD_TWOHAND = 3;

/* Number of weapon types */
constexpr int MAX_WEAPON_TYPES = 26;

/* Critical hit types */
constexpr int CRIT_X2 = 0;
constexpr int CRIT_X3 = 1;
constexpr int CRIT_X4 = 2;

constexpr int MAX_CRIT_TYPE  = CRIT_X4;
constexpr int NUM_CRIT_TYPES = 3;

constexpr int MAX_ARMOR_TYPES = 5;
constexpr int NUM_WIELD_NAMES = 4;

extern const name_table weapon_type; /* MAX_WEAPON_TYPES + 1 of them */
extern const name_table armor_type; /* MAX_ARMOR_TYPES of them */
extern const name_table wield_names; /* NUM_WIELD_NAMES of them */
extern const name_table crit_type; /* NUM_CRIT_TYPES of them */

