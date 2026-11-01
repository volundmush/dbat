/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */
#include "dbat/consts/maximums.h"
#include "dbat/utils.h"
#include "dbat/class.h"

#include "dbat/db.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/feats.h"
#include "dbat/oasis.h"
#include "dbat/act.wizard.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/dg_comm.h"
#include "dbat/config.h"
#include "dbat/act.other.h"
#include "dbat/random.h"
#include "dbat/relocate.h"
#include "dbat/stringutils.h"
#include "dbat/local_limits.h"

#include <errno.h>

static const char *sensei_abbr(int s);

/* Names first */

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only WIZARDS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */

/* 
 * These tables hold the various level configuration setting;
 * experience points, base hit values, character saving throws.
 * They are read from a configuration file (normally etc/levels)
 * as part of the boot process.  The function load_levels() at
 * the end of this file reads in the actual values.
 */

static const char *config_sect[NUM_CONFIG_SECTIONS + 1] = {
    "version",
    "experience",
    "vernum",
    "fortitude",
    "reflex",
    "will",
    "basehit",
    "\n"};

#define CONFIG_LEVEL_VERSION    0
#define CONFIG_LEVEL_EXPERIENCE 1
#define CONFIG_LEVEL_VERNUM     2
#define CONFIG_LEVEL_FORTITUDE  3
#define CONFIG_LEVEL_REFLEX     4
#define CONFIG_LEVEL_WILL       5
#define CONFIG_LEVEL_BASEHIT    6

static char level_version[READ_SIZE];
static int level_vernum = 0;
static int save_classes[SAVING_WILL + 1][NUM_CLASSES];
static int basehit_classes[NUM_CLASSES];

#define SAVE_MANUAL 0
#define SAVE_LOW    1
#define SAVE_HIGH   2

#define BASEHIT_MANUAL 0
#define BASEHIT_LOW    1
#define BASEHIT_MEDIUM 2
#define BASEHIT_HIGH   3

void cedit_creation(struct char_data *ch)
{
  switch (CONFIG_CREATION_METHOD) {
    case CEDIT_CREATION_METHOD_3: /* Points Pool */
      break;
    case CEDIT_CREATION_METHOD_4: /* Racial based template */
      break;
    case CEDIT_CREATION_METHOD_5: /* Class based template */
      break;
    case CEDIT_CREATION_METHOD_2: /* Random rolls, player can adjust */
    case CEDIT_CREATION_METHOD_1: /* Standard random roll, system assigned */
    default:
      break;
  }
  racial_body_parts(ch);
  ch->aff_abils = ch->real_abils;
}

