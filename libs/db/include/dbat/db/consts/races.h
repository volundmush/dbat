#pragma once

/* Races */
#define RACE_UNDEFINED		-1
#define RACE_HUMAN		0
#define RACE_SAIYAN		1
#define RACE_ICER		2
#define RACE_KONATSU		3
#define RACE_NAMEK		4
#define RACE_MUTANT		5
#define RACE_KANASSAN		6
#define RACE_HALFBREED		7
#define RACE_BIO		8
#define RACE_ANDROID		9
#define RACE_DEMON        10
#define RACE_MAJIN        11
#define RACE_KAI        12
#define RACE_TRUFFLE        13
#define RACE_GOBLIN        14
#define RACE_ANIMAL        15
#define RACE_SAIBA        16
#define RACE_SERPENT        17
#define RACE_OGRE        18
#define RACE_YARDRATIAN        19
#define RACE_ARLIAN        20
#define RACE_DRAGON        21
#define RACE_MECHANICAL        22
#define RACE_FAERIE        23

#define NUM_RACES		24

struct aging_data {
  int adult;		/* Adulthood */
  int classdice[3][2];	/* Dice info for starting age based on class age type */
  int middle;		/* Middle age */
  int old;		/* Old age */
  int venerable;	/* Venerable age */
  int maxdice[2];	/* For roll to determine natural death beyond venerable */
};

extern const struct aging_data racial_aging_data[NUM_RACES];

extern const char *d_race_types[NUM_RACES+1];
extern const char *race_names[NUM_RACES+1];
extern const char *pc_race_types[NUM_RACES+1];
extern const int race_def_sizetable[NUM_RACES + 1];
extern const char *race_abbrevs[NUM_RACES+1];