#pragma once
#include <cstdint>


enum class AdminFlag : uint8_t
{
    tell_all = 0,       // Can use 'tell all' to broadcast GOD
    see_invisible = 1,  // Sees other chars inventory IMM
    see_secret = 2,     // Sees secret doors IMM
    know_weather = 3,   // Knows details of weather GOD
    full_where = 4,     // Full output of 'where' command IMM
    money = 5,          // Char has a bottomless wallet GOD
    eat_anything = 6,   // Char can eat anything GOD
    no_poison = 7,      // Char can't be poisoned IMM
    walk_anywhere = 8,  // Char has unrestricted walking IMM
    no_keys = 9,        // Char needs no keys for locks GOD
    instant_kill = 10,  // "kill" command is instant IMPL
    no_steal = 11,      // Char cannot be stolen from IMM
    trans_all = 12,     // Can use 'trans all' GRGOD
    switch_mortal = 13, // Can 'switch' to a mortal PC body IMPL
    force_mass = 14,    // Can force rooms or all GRGOD
    all_houses = 15,    // Can enter any house GRGOD
    no_damage = 16,     // Cannot be damaged IMM
    all_shops = 17,     // Can use all shops GOD
    cedit = 18          // Can use cedit IMPL
};

/*
 * ADM flags - define admin privs for chars
 */
constexpr AdminFlag ADM_TELLALL = AdminFlag::tell_all;       /* Can use 'tell all' to broadcast GOD */
constexpr AdminFlag ADM_SEEINV = AdminFlag::see_invisible;        /* Sees other chars inventory IMM */
constexpr AdminFlag ADM_SEESECRET = AdminFlag::see_secret;     /* Sees secret doors IMM */
constexpr AdminFlag ADM_KNOWWEATHER = AdminFlag::know_weather;   /* Knows details of weather GOD */
constexpr AdminFlag ADM_FULLWHERE = AdminFlag::full_where;     /* Full output of 'where' command IMM */
constexpr AdminFlag ADM_MONEY = AdminFlag::money;         /* Char has a bottomless wallet GOD */
constexpr AdminFlag ADM_EATANYTHING = AdminFlag::eat_anything;   /* Char can eat anything GOD */
constexpr AdminFlag ADM_NOPOISON = AdminFlag::no_poison;      /* Char can't be poisoned IMM */
constexpr AdminFlag ADM_WALKANYWHERE = AdminFlag::walk_anywhere;  /* Char has unrestricted walking IMM */
constexpr AdminFlag ADM_NOKEYS = AdminFlag::no_keys;        /* Char needs no keys for locks GOD */
constexpr AdminFlag ADM_INSTANTKILL = AdminFlag::instant_kill;  /* "kill" command is instant IMPL */
constexpr AdminFlag ADM_NOSTEAL = AdminFlag::no_steal;      /* Char cannot be stolen from IMM */
constexpr AdminFlag ADM_TRANSALL = AdminFlag::trans_all;     /* Can use 'trans all' GRGOD */
constexpr AdminFlag ADM_SWITCHMORTAL = AdminFlag::switch_mortal; /* Can 'switch' to a mortal PC body IMPL */
constexpr AdminFlag ADM_FORCEMASS = AdminFlag::force_mass;    /* Can force rooms or all GRGOD */
constexpr AdminFlag ADM_ALLHOUSES = AdminFlag::all_houses;    /* Can enter any house GRGOD */
constexpr AdminFlag ADM_NODAMAGE = AdminFlag::no_damage;     /* Cannot be damaged IMM */
constexpr AdminFlag ADM_ALLSHOPS = AdminFlag::all_shops;     /* Can use all shops GOD */
constexpr AdminFlag ADM_CEDIT = AdminFlag::cedit;        /* Can use cedit IMPL */