/*
 * Roll the 6 stats for a character... each stat is made of some combination
 * of 6-sided dice with various rolls discarded.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
/* This character creation method is original */

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  int punch;
  struct obj_data *obj;

  char_set_class_level(ch, 1);
  char_set_race_level(ch, 0);
  char_set_level_adj(ch, 0);
  char_set_exp(ch, 1);

  if (char_is_android(ch)) {
    char_set_conditions(ch, HUNGER, -1);
    char_set_conditions(ch, THIRST, -1);
    char_set_conditions(ch, DRUNK, -1);
  } else if (char_is_bio(ch) && char_has_genome(ch, 3)) {
    char_set_conditions(ch, HUNGER, -1);
    char_set_conditions(ch, DRUNK, 0);
    char_set_conditions(ch, THIRST, 48);
  } else if (char_is_namek(ch)) {
    char_set_conditions(ch, HUNGER, -1);
    char_set_conditions(ch, DRUNK, 0);
    char_set_conditions(ch, THIRST, 48);
  } else {
    char_set_conditions(ch, THIRST, 48);
    char_set_conditions(ch, HUNGER, 48);
    char_set_conditions(ch, DRUNK, 0);
  }

  char_prf_set(ch, PRF_AUTOEXIT, true);
  char_prf_set(ch, PRF_HINTS, true);
  char_prf_set(ch, PRF_NOMUSIC, true);
  char_prf_set(ch, PRF_DISPHP, true);
  char_set_limb_condition(ch, (1) - 1, 100);
  char_set_limb_condition(ch, (2) - 1, 100);
  char_set_limb_condition(ch, (3) - 1, 100);
  char_set_limb_condition(ch, (4) - 1, 100);
  char_plr_set(ch, PLR_HEAD, true);

  char_set_skill_slots(ch, 30);

  if (char_get_race(ch) == RACE_HUMAN) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (1));
  } else if (char_get_race(ch) == RACE_SAIYAN) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) - (1));
  } else if (char_get_race(ch) == RACE_TRUFFLE) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (2));
  } else if (char_get_race(ch) == RACE_HALFBREED) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (1));
  } else if (char_get_race(ch) == RACE_MAJIN) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) - (1));
  } else if (char_get_race(ch) == RACE_KAI) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (4));
  }

  if (char_is_tsuna(ch) || char_is_kabito(ch) || char_is_nail(ch))
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (5));

  if (char_get_bonuses(ch, BONUS_GMEMORY))
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (2));
  if (char_get_bonuses(ch, BONUS_BMEMORY))
    char_set_skill_slots(ch, char_get_skill_slots(ch) - (5));

  if (char_get_race(ch) == RACE_SAIYAN || char_get_race(ch) == RACE_HALFBREED) {
    if (char_get_race(ch) != RACE_HALFBREED || (char_get_race(ch) == RACE_HALFBREED && char_get_racial_pref(ch) != 1)) {
      char_plr_set(ch, PLR_STAIL, true);
    }
  }
  if (char_get_race(ch) == RACE_ICER || char_get_race(ch) == RACE_BIO) {
    char_plr_set(ch, PLR_TAIL, true);
  }
  if (char_get_race(ch) == RACE_MAJIN) {
    char_set_absorbs(ch, 0);
    char_set_ingestLearned(ch, 0);
  }
  if (char_get_race(ch) == RACE_BIO) {
    char_set_absorbs(ch, 3);
  }
  char_prf_set(ch, PRF_VIEWORDER, true);
  char_prf_set(ch, PRF_DISPMOVE, true);
  char_prf_set(ch, PRF_DISPKI, true);
  char_prf_set(ch, PRF_DISPEXP, true);
  char_prf_set(ch, PRF_DISPTNL, true);

  if (!char_plr_flagged(ch, PLR_FORGET)) {
    if (ch->choice == 1) {
      punch = rand_number(30, 40);
      char_set_skills_base(ch, SKILL_BLOCK, punch);
    }
    if (ch->choice == 2) {
      punch = rand_number(10, 20);
      char_set_skills_base(ch, SKILL_PUNCH, punch);
    }
    if (ch->choice == 3) {
      punch = rand_number(30, 40);
      char_set_skills_base(ch, SKILL_KICK, punch);
    }
    if (ch->choice == 4) {
      punch = rand_number(20, 30);
      char_set_skills_base(ch, SKILL_SLAM, punch);
    }
    if (ch->choice == 5) {
      punch = rand_number(20, 30);
      char_set_skills_base(ch, SKILL_FOCUS, punch);
    }
    if (char_is_human(ch)) {
      punch = rand_number(5, 15);
      char_set_skills_base(ch, SKILL_BUILD, punch);
    }
    if (char_is_truffle(ch)) {
      punch = rand_number(15, 25);
      char_set_skills_base(ch, SKILL_BUILD, punch);
    }
    if (char_is_konatsu(ch)) {
      punch = rand_number(50, 60);
      char_set_skills_base(ch, SKILL_SWORD, punch);
      punch = rand_number(10, 30);
      char_set_skills_base(ch, SKILL_MOVE_SILENTLY, punch);
      punch = rand_number(10, 30);
      char_set_skills_base(ch, SKILL_HIDE, punch);
    }
    if (char_is_tapion(ch) || char_is_ginyu(ch) || char_is_dabura(ch) || char_is_kurzak(ch)) {
      punch = rand_number(30, 40);
      if (char_is_kurzak(ch) || char_is_tapion(ch)) {
        punch += rand_number(5, 10);
      }
      char_set_skills_base(ch, SKILL_THROW, punch);
    }
    if (char_is_kai(ch) || char_is_kanassan(ch)) {
      punch = rand_number(40, 60);
      char_set_skills_base(ch, SKILL_FOCUS, punch);
    }
    if (char_is_kanassan(ch)) {
      punch = rand_number(40, 60);
      char_set_skills_base(ch, SKILL_CONCENTRATION, punch);
    }
    if (char_is_kai(ch)) {
      punch = rand_number(30, 50);
      char_set_skills_base(ch, SKILL_HEAL, punch);
    }
    if (char_is_demon(ch)) {
      punch = rand_number(50, 60);
      char_set_skills_base(ch, SKILL_SPEAR, punch);
      char_set_skills_base(ch, SKILL_HONOO, punch);
    }

    punch = 0;
    punch = rand_number(50, 70);
    char_set_skills_base(ch, SKILL_PUNCH, char_get_skills_base(ch, SKILL_PUNCH) + punch);
  } /* End CC skills */
  else {
    char_plr_set(ch, PLR_FORGET, false);
  }

  if (char_is_kai(ch) || char_is_kanassan(ch)) {
    punch = rand_number(15, 30);
    char_set_skills_base(ch, SKILL_TELEPATHY, punch);
  }

  if (char_is_majin(ch) || char_is_namek(ch) || char_is_bio(ch)) {
    punch = rand_number(10, 16);
    char_set_skills_base(ch, SKILL_REGENERATE, punch);
  }
  if (char_is_android(ch) && char_plr_flagged(ch, PLR_ABSORB)) {
    punch = rand_number(25, 35);
    char_set_skills_base(ch, SKILL_ABSORB, punch);
  }
  if (char_is_bio(ch)) {
    punch = rand_number(15, 25);
    char_set_skills_base(ch, SKILL_ABSORB, punch);

    // Kai Genome bioandroids.
    if (char_has_genome(ch, 7)) {
      char_set_skills_base(ch, SKILL_TELEPATHY, 30);
      char_set_skills_base(ch, SKILL_FOCUS, 30);
    }
  }
  if (char_is_arlian(ch)) {
    punch = rand_number(30, 50);
    char_set_skills_base(ch, SKILL_SEISHOU, punch);
  }
  if (char_is_icer(ch)) {
    punch = rand_number(20, 30);
    char_set_skills_base(ch, SKILL_TAILWHIP, punch);
  }

  if (char_get_class(ch) < 0 || char_get_class(ch) > NUM_CLASSES) {
    mud_log("Unknown character class %d in do_start, resetting.", char_get_class(ch));
    //GET_CLASS(ch) = 0;
  }
  if (char_get_alignment(ch) < 51 && char_get_alignment(ch) > -51) {
    set_title(ch, "the Warrior");
  }
  if (char_get_alignment(ch) >= 51) {
    set_title(ch, "the Hero");
  }
  if (char_get_alignment(ch) <= -51) {
    set_title(ch, "The Villain");
  }
  /* roll_real_abils(ch); */
  if (char_get_gold(ch) <= 0) {
    char_set_gold(ch, dice(3, 6) * 10);
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (char_get_race(ch)) {
    case RACE_HUMAN:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    case RACE_SAIYAN:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    case RACE_HALFBREED:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    case RACE_ICER:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    case RACE_KONATSU:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    case RACE_NAMEK:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    case RACE_MUTANT:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    case RACE_ANDROID:
      char_aff_set(ch, AFF_INFRAVISION, true);
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
    default:
      char_set_skills_base(ch, SKILL_LANG_COMMON, 1);
      break;
  }

  char_set_speaking(ch, SKILL_LANG_COMMON);

  char_set_lifeperc(ch, 75);

  /* assign starting items etc...*/
  obj = read_object(17, VIRTUAL);
  obj_to_char(obj, ch);
  if (char_is_hoshijin(ch)) {
    obj = read_object(3428, VIRTUAL);
    obj_to_char(obj, ch);
  }
  struct obj_data *obj2;
  obj2 = read_object(17998, VIRTUAL);
  obj_to_char(obj2, ch);

  if (char_is_tapion(ch) || char_is_ginyu(ch)) {
    struct obj_data *throw_obj;
    throw_obj = read_object(19050, VIRTUAL);
    obj_to_char(throw_obj, ch);
    if (rand_number(1, 2) == 2) {
      throw_obj = NULL;
      throw_obj = read_object(19050, VIRTUAL);
      obj_to_char(throw_obj, ch);
    }
    if (rand_number(1, 2) == 2) {
      throw_obj = NULL;
      throw_obj = read_object(19050, VIRTUAL);
      obj_to_char(throw_obj, ch);
    }

  } else if (char_is_dabura(ch)) {
    struct obj_data *throw_obj;
    throw_obj = read_object(19055, VIRTUAL);
    obj_to_char(throw_obj, ch);
    throw_obj = NULL;
    throw_obj = read_object(19055, VIRTUAL);
    obj_to_char(throw_obj, ch);
  }

  send_to_imm("New character created, %s, by user, %s.", char_get_name(ch), char_get_user(ch));
  advance_level(ch, char_get_class(ch));
  /*mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));*/

  ch->basepl = MAX(ch->basepl, 100L);
  ch->baseki = MAX(ch->baseki, 100L);
  ch->basest = MAX(ch->basest, 100L);

  if (char_is_android(ch) && char_plr_flagged(ch, PLR_SENSEM)) {
    char_set_skills_base(ch, SKILL_SENSE, 100);
    gainBasePL(ch, rand_number(400, 500), false);
    gainBaseST(ch, rand_number(400, 500), false);
    gainBaseKI(ch, rand_number(400, 500), false);
  }

  if (ch->real_abils.str > 20) {
    ch->real_abils.str = 20;
  }
  if (ch->real_abils.str < 8) {
    ch->real_abils.str = 8;
  }
  if (ch->real_abils.con > 20) {
    ch->real_abils.con = 20;
  }
  if (ch->real_abils.con < 8) {
    ch->real_abils.con = 8;
  }
  if (ch->real_abils.intel > 20) {
    ch->real_abils.intel = 20;
  }
  if (ch->real_abils.intel < 8) {
    ch->real_abils.intel = 8;
  }
  if (ch->real_abils.cha > 20) {
    ch->real_abils.cha = 20;
  }
  if (ch->real_abils.cha < 8) {
    ch->real_abils.cha = 8;
  }
  if (ch->real_abils.dex > 20) {
    ch->real_abils.dex = 20;
  }
  if (ch->real_abils.dex < 8) {
    ch->real_abils.dex = 8;
  }
  if (ch->real_abils.wis > 20) {
    ch->real_abils.wis = 20;
  }
  if (ch->real_abils.wis < 8) {
    ch->real_abils.wis = 8;
  }

  char_set_transclass(ch, rand_number(1, 3));

  if (CONFIG_SITEOK_ALL)
    char_plr_set(ch, PLR_SITEOK, true);

  if (char_get_race(ch) == RACE_SAIYAN && rand_number(1, 100) >= 95) {
    char_plr_set(ch, PLR_LSSJ, true);
    write_to_output(ch->desc, "@GYou were one of the few born a Legendary Super Saiyan!@n\r\n");
  }
  restoreVitals(ch, true);

  ch->player_specials->olc_zone = NOWHERE;
  save_char(ch);
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */

/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
/* Rillao: transloc, add new transes here */
void advance_level(struct char_data *ch, int whichclass)
{
  int64_t add_hp = 0, add_move = 0, add_mana = 0;
  int add_prac      = 1, add_train, i, ranks;
  int add_gen_feats = 0;
  char buf[MAX_STRING_LENGTH];

  if (whichclass < 0 || whichclass >= NUM_CLASSES) {
    mud_log("Invalid class %d passed to advance_level, resetting.", whichclass);
    whichclass = 0;
  }

  if (!CONFIG_ALLOW_MULTICLASS && whichclass != char_get_class(ch)) {
    mud_log("Attempt to gain a second class without multiclass enabled for %s", char_get_name(ch));
    whichclass = char_get_class(ch);
  }
  ranks = char_get_class_ranks(ch, whichclass);

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (ranks) {
    case 1:
      break;
    case 2:
      switch (whichclass) {
        case CLASS_ROSHI:
          break;
        case CLASS_PICCOLO:
          break;
      }
      break;
    case 6:
      switch (whichclass) {
        case CLASS_ROSHI:
          break;
      }
      break;
    case 9:
      switch (whichclass) {
        case CLASS_ROSHI:
          break;
      }
      break;
  }

  if (char_get_class_level(ch) == 1 && char_get_race_level(ch) < 2) { /* Filled in below */
    char_set_race_level(ch, 0);
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (char_get_level(ch) >= 1) {
    double pl_percent = 10, ki_percent = 10, st_percent = 10, prac_reward = char_get_aff_abils_wis(ch);

    if (prac_reward < 10) {
      prac_reward = 10;
    }

    if (char_get_level(ch) >= 91) {
      pl_percent -= 7.2;
      ki_percent -= 7.2;
      st_percent -= 7.2;
    } else if (char_get_level(ch) >= 81) {
      pl_percent -= 5.8;
      ki_percent -= 5.8;
      st_percent -= 5.8;
    } else if (char_get_level(ch) >= 71) {
      pl_percent -= 5;
      ki_percent -= 5;
      st_percent -= 5;
    } else if (char_get_level(ch) >= 61) {
      pl_percent -= 4;
      ki_percent -= 4;
      st_percent -= 4;
    } else if (char_get_level(ch) >= 51) {
      pl_percent -= 3;
      ki_percent -= 3;
      st_percent -= 3;
    } else if (char_get_level(ch) >= 41) {
      pl_percent -= 1.7;
      ki_percent -= 1.7;
      st_percent -= 1.7;
    } else if (char_get_level(ch) >= 31) {
      pl_percent -= 0.8;
      ki_percent -= 0.8;
      st_percent -= 0.8;
    } else if (char_get_level(ch) >= 21) {
      pl_percent -= 0.35;
      ki_percent -= 0.35;
      st_percent -= 0.35;
    } else if (char_get_level(ch) >= 11) {
      pl_percent -= 0.15;
      ki_percent -= 0.15;
      st_percent -= 0.15;
    }

    switch (char_get_race(ch)) {
      case RACE_HUMAN:
        ki_percent += 3;
        break;
      case RACE_SAIYAN:
      case RACE_MAJIN:
        pl_percent += 3;
        break;
      case RACE_MUTANT:
        st_percent += 3;
        break;
      case RACE_HALFBREED:
        pl_percent += 1.5;
        ki_percent += 1.5;
        prac_reward -= prac_reward * 0.4;
        break;
      case RACE_TRUFFLE:
        prac_reward += prac_reward * 0.5;
        break;
    }

    if (!char_is_human(ch)) {
      add_hp = ((getBasePL(ch)) * 0.01) * pl_percent;
    } else if (char_is_human(ch)) {
      add_hp = (((getBasePL(ch)) * 0.01) * pl_percent) * 0.8;
    }
    add_mana = ((getBaseKI(ch)) * 0.01) * ki_percent;
    add_move = ((getBaseST(ch)) * 0.01) * st_percent;
    add_prac = prac_reward + char_get_aff_abils_intel(ch);
  }
  if (add_hp >= 300000 && add_hp < 600000) {
    add_hp *= .75;
    if (add_hp < 300000) {
      add_hp = rand_number(300000, 330000);
    }
  } else if (add_hp >= 600000 && add_hp < 1000000) {
    add_hp *= .70;
    if (add_hp < 600000) {
      add_hp = rand_number(600000, 650000);
    }
  } else if (add_hp >= 1000000 && add_hp < 2000000) {
    add_hp *= .65;
    if (add_hp < 1000000) {
      add_hp = rand_number(1000000, 1250000);
    }
  } else if (add_hp >= 2000000) {
    add_hp *= .45;
    if (add_hp < 2000000) {
      add_hp = rand_number(2000000, 2250000);
    }
  }
  if (add_hp >= 15000000) {
    add_hp = 15000000;
  }
  if (add_move >= 300000 && add_move < 600000) {
    add_move *= .75;
    if (add_move < 300000) {
      add_move = rand_number(300000, 330000);
    }
  } else if (add_move >= 600000 && add_move < 1000000) {
    add_move *= .70;
    if (add_move < 600000) {
      add_move = rand_number(600000, 650000);
    }
  } else if (add_move >= 1000000 && add_move < 2000000) {
    add_move *= .65;
    if (add_move < 1000000) {
      add_move = rand_number(1000000, 1250000);
    }
  } else if (add_move >= 2000000) {
    add_move *= .45;
    if (add_move < 2000000) {
      add_move = rand_number(2000000, 2250000);
    }
  }
  if (add_move >= 15000000) {
    add_move = 15000000;
  }
  if (add_mana >= 300000 && add_mana < 600000) {
    add_mana *= .75;
    if (add_mana < 300000) {
      add_mana = rand_number(300000, 330000);
    }
  } else if (add_mana >= 600000 && add_mana < 1000000) {
    add_mana *= .70;
    if (add_mana < 600000) {
      add_mana = rand_number(600000, 650000);
    }
  } else if (add_mana >= 1000000 && add_mana < 2000000) {
    add_mana *= .65;
    if (add_mana < 1000000) {
      add_mana = rand_number(1000000, 1250000);
    }
  } else if (add_mana >= 2000000) {
    add_mana *= .45;
    if (add_mana < 2000000) {
      add_mana = rand_number(2000000, 2250000);
    }
  }
  if (add_mana >= 15000000) {
    add_mana = 15000000;
  }
  switch (char_get_level(ch)) {
    case 5:
      add_hp += rand_number(600, 1000);
      add_move += rand_number(600, 1000);
      add_mana += rand_number(600, 1000);
      break;
    case 10:
      add_hp += rand_number(5000, 8000);
      add_move += rand_number(5000, 8000);
      add_mana += rand_number(5000, 8000);
      break;
    case 20:
      add_hp += rand_number(15000, 18000);
      add_move += rand_number(15000, 18000);
      add_mana += rand_number(15000, 18000);
      break;
    case 30:
      add_hp += rand_number(20000, 30000);
      add_move += rand_number(20000, 30000);
      add_mana += rand_number(20000, 30000);
      break;
    case 40:
      add_hp += rand_number(50000, 60000);
      add_move += rand_number(50000, 60000);
      add_mana += rand_number(50000, 60000);
      break;
    case 50:
      add_hp += rand_number(60000, 70000);
      add_move += rand_number(60000, 70000);
      add_mana += rand_number(60000, 70000);
      break;
    case 60:
      add_hp += rand_number(80000, 100000);
      add_move += rand_number(80000, 100000);
      add_mana += rand_number(80000, 100000);
      break;
    case 70:
      add_hp += rand_number(100000, 150000);
      add_move += rand_number(100000, 150000);
      add_mana += rand_number(100000, 150000);
      break;
    case 80:
      add_hp += rand_number(150000, 200000);
      add_move += rand_number(150000, 200000);
      add_mana += rand_number(150000, 200000);
      break;
    case 90:
      add_hp += rand_number(150000, 200000);
      add_move += rand_number(150000, 200000);
      add_mana += rand_number(150000, 200000);
      break;
    case 99:
      add_hp += rand_number(1500000, 2000000);
      add_move += rand_number(1500000, 2000000);
      add_mana += rand_number(1500000, 2000000);
      break;
    case 100:
      add_hp += rand_number(5000000, 6000000);
      add_move += rand_number(5000000, 6000000);
      add_mana += rand_number(5000000, 6000000);
      break;
  }
  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (char_get_level(ch) == 1 || !(char_get_level(ch) % 3)) {
    add_gen_feats += 1;
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (char_get_race(ch) == RACE_HUMAN) /* Humans get 4 extra at level 1 and  */
    add_prac += 2;                     /* 1 extra per level for adaptability */

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  i = ability_mod_value(char_get_aff_abils_con(ch));
  if (char_get_level(ch) > 1) {
    /* blah */
  } else {
    gainBasePL(ch, rand_number(1, 20), false);
    ch->basepl = MAX(ch->basepl, 250L);
    ch->baseki = MAX(ch->baseki, 250L);
    ch->basest = MAX(ch->basest, 250L);

    add_prac = 5;
    if (char_plr_flagged(ch, PLR_SKILLP)) {
      char_plr_set(ch, PLR_SKILLP, false);
      add_prac *= 5;
    } else {
      add_prac *= 2;
    }
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (rand_number(1, 8) == 2) {
    add_train = 1;
    if (add_train) {
      char_set_ability_trains(ch, char_get_ability_trains(ch) + (add_train));
    }
  }
  if (rand_number(1, 4) == 4) {
    send_to_char(ch, "@D[@mPractice Session Bonus!@D]@n\r\n");
    add_prac += rand_number(4, 12);
  }

  if ((char_is_demon(ch) || char_is_kanassan(ch)) && char_get_level(ch) > 80) {
    add_hp *= 2;
    add_mana *= 2;
    add_move *= 2;
  } else if ((char_is_demon(ch) || char_is_kanassan(ch)) && char_get_level(ch) > 60) {
    add_hp *= 1.75;
    add_mana *= 1.75;
    add_move *= 1.75;
  } else if ((char_is_demon(ch) || char_is_kanassan(ch)) && char_get_level(ch) > 50) {
    add_hp *= 1.5;
    add_mana *= 1.5;
    add_move *= 1.5;
  }
  /* Rillao: transloc, add new transes here */
  else if ((char_is_demon(ch) || char_is_kanassan(ch)) && char_get_level(ch) > 40) {
    add_hp *= 1.25;
    add_mana *= 1.25;
    add_move *= 1.25;
  }
  char_set_practice_points(ch, char_get_practice_points(ch) + (add_prac));
  gainBasePL(ch, add_hp, true);
  gainBaseKI(ch, add_mana, true);
  gainBaseST(ch, add_move, true);
  int nhp  = add_hp;
  int nma  = add_mana;
  int nmo  = add_move;
  add_hp   = nhp;
  add_mana = nma;
  add_move = nmo;
  /* epic and class feat points no longer exist; epic characters gained only those */
  if (char_get_class_level(ch) < LVL_EPICSTART)
    char_set_feat_points(ch, char_get_feat_points(ch) + (add_gen_feats));

  if (char_get_admlevel(ch) >= ADMLVL_IMMORT) {
    for (i = 0; i < 3; i++)
      char_set_conditions(ch, i, (char)-1);
    char_prf_set(ch, PRF_HOLYLIGHT, true);
  }

  sprintf(buf, "@D[@YGain@D: @RPl@D(@G%s@D) @gSt@D(@G%s@D) @CKi@D(@G%s@D) @bPS@D(@G%s@D)]", add_commas(add_hp), add_commas(add_move), add_commas(add_mana), add_commas(add_prac));
  if (char_get_bonuses(ch, BONUS_GMEMORY) && (char_get_level(ch) == 20 || char_get_level(ch) == 40 || char_get_level(ch) == 60 || char_get_level(ch) == 80 || char_get_level(ch) == 100)) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (1));
    send_to_char(ch, "@CYou feel like you could remember a new skill!@n\r\n");
  }
  if (char_is_namek(ch) && rand_number(1, 100) <= 5) {
    char_set_skill_slots(ch, char_get_skill_slots(ch) + (1));
    send_to_char(ch, "@CYou feel as though you could learn another skill.@n\r\n");
  }
  if (char_is_icer(ch) && rand_number(1, 100) <= 25) {
    bring_to_cap(ch);
    send_to_char(ch, "@GYou feel your body obtain its current optimal strength!@n\r\n");
  }

  int gain_stat = FALSE;

  switch (char_get_level(ch)) {
    case 10:
    case 20:
    case 30:
    case 40:
    case 50:
    case 60:
    case 70:
    case 80:
    case 90:
    case 100:
      gain_stat = TRUE;
      break;
  }

  if (gain_stat == TRUE) {
    int raise = FALSE, stat_fail = 0;
    if (char_is_konatsu(ch)) {
      while (raise == FALSE) {
        if (ch->real_abils.dex < 100 && rand_number(1, 2) == 2 && stat_fail != 1) {
          if (ch->real_abils.dex < 45 || char_get_bonuses(ch, BONUS_CLUMSY) <= 0) {
            ch->real_abils.dex += 1;
            send_to_char(ch, "@GYou feel your agility increase!@n\r\n");
            raise = TRUE;
          } else {
            stat_fail += 1;
          }
        } else if (ch->real_abils.cha < 100 && raise == FALSE && stat_fail < 2) {
          if (ch->real_abils.cha < 45 || char_get_bonuses(ch, BONUS_SLOW) > 0) {
            ch->real_abils.cha += 1;
            send_to_char(ch, "@GYou feel your speed increase!@n\r\n");
            raise = TRUE;
          } else {
            stat_fail += 2;
          }
        } else if (stat_fail == 3) {
          send_to_char(ch, "@RBoth agility and speed are capped!@n");
          raise = TRUE;
        }
      } // End while
    } // End Konatsu

    else if (char_is_mutant(ch)) {
      while (raise == FALSE) {
        if (ch->real_abils.con < 100 && rand_number(1, 2) == 2 && stat_fail != 1) {
          if (ch->real_abils.con < 45 || char_get_bonuses(ch, BONUS_FRAIL) <= 0) {
            ch->real_abils.con += 1;
            send_to_char(ch, "@GYou feel your constitution increase!@n\r\n");
            raise = TRUE;
          } else {
            stat_fail += 1;
          }
        } else if (ch->real_abils.cha < 100 && raise == FALSE && stat_fail < 2) {
          if (ch->real_abils.cha < 45 || char_get_bonuses(ch, BONUS_SLOW) > 0) {
            ch->real_abils.cha += 1;
            send_to_char(ch, "@GYou feel your speed increase!@n\r\n");
            raise = TRUE;
          } else {
            stat_fail += 2;
          }
        } else if (stat_fail == 3) {
          send_to_char(ch, "@RBoth constitution and speed are capped!@n");
          raise = TRUE;
        }
      } // End while
    } // End Mutant

    else if (char_is_hoshijin(ch)) {
      while (raise == FALSE) {
        if (ch->real_abils.str < 100 && rand_number(1, 2) == 2 && stat_fail != 1) {
          if (ch->real_abils.str < 45 || char_get_bonuses(ch, BONUS_WIMP) <= 0) {
            ch->real_abils.str += 1;
            send_to_char(ch, "@GYou feel your strength increase!@n\r\n");
            raise = TRUE;
          } else {
            stat_fail += 1;
          }
        } else if (ch->real_abils.dex < 100 && raise == FALSE && stat_fail < 2) {
          if (ch->real_abils.dex < 45 || char_get_bonuses(ch, BONUS_SLOW) > 0) {
            ch->real_abils.dex += 1;
            send_to_char(ch, "@GYou feel your agility increase!@n\r\n");
            raise = TRUE;
          } else {
            stat_fail += 2;
          }
        } else if (stat_fail == 3) {
          send_to_char(ch, "@RBoth strength and agility are capped!@n");
          raise = TRUE;
        }
      } // End while
    } // End Mutant

  } // End stat bonus on level gain

  strcat(buf, ".\r\n");
  send_to_char(ch, "%s", buf);

  if (char_get_skill(ch, SKILL_POTENTIAL) && rand_number(1, 4) == 4) {
    send_to_char(ch, "You can now perform another Potential Release.\r\n");
    char_set_boosts(ch, char_get_boosts(ch) + (1));
  }
  if (char_is_majin(ch) && char_get_level(ch) == 25) {
    send_to_char(ch, "You can now perform another Majinization.\r\n");
    char_set_boosts(ch, char_get_boosts(ch) + (1));
  }
  if (char_is_majin(ch) && char_get_level(ch) == 50) {
    send_to_char(ch, "You can now perform another Majinization.\r\n");
    char_set_boosts(ch, char_get_boosts(ch) + (1));
  }
  if (char_is_majin(ch) && char_get_level(ch) == 75) {
    send_to_char(ch, "You can now perform another Majinization.\r\n");
    char_set_boosts(ch, char_get_boosts(ch) + (1));
  }
  if (char_is_majin(ch) && char_get_level(ch) == 100) {
    send_to_char(ch, "You can now perform another Majinization.\r\n");
    char_set_boosts(ch, char_get_boosts(ch) + (1));
  }

  switch (char_get_level(ch)) {
    case 10:
    case 20:
    case 30:
    case 40:
    case 50:
    case 60:
    case 70:
    case 80:
    case 90:
    case 100:
      if (char_get_bonuses(ch, BONUS_BRAWNY) > 0) {
        ch->real_abils.str += 2;
        send_to_char(ch, "@GYour muscles have grown stronger!@n\r\n");
      }
      if (char_get_bonuses(ch, BONUS_SCHOLARLY) > 0) {
        ch->real_abils.intel += 2;
        send_to_char(ch, "@GYour mind has grown sharper!@n\r\n");
      }
      if (char_get_bonuses(ch, BONUS_SAGE) > 0) {
        ch->real_abils.wis += 2;
        send_to_char(ch, "@GYour understanding about life has improved!@n\r\n");
      }
      if (char_get_bonuses(ch, BONUS_AGILE) > 0) {
        ch->real_abils.dex += 2;
        send_to_char(ch, "@GYour body has grown more agile!@n\r\n");
      }
      if (char_get_bonuses(ch, BONUS_QUICK) > 0) {
        ch->real_abils.cha += 2;
        send_to_char(ch, "@GYou feel like your speed has improved!@n\r\n");
      }
      if (char_get_bonuses(ch, BONUS_STURDY) > 0) {
        ch->real_abils.con += 2;
        send_to_char(ch, "@GYour body feels tougher now!@n\r\n");
      }
      break;
  }

  if (char_get_level(ch) == 1) {
    char_set_armor(ch, 0);
  }
  if (char_get_level(ch) == 2) {
    ERAPLAYERS += 1;
  }
  snoop_check(ch);
  save_char(ch);
}

/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (obj_extra_flagged(obj, ITEM_ANTI_WIZARD) && char_is_roshi(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_CLERIC) && char_is_piccolo(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_FIGHTER) && char_is_nail(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_ROGUE) && char_is_krane(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_MONK) && char_is_bardock(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_MONK) && !char_is_bardock(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_WIZARD) && !char_is_roshi(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_JINTO) && !char_is_jinto(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_CLERIC) && !char_is_piccolo(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_ROGUE) && !char_is_krane(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_ASSASSIN) && !char_is_kurzak(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_FIGHTER) && !char_is_nail(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_PALADIN) && !char_is_ginyu(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_WIZARD) && char_is_roshi(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_PALADIN) && char_is_ginyu(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_BARBARIAN) && char_is_kabito(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_BARD) && char_is_andsix(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ONLY_BARD) && !char_is_andsix(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_RANGER) && char_is_dabura(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_DRUID) && char_is_tapion(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_ARCANE_ARCHER) && char_is_jinto(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_ARCANE_TRICKSTER) && char_is_tsuna(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_ARCHMAGE) && char_is_kurzak(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_BLACKGUARD) && char_is_blackguard(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_DRAGON_DISCIPLE) && char_is_dragon_disciple(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_DUELIST) && char_is_duelist(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_DWARVEN_DEFENDER) && char_is_dwarven_defender(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_ELDRITCH_KNIGHT) && char_is_eldritch_knight(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT) && char_is_hierophant(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_HORIZON_WALKER) && char_is_horizon_walker(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_LOREMASTER) && char_is_loremaster(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_MYSTIC_THEURGE) && char_is_mystic_theurge(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_SHADOWDANCER) && char_is_shadowdancer(ch))
    return TRUE;

  if (obj_extra_flagged(obj, ITEM_ANTI_THAUMATURGIST) && char_is_thaumaturgist(ch))
    return TRUE;

  return FALSE;
}

/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */

/* Function to return the exp required for each class/level */
int level_exp(struct char_data *ch, int level)
{
  int req = 1;

  switch (level) {
    case 0:
      req = 0;
      break;
    case 1:
      req = 1;
      break;
    case 2:
      req = 999;
      break;
    case 3:
      req = 2000;
      break;
    case 4:
      req = 4500;
      break;
    case 5:
      req = 6000;
      break;
    case 6:
      req = 10000;
      break;
    case 7:
      req = 15000;
      break;
    case 8:
      req = 25000;
      break;
    case 9:
      req = 35000;
      break;
    case 10:
      req = 50000;
      break;
    case 11:
      req = 75000;
      break;
    case 12:
      req = 100000;
      break;
    case 13:
      req = 125000;
      break;
    case 14:
      req = 150000;
      break;
    case 15:
      req = 175000;
      break;
    case 16:
      req = 225000;
      break;
    case 17:
      req = 300000;
      break;
    case 18:
      req = 350000;
      break;
    case 19:
      req = 400000;
      break;
    case 20:
      req = 500000;
      break;
    case 21:
      req = 650000;
      break;
    case 22:
      req = 800000;
      break;
    case 23:
      req = 1000000;
      break;
    case 24:
      req = 1250000;
      break;
    case 25:
      req = 1500000;
      break;
    case 26:
      req = 1750000;
      break;
    case 27:
      req = 2000000;
      break;
    case 28:
      req = 2250000;
      break;
    case 29:
      req = 2500000;
      break;
    case 30:
      req = 2750000;
      break;
    case 31:
      req = 3000000;
      break;
    case 32:
      req = 3250000;
      break;
    case 33:
      req = 3500000;
      break;
    case 34:
      req = 3750000;
      break;
    case 35:
      req = 4000000;
      break;
    case 36:
      req = 4250000;
      break;
    case 37:
      req = 4500000;
      break;
    case 38:
      req = 4750000;
      break;
    case 39:
      req = 5000000;
      break;
    case 40:
      req = 5500000;
      break;
    case 41:
      req = 6000000;
      break;
    case 42:
      req = 6500000;
      break;
    case 43:
      req = 7000000;
      break;
    case 44:
      req = 7500000;
      break;
    case 45:
      req = 8000000;
      break;
    case 46:
      req = 8500000;
      break;
    case 47:
      req = 9000000;
      break;
    case 48:
      req = 9500000;
      break;
    case 49:
      req = 10000000;
      break;
    case 50:
      req = 10500000;
      break;
    case 51:
      req = 11000000;
      break;
    case 52:
      req = 11500000;
      break;
    case 53:
      req = 12000000;
      break;
    case 54:
      req = 12500000;
      break;
    case 55:
      req = 13000000;
      break;
    case 56:
      req = 13500000;
      break;
    case 57:
      req = 14000000;
      break;
    case 58:
      req = 14500000;
      break;
    case 59:
      req = 15000000;
      break;
    case 60:
      req = 18000000;
      break;
    case 61:
      req = 25000000;
      break;
    case 62:
      req = 28000000;
      break;
    case 63:
      req = 31000000;
      break;
    case 64:
      req = 34000000;
      break;
    case 65:
      req = 37000000;
      break;
    case 66:
      req = 40000000;
      break;
    case 67:
      req = 43000000;
      break;
    case 68:
      req = 46000000;
      break;
    case 69:
      req = 49000000;
      break;
    case 70:
      req = 52000000;
      break;
    case 71:
      req = 55000000;
      break;
    case 72:
      req = 58000000;
      break;
    case 73:
      req = 61000000;
      break;
    case 74:
      req = 64000000;
      break;
    case 75:
      req = 67000000;
      break;
    case 76:
      req = 70000000;
      break;
    case 77:
      req = 73000000;
      break;
    case 78:
      req = 76000000;
      break;
    case 79:
      req = 79000000;
      break;
    case 80:
      req = 82000000;
      break;
    case 81:
      req = 88000000;
      break;
    case 82:
      req = 94000000;
      break;
    case 83:
      req = 100000000;
      break;
    case 84:
      req = 106000000;
      break;
    case 85:
      req = 112000000;
      break;
    case 86:
      req = 118000000;
      break;
    case 87:
      req = 124000000;
      break;
    case 88:
      req = 130000000;
      break;
    case 89:
      req = 136000000;
      break;
    case 90:
      req = 142000000;
      break;
    case 91:
      req = 150000000;
      break;
    case 92:
      req = 175000000;
      break;
    case 93:
      req = 200000000;
      break;
    case 94:
      req = 225000000;
      break;
    case 95:
      req = 250000000;
      break;
    case 96:
      req = 300000000;
      break;
    case 97:
      req = 400000000;
      break;
    case 98:
      req = 500000000;
      break;
    case 99:
      req = 600000000;
      break;
    case 100:
      req = 800000000;
      break;
  }

  if (char_is_kai(ch)) {
    req += req * 0.15;
  }

  return (req);
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int8_t ability_mod_value(int abil)
{
  return ((int)(abil / 2)) - 5;
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int8_t dex_mod_capped(const struct char_data *ch)
{
  int8_t mod;
  struct obj_data *armor;
  mod   = ability_mod_value(char_get_aff_abils_dex(ch));
  armor = char_get_equipment(ch, WEAR_BODY);
  if (armor && obj_get_type_flag(armor) == ITEM_ARMOR) {
    mod = MIN(mod, obj_get_value(armor, VAL_ARMOR_MAXDEXMOD));
  }
  return mod;
}

static int cabbr_ranktable[NUM_CLASSES];

static int comp_rank(const void *a, const void *b)
{
  int first, second;
  first  = *(const int *)a;
  second = *(const int *)b;
  return cabbr_ranktable[second] - cabbr_ranktable[first];
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
char *class_desc_str(struct char_data *ch, int howlong, int wantthe)
{
  static char str[MAX_STRING_LENGTH];
  char *ptr = str;
  int i, rank, j;
  int rankorder[NUM_CLASSES];
  const char *buf, *buf2, *buf3;

  if (char_is_npc(ch)) {
    snprintf(ptr, sizeof(str) - (ptr - str), "%s%d", char_class_abbr(ch), char_get_level(ch));
    return str;
  }

  if (wantthe)
    ptr += sprintf(str, "the ");

  if (howlong) {
    buf2 = buf = buf3 = "";
    if (howlong == 2) {
      buf3 = " ";
      if (char_get_class_level(ch) >= LVL_EPICSTART)
        ptr += sprintf(ptr, "Epic ");
    }
    for (i = 0; i < NUM_CLASSES; i++) {
      cabbr_ranktable[i] = char_get_class_ranks(ch, i);
      rankorder[i]       = i;
    }
    rankorder[0]                  = char_get_class(ch); /* we always want primary class first */
    rankorder[char_get_class(ch)] = 0;
    qsort((void *)rankorder, NUM_CLASSES, sizeof(int), comp_rank);
    for (int sen = 0; sen < NUM_CLASSES; sen++) {
      if (!sensei_exists(sen))
        continue;
      ptr += snprintf(ptr, sizeof(str) - (ptr - str), "%s%s%s%s%s%d", buf, buf2, buf,
                      (howlong == 2 ? sensei_name(sen) : sensei_abbr(sen)), buf3,
                      cabbr_ranktable[sen]);
      buf2 = "/";
      if (howlong == 2)
        buf = " ";
    }
    return str;
  } else {
    rank = char_get_class_ranks(ch, char_get_class(ch));
    j    = char_get_class(ch);
    for (i = 0; i < NUM_CLASSES; i++)
      if (char_get_class_ranks(ch, i) > rank) {
        j    = i;
        rank = char_get_class_ranks(ch, j);
      }
    rank = char_get_class_ranks(ch, char_get_class(ch));
    snprintf(ptr, sizeof(str) - (ptr - str), "%s%d%s", char_sensei_name_lower(ch),
             rank, char_get_level(ch) == rank ? "" : "+");
    return str;
  }
}

int load_levels()
{
  FILE *fp;
  char line[READ_SIZE], sect_name[READ_SIZE] = {'\0'}, *ptr;
  int linenum = 0, tp, cls, sect_type = -1;

  if (!(fp = fopen(LEVEL_CONFIG, "r"))) {
    mud_log("SYSERR: Could not open level configuration file, error: %s!",
            strerror(errno));
    return -1;
  }

  for (cls = 0; cls < NUM_CLASSES; cls++) {
    for (tp = 0; tp <= SAVING_WILL; tp++) {
      save_classes[tp][cls] = 0;
    }
    basehit_classes[cls] = 0;
  }

  for (;;) {
    linenum++;
    if (!fgets(line, READ_SIZE, fp)) { /* eof check */
      mud_log("SYSERR: Unexpected EOF in file %s.", LEVEL_CONFIG);
      return -1;
    } else if (*line == '$') { /* end of file */
      break;
    } else if (*line == '*') { /* comment line */
      continue;
    } else if (*line == '#') { /* start of a section */
      if ((tp = sscanf(line, "#%s", sect_name)) != 1) {
        mud_log("SYSERR: Format error in file %s, line number %d - text: %s.",
                LEVEL_CONFIG, linenum, line);
        return -1;
      } else if ((sect_type = search_block(sect_name, config_sect, FALSE)) == -1) {
        mud_log("SYSERR: Invalid section in file %s, line number %d: %s.",
                LEVEL_CONFIG, linenum, sect_name);
        return -1;
      }
    } else {
      if (sect_type == CONFIG_LEVEL_VERSION) {
        if (!strncmp(line, "Suntzu", 6)) {
          mud_log("SYSERR: Suntzu %s config files are not compatible with rasputin", LEVEL_CONFIG);
          return -1;
        } else {
          strcpy(level_version, line); /* OK - both are READ_SIZE */
        }
      } else if (sect_type == CONFIG_LEVEL_VERNUM) {
        level_vernum = atoi(line);
      } else if (sect_type == CONFIG_LEVEL_EXPERIENCE) {
        tp             = atoi(line);
        exp_multiplier = tp;
      } else if ((sect_type >= CONFIG_LEVEL_FORTITUDE && sect_type <= CONFIG_LEVEL_WILL) ||
                 sect_type == CONFIG_LEVEL_BASEHIT) {
        for (ptr = line; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (!ptr || !*ptr || !isdigit(*ptr)) {
          mud_log("SYSERR: Cannot find class number in file %s, line number %d, section %s.",
                  LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        cls = atoi(ptr);
        for (; ptr && *ptr && isdigit(*ptr); ptr++);
        for (; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (ptr && *ptr && !isdigit(*ptr)) {
          mud_log("SYSERR: Non-numeric entry in file %s, line number %d, section %s.",
                  LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (ptr && *ptr) /* There's a value */
          tp = atoi(ptr);
        else {
          mud_log("SYSERR: Need 1 value in %s, line number %d, section %s.",
                  LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (cls < 0 || cls >= NUM_CLASSES) {
          mud_log("SYSERR: Invalid class number %d in file %s, line number %d.",
                  cls, LEVEL_CONFIG, linenum);
          return -1;
        } else {
          if (sect_type == CONFIG_LEVEL_BASEHIT) {
            basehit_classes[cls] = tp;
          } else {
            save_classes[SAVING_FORTITUDE + sect_type - CONFIG_LEVEL_FORTITUDE][cls] = tp;
          }
        }
      } else {
        mud_log("Unsupported level config option");
      }
    }
  }
  fclose(fp);

  return 0;
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int highest_skill_value(int level, int type)
{
  if (level >= 60)
    return 100;
  else if (level >= 20)
    return level + 40;
  else if (level >= 10)
    return level + 30;
  else if (level >= 1)
    return level + 25;
  else
    return 0;
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
/* Not anymore because for DBZ it was crap and unnecessary. */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
time_t birth_age(struct char_data *ch)
{
  int tmp;

  tmp = rand_number(16, 18);

  return tmp;
}

time_t max_age(struct char_data *ch)
{
  return 120;
}

/*
 * Rogues follow opposite logic - they can take any feat in place of these,
 * all of these are abilities that are not normally able to be taken as
 * feats. Most classes can ONLY take from these lists for their class
 * feats.
 */

/* Sensei behaviour. A sensei is identified by its CLASS_* constant (ch->chclass). */

bool sensei_exists(int s)
{
  switch (s) {
    case CLASS_ROSHI:
    case CLASS_PICCOLO:
    case CLASS_KRANE:
    case CLASS_NAIL:
    case CLASS_BARDOCK:
    case CLASS_GINYU:
    case CLASS_FRIEZA:
    case CLASS_TAPION:
    case CLASS_ANDSIX:
    case CLASS_DABURA:
    case CLASS_KABITO:
    case CLASS_JINTO:
    case CLASS_TSUNA:
    case CLASS_KURZAK:
    case CLASS_NPC_COMMONER:
      return true;
    default:
      return false;
  }
}

const char *sensei_name(int s)
{
  switch (s) {
    case CLASS_ROSHI:
      return "Roshi";
    case CLASS_PICCOLO:
      return "Piccolo";
    case CLASS_KRANE:
      return "Krane";
    case CLASS_NAIL:
      return "Nail";
    case CLASS_BARDOCK:
      return "Bardock";
    case CLASS_GINYU:
      return "Ginyu";
    case CLASS_FRIEZA:
      return "Frieza";
    case CLASS_TAPION:
      return "Tapion";
    case CLASS_ANDSIX:
      return "Android 16";
    case CLASS_DABURA:
      return "Dabura";
    case CLASS_KABITO:
      return "Kibito";
    case CLASS_JINTO:
      return "Jinto";
    case CLASS_TSUNA:
      return "Tsuna";
    case CLASS_KURZAK:
      return "Kurzak";
    case CLASS_NPC_COMMONER:
      return "Commoner";
    default:
      return "Unknown";
  }
}

static const char *sensei_abbr(int s)
{
  switch (s) {
    case CLASS_ROSHI:
      return "Ro";
    case CLASS_PICCOLO:
      return "Pi";
    case CLASS_KRANE:
      return "Kr";
    case CLASS_NAIL:
      return "Na";
    case CLASS_BARDOCK:
      return "Ba";
    case CLASS_GINYU:
      return "Gi";
    case CLASS_FRIEZA:
      return "Fr";
    case CLASS_TAPION:
      return "Ta";
    case CLASS_ANDSIX:
      return "16";
    case CLASS_DABURA:
      return "Da";
    case CLASS_KABITO:
      return "Ki";
    case CLASS_JINTO:
      return "Ji";
    case CLASS_TSUNA:
      return "Ts";
    case CLASS_KURZAK:
      return "Kurzak";
    case CLASS_NPC_COMMONER:
      return "--";
    default:
      return "??";
  }
}

bool sensei_is_pc_ok(int s)
{
  switch (s) {
    case CLASS_NPC_COMMONER:
      return false;
    default:
      return sensei_exists(s);
  }
}

bool sensei_available_for_race(int s, int r)
{
  switch (s) {
    case CLASS_ANDSIX:
      return r == RACE_ANDROID;
    case CLASS_DABURA:
      return r == RACE_DEMON;
    case CLASS_TSUNA:
      return r == RACE_KANASSAN;
    case CLASS_KURZAK:
      return r == RACE_ARLIAN;
    case CLASS_JINTO:
      return r == RACE_HOSHIJIN;
    default:
      return r != RACE_ANDROID;
  }
}

int sensei_find(const char *arg)
{
  for (int s = 0; s < NUM_CLASSES; s++) {
    if (sensei_exists(s) && !strcasecmp(arg, sensei_name(s)))
      return s;
  }
  return CLASS_UNDEFINED;
}
