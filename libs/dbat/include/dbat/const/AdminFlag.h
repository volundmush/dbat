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
constexpr int ADM_TELLALL = 0;       /* Can use 'tell all' to broadcast GOD */
constexpr int ADM_SEEINV = 1;        /* Sees other chars inventory IMM */
constexpr int ADM_SEESECRET = 2;     /* Sees secret doors IMM */
constexpr int ADM_KNOWWEATHER = 3;   /* Knows details of weather GOD */
constexpr int ADM_FULLWHERE = 4;     /* Full output of 'where' command IMM */
constexpr int ADM_MONEY = 5;         /* Char has a bottomless wallet GOD */
constexpr int ADM_EATANYTHING = 6;   /* Char can eat anything GOD */
constexpr int ADM_NOPOISON = 7;      /* Char can't be poisoned IMM */
constexpr int ADM_WALKANYWHERE = 8;  /* Char has unrestricted walking IMM */
constexpr int ADM_NOKEYS = 9;        /* Char needs no keys for locks GOD */
constexpr int ADM_INSTANTKILL = 10;  /* "kill" command is instant IMPL */
constexpr int ADM_NOSTEAL = 11;      /* Char cannot be stolen from IMM */
constexpr int ADM_TRANSALL = 12;     /* Can use 'trans all' GRGOD */
constexpr int ADM_SWITCHMORTAL = 13; /* Can 'switch' to a mortal PC body IMPL */
constexpr int ADM_FORCEMASS = 14;    /* Can force rooms or all GRGOD */
constexpr int ADM_ALLHOUSES = 15;    /* Can enter any house GRGOD */
constexpr int ADM_NODAMAGE = 16;     /* Cannot be damaged IMM */
constexpr int ADM_ALLSHOPS = 17;     /* Can use all shops GOD */
constexpr int ADM_CEDIT = 18;        /* Can use cedit IMPL */
