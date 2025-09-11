#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "Typedefs.h"
#include "const/Position.h"
#include "const/AffectFlag.h"
#include "Flags.h"

struct Character;

/* Modifier constants used with obj affects ('A' fields) */
constexpr int APPLY_NONE = 0; /* No effect */

constexpr int APPLY_CATTR_BASE = 1;      /* Bitwise: Flat Modifier applied to Attribute base */
constexpr int APPLY_CATTR_MULT = 2;      /* Bitwise: Modifier for Attribute mult */
constexpr int APPLY_CATTR_POST = 3;      /* Bitwise: Flat modifier added after mult.  */
constexpr int APPLY_CATTR_GAIN_MULT = 4; /* bitwise: other stats gain multiplier */

constexpr int APPLY_CVIT_BASE = 5;       /* Bitwise: Flat modifier applied to vital base */
constexpr int APPLY_CVIT_MULT = 6;       /* Bitwise: modifier for vitals mult */
constexpr int APPLY_CVIT_POST = 7;       /* Bitwise: flat modifier applied after mult */
constexpr int APPLY_CVIT_GAIN_MULT = 8;  /* Bitwise: base gains multiplier */
constexpr int APPLY_CVIT_REGEN_MULT = 9; /* Bitwise: regen multiplier */
constexpr int APPLY_CVIT_DOT_MULT = 10;  /* Bitwise: damage over time multiplier */

constexpr int APPLY_CSTAT_BASE = 11;      /* bitwise: other stats base */
constexpr int APPLY_CSTAT_MULT = 12;      /* bitwise: other stats multiplier */
constexpr int APPLY_CSTAT_POST = 13;      /* bitwise: flat modifier applied after mult */
constexpr int APPLY_CSTAT_GAIN_MULT = 14; /* bitwise: other stats gain multiplier */

constexpr int APPLY_CDIM_BASE = 15; /* bitwise: character dimension base */
constexpr int APPLY_CDIM_MULT = 16; /* bitwise: character dimension multiplier */
constexpr int APPLY_CDIM_POST = 17; /* bitwise: flat modifier applied after mult */

constexpr int APPLY_COMBAT_BASE = 18; /* bitwise: combat base bonuses */
constexpr int APPLY_COMBAT_MULT = 19; /* bitwise: combat multiplicative bonuses */

constexpr int APPLY_DTYPE_RES = 20; /* bitwise: damage type resistance */
constexpr int APPLY_DTYPE_BON = 21; /* bitwise: damage type bonuses */

constexpr int APPLY_ATKTIER_RES = 22;       /* bitwise: attack tier resistance */
constexpr int APPLY_ATKTIER_BON = 23;       /* bitwise: attack tier bonuses */
constexpr int APPLY_TRANS_UPKEEP_CVIT = 24; /* bitwise: cvitals cost modifier for transformations */

constexpr int APPLY_SKILL = 25; /* !bitwise: Apply to a specific skill    */

constexpr int APPLY_CDER_BASE = 26; /* bitwise: character derived base */
constexpr int APPLY_CDER_MULT = 27; /* bitwise: character derived multiplier */
constexpr int APPLY_CDER_POST = 28; /* bitwise: character derived post multiplier */

constexpr int CDER_CARRY_CAPACITY = 1 << 0;


struct affect_t {
    // DO NOT CHANGE THE ORDER OF THESE FIELDS.
    explicit affect_t() = default;
    affect_t(int loc, double mod, int spec) : location(loc), modifier(mod), specific(spec) {};
    uint64_t location{0};
    double modifier{0.0};
    uint64_t specific{0};
    std::vector<std::string> specificNames();
    std::string locName();
    bool isBitwise();
    bool match(int loc, int spec);
    bool isPercent();
};

struct character_affect_type : affect_t {
    std::function<double(struct Character *ch)> func{};

    character_affect_type(int loc, double mod, int spec, std::function<double(struct Character *ch)> f = {})
            : affect_t{loc, mod, spec}, func{f} {}
};

/* An affect structure. */
struct affected_type : affect_t {
    using affect_t::affect_t;
    int16_t type{};          /* The type of spell that caused this      */
    int16_t duration{};      /* For how long its effects will last      */
    FlagHandler<AffectFlag> aff_flags{}; /* Tells which bits to set (AFF_XXX) */
    struct affected_type *next{};
};

extern std::unordered_map<Position, std::vector<character_affect_type>> pos_affects;