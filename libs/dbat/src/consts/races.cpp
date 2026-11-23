#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/sizes.hpp"

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */

static constexpr std::string_view race_names_entries[] = {
    "human",
    "saiyan",
    "icer",
    "konatsu",
    "namekian",
    "mutant",
    "kanassan",
    "halfbreed",
    "bioandroid",
    "android",
    "demon",
    "majin",
    "kai",
    "truffle",
    "hoshijin",
    "animal",
    "saiba",
    "serpent",
    "ogre",
    "yardratian",
    "arlian",
    "dragon",
    "mechanical",
    "spirit"};
static_assert(std::size(race_names_entries) == NUM_RACES);
constinit const name_table race_names(race_names_entries);

static constexpr std::string_view race_abbrevs_entries[] = {
    "Hum",
    "Sai",
    "Ice",
    "Kon",
    "Nam",
    "Mut",
    "Kan",
    "H-B",
    "Bio",
    "And",
    "Dem",
    "Maj",
    "Kai",
    "Tru",
    "Hos",
    "Ict",
    "Sab",
    "Ser",
    "Trl",
    "Dra",
    "Arl",
    "Mnd",
    "Mec",
    "Spi"};
static_assert(std::size(race_abbrevs_entries) == NUM_RACES);
constinit const name_table race_abbrevs(race_abbrevs_entries);

static constexpr std::string_view pc_race_types_entries[] = {
    "Human",
    "Saiyan",
    "Icer",
    "Konatsu",
    "Namekian",
    "Mutant",
    "Kanassan",
    "Halfbreed",
    "Bioandroid",
    "Android",
    "Demon",
    "Majin",
    "Kai",
    "Truffle",
    "Hoshijin",
    "animal",
    "Saiba",
    "Serpent",
    "Ogre",
    "Yardratian",
    "Arlian",
    "Dragon",
    "mechanical",
    "Spirit"};
static_assert(std::size(pc_race_types_entries) == NUM_RACES);
constinit const name_table pc_race_types(pc_race_types_entries);
