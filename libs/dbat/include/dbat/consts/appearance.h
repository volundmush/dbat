#pragma once
/* Distinguishing Feature */
constexpr int DISTFEA_EYE    = 0;
constexpr int DISTFEA_HAIR   = 1;
constexpr int DISTFEA_SKIN   = 2;
constexpr int DISTFEA_HEIGHT = 3;
constexpr int DISTFEA_WEIGHT = 4;

/* Custom Aura */
constexpr int AURA_WHITE  = 0;
constexpr int AURA_BLUE   = 1;
constexpr int AURA_RED    = 2;
constexpr int AURA_GREEN  = 3;
constexpr int AURA_PINK   = 4;
constexpr int AURA_PURPLE = 5;
constexpr int AURA_YELLOW = 6;
constexpr int AURA_BLACK  = 7;
constexpr int AURA_ORANGE = 8;

/* Eye Color */
constexpr int EYE_UNDEFINED = -1;
constexpr int EYE_BLUE      = 0;
constexpr int EYE_BLACK     = 1;
constexpr int EYE_GREEN     = 2;
constexpr int EYE_BROWN     = 3;
constexpr int EYE_RED       = 4;
constexpr int EYE_AQUA      = 5;
constexpr int EYE_PINK      = 6;
constexpr int EYE_PURPLE    = 7;
constexpr int EYE_CRIMSON   = 8;
constexpr int EYE_GOLD      = 9;
constexpr int EYE_AMBER     = 10;
constexpr int EYE_EMERALD   = 11;

/*Hair Length */
constexpr int HAIRL_UNDEFINED = -1;
constexpr int HAIRL_BALD      = 0;
constexpr int HAIRL_SHORT     = 1;
constexpr int HAIRL_MEDIUM    = 2;
constexpr int HAIRL_LONG      = 3;
constexpr int HAIRL_RLONG     = 4;

/*Hair Color */
constexpr int HAIRC_UNDEFINED = -1;
constexpr int HAIRC_NONE      = 0;
constexpr int HAIRC_BLACK     = 1;
constexpr int HAIRC_BROWN     = 2;
constexpr int HAIRC_BLONDE    = 3;
constexpr int HAIRC_GREY      = 4;
constexpr int HAIRC_RED       = 5;
constexpr int HAIRC_ORANGE    = 6;
constexpr int HAIRC_GREEN     = 7;
constexpr int HAIRC_BLUE      = 8;
constexpr int HAIRC_PINK      = 9;
constexpr int HAIRC_PURPLE    = 10;
constexpr int HAIRC_SILVER    = 11;
constexpr int HAIRC_CRIMSON   = 12;
constexpr int HAIRC_WHITE     = 13;

/* Hair Style */
constexpr int HAIRS_UNDEFINED = -1;
constexpr int HAIRS_NONE      = 0;
constexpr int HAIRS_PLAIN     = 1;
constexpr int HAIRS_MOHAWK    = 2;
constexpr int HAIRS_SPIKY     = 3;
constexpr int HAIRS_CURLY     = 4;
constexpr int HAIRS_UNEVEN    = 5;
constexpr int HAIRS_PONYTAIL  = 6;
constexpr int HAIRS_AFRO      = 7;
constexpr int HAIRS_FADE      = 8;
constexpr int HAIRS_CREW      = 9;
constexpr int HAIRS_FEATHERED = 10;
constexpr int HAIRS_DRED      = 11;

/* Skin Color */
constexpr int SKIN_UNDEFINED = -1;
constexpr int SKIN_WHITE     = 0;
constexpr int SKIN_BLACK     = 1;
constexpr int SKIN_GREEN     = 2;
constexpr int SKIN_ORANGE    = 3;
constexpr int SKIN_YELLOW    = 4;
constexpr int SKIN_RED       = 5;
constexpr int SKIN_GREY      = 6;
constexpr int SKIN_BLUE      = 7;
constexpr int SKIN_AQUA      = 8;
constexpr int SKIN_PINK      = 9;
constexpr int SKIN_PURPLE    = 10;
constexpr int SKIN_TAN       = 11;

/* Annual Sign Phase */

constexpr int PHASE_PURITY    = 0;
constexpr int PHASE_BRAVERY   = 1;
constexpr int PHASE_HATRED    = 2;
constexpr int PHASE_DOMINANCE = 3;
constexpr int PHASE_GUARDIAN  = 4;
constexpr int PHASE_LOVE      = 5;
constexpr int PHASE_STRENGTH  = 6;

extern const char *hairl_types[];
extern const char *eye_types[];
extern const char *FHA_types[];
extern const char *hairc_types[];
extern const char *hairs_types[];
extern const char *skin_types[];
extern const char *aura_types[];