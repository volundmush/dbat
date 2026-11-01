#pragma once
/*
 * ADM flags - define admin privs for chars
 */
constexpr int ADM_TELLALL      = 0;  /* Can use 'tell all' to broadcast GOD */
constexpr int ADM_SEEINV       = 1;  /* Sees other chars inventory IMM */
constexpr int ADM_SEESECRET    = 2;  /* Sees secret doors IMM */
constexpr int ADM_KNOWWEATHER  = 3;  /* Knows details of weather GOD */
constexpr int ADM_FULLWHERE    = 4;  /* Full output of 'where' command IMM */
constexpr int ADM_MONEY        = 5;  /* Char has a bottomless wallet GOD */
constexpr int ADM_EATANYTHING  = 6;  /* Char can eat anything GOD */
constexpr int ADM_NOPOISON     = 7;  /* Char can't be poisoned IMM */
constexpr int ADM_WALKANYWHERE = 8;  /* Char has unrestricted walking IMM */
constexpr int ADM_NOKEYS       = 9;  /* Char needs no keys for locks GOD */
constexpr int ADM_INSTANTKILL  = 10; /* "kill" command is instant IMPL */
constexpr int ADM_NOSTEAL      = 11; /* Char cannot be stolen from IMM */
constexpr int ADM_TRANSALL     = 12; /* Can use 'trans all' GRGOD */
constexpr int ADM_SWITCHMORTAL = 13; /* Can 'switch' to a mortal PC body IMPL */
constexpr int ADM_FORCEMASS    = 14; /* Can force rooms or all GRGOD */
constexpr int ADM_ALLHOUSES    = 15; /* Can enter any house GRGOD */
constexpr int ADM_NODAMAGE     = 16; /* Cannot be damaged IMM */
constexpr int ADM_ALLSHOPS     = 17; /* Can use all shops GOD */
constexpr int ADM_CEDIT        = 18; /* Can use cedit IMPL */

constexpr int NUM_ADMFLAGS = 19;
constexpr int AD_ARRAY_MAX = 4;