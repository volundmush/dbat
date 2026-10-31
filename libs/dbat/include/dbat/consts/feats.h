#pragma once

/* Combat feats that apply to a specific weapon type */
constexpr int CFEAT_IMPROVED_CRITICAL             = 0;
constexpr int CFEAT_WEAPON_FINESSE                = 1;
constexpr int CFEAT_WEAPON_FOCUS                  = 2;
constexpr int CFEAT_WEAPON_SPECIALIZATION         = 3;
constexpr int CFEAT_GREATER_WEAPON_FOCUS          = 4;
constexpr int CFEAT_GREATER_WEAPON_SPECIALIZATION = 5;

constexpr int CFEAT_MAX = 5;

constexpr int MAX_FEATS         = 750;
constexpr int FT_ARRAY_MAX      = 4;
constexpr int NUM_FEATS_DEFINED = 252;