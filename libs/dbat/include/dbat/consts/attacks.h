#pragma once

extern const char *attack_names_comp[];
extern const char *attack_names[];
extern const int attack_skills[];

constexpr int TYPE_HIT      = 300;
constexpr int TYPE_STING    = 301;
constexpr int TYPE_WHIP     = 302;
constexpr int TYPE_SLASH    = 303;
constexpr int TYPE_BITE     = 304;
constexpr int TYPE_BLUDGEON = 305;
constexpr int TYPE_CRUSH    = 306;
constexpr int TYPE_POUND    = 307;
constexpr int TYPE_CLAW     = 308;
constexpr int TYPE_MAUL     = 309;
constexpr int TYPE_THRASH   = 310;
constexpr int TYPE_PIERCE   = 311;
constexpr int TYPE_BLAST    = 312;
constexpr int TYPE_PUNCH    = 313;
constexpr int TYPE_STAB     = 314;
/* new attack types can be added here - up to TYPE_SUFFERING */
constexpr int TYPE_SUFFERING = 399;