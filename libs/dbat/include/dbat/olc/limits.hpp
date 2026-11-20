#pragma once

/* What OLC lets a builder set: tune them here */
constexpr int MAX_ROOM_NAME  = 100;
constexpr int MAX_ROOM_DESC  = 4096;
constexpr int MAX_EXIT_DESC  = 256;
constexpr int MAX_MOB_DESC   = 1024;
constexpr int MAX_DUPLICATES = 10000; /* when loading in zedit */
constexpr int MAX_FROM_ROOM  = 50;    /* when loading in zedit */

/* max weapon is 50d50 .. avg. 625 dam... */
constexpr int MAX_WEAPON_SDICE = 50;
constexpr int MAX_WEAPON_NDICE = 50;

constexpr int MAX_OBJ_WEIGHT     = 100000000;
constexpr int MAX_OBJ_COST       = 2000000;
constexpr int MAX_OBJ_RENT       = 2000000;
constexpr int MAX_CONTAINER_SIZE = 500000000;
constexpr int MAX_OBJ_TIMER      = 1071000; /* one mud year */

constexpr int MAX_MOB_GOLD = 1000000;
constexpr int MAX_MOB_EXP  = 1500000;
