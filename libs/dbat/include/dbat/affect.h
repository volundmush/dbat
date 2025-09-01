#pragma once
#include "sysdep.h"

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
    bitvector_t bitvector{}; /* Tells which bits to set (AFF_XXX) */
    struct affected_type *next{};
};