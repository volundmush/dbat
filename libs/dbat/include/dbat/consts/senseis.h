#pragma once

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
constexpr int CLASS_UNDEFINED	        = -1;
constexpr int CLASS_ROSHI             = 0;
constexpr int CLASS_PICCOLO           = 1;
constexpr int CLASS_KRANE             = 2;
constexpr int CLASS_NAIL              = 3;
constexpr int CLASS_BARDOCK           = 4;
constexpr int CLASS_GINYU             = 5;
constexpr int CLASS_FRIEZA            = 6;
constexpr int CLASS_TAPION            = 7;
constexpr int CLASS_ANDSIX            = 8;
constexpr int CLASS_DABURA            = 9;
constexpr int CLASS_KABITO            = 10;
constexpr int CLASS_JINTO             = 11;
constexpr int CLASS_TSUNA             = 12;
constexpr int CLASS_KURZAK            = 13;
constexpr int CLASS_ASSASSIN          = 14;
constexpr int CLASS_BLACKGUARD        = 15;
constexpr int CLASS_DRAGON_DISCIPLE   = 16;
constexpr int CLASS_DUELIST           = 17;
constexpr int CLASS_DWARVEN_DEFENDER  = 18;
constexpr int CLASS_ELDRITCH_KNIGHT   = 19;
constexpr int CLASS_HIEROPHANT        = 20;
constexpr int CLASS_HORIZON_WALKER    = 21;
constexpr int CLASS_LOREMASTER        = 22;
constexpr int CLASS_MYSTIC_THEURGE    = 23;
constexpr int CLASS_SHADOWDANCER      = 24;
constexpr int CLASS_THAUMATURGIST     = 25;
constexpr int CLASS_NPC_EXPERT	= 26;
constexpr int CLASS_NPC_ADEPT		= 27;
constexpr int CLASS_NPC_COMMONER	= 28;
constexpr int CLASS_NPC_ARISTOCRAT	= 29;
constexpr int CLASS_NPC_WARRIOR	= 30;

constexpr int MAX_SENSEI              = 15; /* Used by Sensei Style */
constexpr int NUM_CLASSES             = 31;
constexpr int NUM_NPC_CLASSES 	= 4;
constexpr int NUM_PRESTIGE_CLASSES	= 15;
constexpr int NUM_BASIC_CLASSES	= (14);


/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
constexpr int SENSEI_UNDEFINED	        = -1;
constexpr int SENSEI_ROSHI             = 0;
constexpr int SENSEI_PICCOLO           = 1;
constexpr int SENSEI_KRANE             = 2;
constexpr int SENSEI_NAIL              = 3;
constexpr int SENSEI_BARDOCK           = 4;
constexpr int SENSEI_GINYU             = 5;
constexpr int SENSEI_FRIEZA            = 6;
constexpr int SENSEI_TAPION            = 7;
constexpr int SENSEI_ANDSIX            = 8;
constexpr int SENSEI_DABURA            = 9;
constexpr int SENSEI_KABITO            = 10;
constexpr int SENSEI_JINTO             = 11;
constexpr int SENSEI_TSUNA             = 12;
constexpr int SENSEI_KURZAK            = 13;
constexpr int SENSEI_ASSASSIN          = 14;
constexpr int SENSEI_BLACKGUARD        = 15;
constexpr int SENSEI_DRAGON_DISCIPLE   = 16;
constexpr int SENSEI_DUELIST           = 17;
constexpr int SENSEI_DWARVEN_DEFENDER  = 18;
constexpr int SENSEI_ELDRITCH_KNIGHT   = 19;
constexpr int SENSEI_HIEROPHANT        = 20;
constexpr int SENSEI_HORIZON_WALKER    = 21;
constexpr int SENSEI_LOREMASTER        = 22;
constexpr int SENSEI_MYSTIC_THEURGE    = 23;
constexpr int SENSEI_SHADOWDANCER      = 24;
constexpr int SENSEI_THAUMATURGIST     = 25;
constexpr int SENSEI_NPC_EXPERT	= 26;
constexpr int SENSEI_NPC_ADEPT		= 27;
constexpr int SENSEI_NPC_COMMONER	= 28;
constexpr int SENSEI_NPC_ARISTOCRAT	= 29;
constexpr int SENSEI_NPC_WARRIOR	= 30;

extern const char *sensei_style[MAX_SENSEI];

extern const char *pc_class_types[NUM_CLASSES+1];
extern const char *class_names[NUM_CLASSES+1];
extern const char *class_abbrevs[NUM_CLASSES+1];