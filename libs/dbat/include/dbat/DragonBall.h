#pragma once
#include <unordered_set>

#include "Typedefs.h"
struct Character;

constexpr int SHADOW_DRAGON1_VNUM = 81;
constexpr int SHADOW_DRAGON2_VNUM = 82;
constexpr int SHADOW_DRAGON3_VNUM = 83;
constexpr int SHADOW_DRAGON4_VNUM = 84;
constexpr int SHADOW_DRAGON5_VNUM = 85;
constexpr int SHADOW_DRAGON6_VNUM = 86;
constexpr int SHADOW_DRAGON7_VNUM = 87;

extern Character *EDRAGON;
extern int WISH[2];
extern int DRAGONR, DRAGONZ, DRAGONC, SHENRON;

extern int dballtime;

extern std::unordered_set<obj_vnum> dbVnums;

extern int SELFISHMETER, SHADOW_DRAGON1, SHADOW_DRAGON2, SHADOW_DRAGON3, SHADOW_DRAGON4, SHADOW_DRAGON5;
extern int SHADOW_DRAGON6, SHADOW_DRAGON7;

extern int DBALL_HUNTER1, DBALL_HUNTER2, DBALL_HUNTER3, DBALL_HUNTER4;
extern int DBALL_HUNTER1_VNUM, DBALL_HUNTER2_VNUM, DBALL_HUNTER3_VNUM, DBALL_HUNTER4_VNUM;