#pragma once

/* Races */
constexpr int RACE_UNDEFINED  = -1;
constexpr int RACE_HUMAN      = 0;
constexpr int RACE_SAIYAN     = 1;
constexpr int RACE_ICER       = 2;
constexpr int RACE_KONATSU    = 3;
constexpr int RACE_NAMEK      = 4;
constexpr int RACE_MUTANT     = 5;
constexpr int RACE_KANASSAN   = 6;
constexpr int RACE_HALFBREED  = 7;
constexpr int RACE_BIO        = 8;
constexpr int RACE_ANDROID    = 9;
constexpr int RACE_DEMON      = 10;
constexpr int RACE_MAJIN      = 11;
constexpr int RACE_KAI        = 12;
constexpr int RACE_TRUFFLE    = 13;
constexpr int RACE_HOSHIJIN   = 14;
constexpr int RACE_ANIMAL     = 15;
constexpr int RACE_SAIBA      = 16;
constexpr int RACE_SERPENT    = 17;
constexpr int RACE_OGRE       = 18;
constexpr int RACE_YARDRATIAN = 19;
constexpr int RACE_ARLIAN     = 20;
constexpr int RACE_DRAGON     = 21;
constexpr int RACE_MECHANICAL = 22;
constexpr int RACE_SPIRIT     = 23;

constexpr int NUM_RACES = 24;

struct aging_data {
  int adult;           /* Adulthood */
  int classdice[3][2]; /* Dice info for starting age based on class age type */
  int middle;          /* Middle age */
  int old;             /* Old age */
  int venerable;       /* Venerable age */
  int maxdice[2];      /* For roll to determine natural death beyond venerable */
};

extern const struct aging_data racial_aging_data[NUM_RACES];

extern const char *d_race_types[NUM_RACES + 1];
extern const char *race_names[NUM_RACES + 1];
extern const char *pc_race_types[NUM_RACES + 1];
extern const int race_def_sizetable[NUM_RACES + 1];
extern const char *race_abbrevs[NUM_RACES + 1];