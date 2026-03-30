#pragma once
/* Distinguishing Feature */
#define DISTFEA_EYE             0
#define DISTFEA_HAIR            1
#define DISTFEA_SKIN            2
#define DISTFEA_HEIGHT          3
#define DISTFEA_WEIGHT          4

/* Custom Aura */
#define AURA_WHITE              0
#define AURA_BLUE               1
#define AURA_RED                2
#define AURA_GREEN              3
#define AURA_PINK               4
#define AURA_PURPLE             5
#define AURA_YELLOW             6
#define AURA_BLACK              7
#define AURA_ORANGE             8

/* Eye Color */
#define EYE_UNDEFINED           -1
#define EYE_BLUE                0
#define EYE_BLACK               1
#define EYE_GREEN               2
#define EYE_BROWN               3
#define EYE_RED                 4
#define EYE_AQUA                5
#define EYE_PINK                6
#define EYE_PURPLE              7
#define EYE_CRIMSON             8
#define EYE_GOLD                9
#define EYE_AMBER               10
#define EYE_EMERALD             11

/*Hair Length */
#define HAIRL_UNDEFINED         -1
#define HAIRL_BALD              0
#define HAIRL_SHORT             1
#define HAIRL_MEDIUM            2
#define HAIRL_LONG              3
#define HAIRL_RLONG             4


/*Hair Color */
#define HAIRC_UNDEFINED         -1
#define HAIRC_NONE              0
#define HAIRC_BLACK             1
#define HAIRC_BROWN             2
#define HAIRC_BLONDE            3
#define HAIRC_GREY              4
#define HAIRC_RED               5
#define HAIRC_ORANGE            6
#define HAIRC_GREEN             7
#define HAIRC_BLUE              8
#define HAIRC_PINK              9
#define HAIRC_PURPLE            10
#define HAIRC_SILVER            11
#define HAIRC_CRIMSON           12
#define HAIRC_WHITE             13

/* Hair Style */
#define HAIRS_UNDEFINED         -1
#define HAIRS_NONE              0
#define HAIRS_PLAIN             1
#define HAIRS_MOHAWK            2
#define HAIRS_SPIKY             3
#define HAIRS_CURLY             4
#define HAIRS_UNEVEN            5
#define HAIRS_PONYTAIL          6
#define HAIRS_AFRO              7
#define HAIRS_FADE              8
#define HAIRS_CREW              9
#define HAIRS_FEATHERED         10
#define HAIRS_DRED              11


/* Skin Color */
#define SKIN_UNDEFINED          -1
#define SKIN_WHITE              0
#define SKIN_BLACK              1
#define SKIN_GREEN              2
#define SKIN_ORANGE             3
#define SKIN_YELLOW             4
#define SKIN_RED                5
#define SKIN_GREY               6
#define SKIN_BLUE               7
#define SKIN_AQUA               8
#define SKIN_PINK               9
#define SKIN_PURPLE             10
#define SKIN_TAN                11

/* Annual Sign Phase */

#define PHASE_PURITY            0
#define PHASE_BRAVERY           1
#define PHASE_HATRED            2
#define PHASE_DOMINANCE         3
#define PHASE_GUARDIAN          4
#define PHASE_LOVE              5
#define PHASE_STRENGTH          6



extern const char *hairl_types[];
extern const char *eye_types[];
extern const char *FHA_types[];
extern const char *hairc_types[];
extern const char *hairs_types[];
extern const char *skin_types[];
extern const char *aura_types[];