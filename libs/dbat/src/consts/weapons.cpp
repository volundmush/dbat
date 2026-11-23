#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/weapons.hpp"

/* Armor Types */
static constexpr std::string_view armor_type_entries[] = {
    "Undefined",
    "Light",
    "Medium",
    "Heavy",
    "Shield"};
static_assert(std::size(armor_type_entries) == MAX_ARMOR_TYPES);
constinit const name_table armor_type(armor_type_entries);

/* Weapon Types */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static constexpr std::string_view weapon_type_entries[] = {
    "undefined",
    "unarmed",
    "dagger",
    "mace",
    "sickle",
    "spear",
    "staff",
    "crossbow",
    "longbow",
    "shortbow",
    "sling",
    "shuriken",
    "hammer",
    "lance",
    "flail",
    "longsword",
    "shortsword",
    "greatsword",
    "rapier",
    "scimitar",
    "polearm",
    "club",
    "bastard sword",
    "monk weapon",
    "double weapon",
    "axe",
    "whip"};
static_assert(std::size(weapon_type_entries) == MAX_WEAPON_TYPES + 1);
constinit const name_table weapon_type(weapon_type_entries);

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static constexpr std::string_view crit_type_entries[] = {
        "x2",
        "x3",
        "x4"};
static_assert(std::size(crit_type_entries) == NUM_CRIT_TYPES);
constinit const name_table crit_type(crit_type_entries);

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static constexpr std::string_view wield_names_entries[] = {
    "if you were bigger",
    "with ease",
    "one-handed",
    "two-handed"};
static_assert(std::size(wield_names_entries) == NUM_WIELD_NAMES);
constinit const name_table wield_names(wield_names_entries);
