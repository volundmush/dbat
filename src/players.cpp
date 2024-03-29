/* ************************************************************************
*   File: players.c                                     Part of CircleMUD *
*  Usage: Player loading/saving and utility routines                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "players.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "pfdefaults.h"
#include "feats.h"
#include "dg_scripts.h"
#include "comm.h"
#include "genmob.h"
#include "constants.h"
#include "imc.h"
#include "class.h"
#include "config.h"

#define LOAD_HIT	0
#define LOAD_MANA	1
#define LOAD_MOVE	2
#define LOAD_KI		3
#define LOAD_LIFE       4

/* local functions */
void build_player_index(void);
void save_etext(struct char_data *ch);
int sprintascii(char *out, bitvector_t bits);
void tag_argument(char *argument, char *tag);
void load_affects(FILE *fl, struct char_data *ch, int violence);
void load_bonuses(FILE *fl, struct char_data *ch, bool mods);
void load_skills(FILE *fl, struct char_data *ch, bool mods);
void load_feats(FILE *fl, struct char_data *ch);
void load_HMVS(struct char_data *ch, const char *line, int mode);
void load_BASE(struct char_data *ch, const char *line, int mode);
void load_molt(struct char_data *ch, const char *line);
void load_majin(struct char_data *ch, const char *line);


/* 'global' vars */
struct player_index_element *player_table = NULL;	/* index to plr file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;		/* ref of size of p file	 */
long top_idnum = 0;		/* highest idnum in use		 */


/* external ASCII Player Files vars */

/* ASCII Player Files - set this TRUE if you want poofin/poofout
   strings saved in the pfiles
 */
#define ASCII_SAVE_POOFS  FALSE


/*************************************************************************
*  stuff related to the player index					 *
*************************************************************************/

void load_imc_pfile( struct char_data *ch );
/*char *imc_fread_word( char *buf, size_t len, FILE *fp );*/
void save_imc_pfile(struct char_data *ch);

void save_imc_pfile(struct char_data *ch)
{
  FILE *fl;
  char filename[PATH_MAX];

  if (!CH_IMCDATA(ch))
    return;

  if (!get_filename(filename, sizeof(filename), IMC_FILE, GET_PC_NAME(ch)))
    return;

  if (!(fl = fopen(filename, "w")))
  {
    if (errno != ENOENT)
      log("SYSERR: opening IMC2 file '%s' for writing: %s", filename, strerror(errno));
    return;
  }

  imc_savechar(ch, fl);
  fclose(fl);
}

void load_imc_pfile(struct char_data *ch)
{
  FILE *fl;
  char filename[PATH_MAX];
  char *word;

  if (!get_filename(filename, sizeof(filename), IMC_FILE, GET_PC_NAME(ch)))
    return;

  if (!(fl = fopen(filename, "r")))
  {
    if (errno != ENOENT)
      log("SYSERR: opening IMC2 file '%s' for reading: %s", filename, strerror(errno));
    return;
  }

  for (;;)
  {
    word = imcfread_word(fl);
    /*imc_fread_word(word, sizeof(word), fl);*/
    if (*word != 'I')
      break;
    imc_loadchar(ch, fl, word);
  }
  fclose(fl);
}

/* new version to build player index for ASCII Player Files */
/* generate index table for the player file */
void build_player_index(void)
{
  int rec_count = 0, i;
  FILE *plr_index;
  char index_name[40], line[256], bits[64];
  char arg2[80];

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(plr_index = fopen(index_name, "r"))) {
    top_of_p_table = -1;
    log("No player index file!  First new char will be IMP!");
    return;
  }

  while (get_line(plr_index, line))
    if (*line != '~')
      rec_count++;
  rewind(plr_index);

  if (rec_count == 0) {
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }

  CREATE(player_table, struct player_index_element, rec_count);
  for (i = 0; i < rec_count; i++) {
    get_line(plr_index, line);
    player_table[i].admlevel = ADMLVL_NONE; /* In case they're not in the index yet */
    sscanf(line, "%ld %s %d %s %ld %d %d %d %ld", &player_table[i].id, arg2,
      &player_table[i].level, bits, &player_table[i].last, &player_table[i].admlevel, &player_table[i].ship, &player_table[i].shiproom, &player_table[i].played);
    CREATE(player_table[i].name, char, strlen(arg2) + 1);
    strcpy(player_table[i].name, arg2);
    player_table[i].flags = asciiflag_conv(bits);
    top_idnum = MAX(top_idnum, player_table[i].id);
  }

  fclose(plr_index);
  top_of_p_file = top_of_p_table = i - 1;
}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(char *name)
{
  int i, pos;

  if (top_of_p_table == -1) {	/* no table */
    CREATE(player_table, struct player_index_element, 1);
    pos = top_of_p_table = 0;
  } else if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
	/* Nothing */;

  /* clear the bitflag in case we have garbage data */
  player_table[pos].flags = 0;

  return (pos);
}


/* This function necessary to save a seperate ASCII player index */
void save_player_index(void)
{
  int i;
  char index_name[50], bits[64];
  FILE *index_file;

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w"))) {
    log("SYSERR: Could not write player index file");
    return;
  }

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name) {
      sprintascii(bits, player_table[i].flags);
      fprintf(index_file, "%ld %s %d %s %ld %d %d %d %ld\n", player_table[i].id,
	player_table[i].name, player_table[i].level, *bits ? bits : "0",
	player_table[i].last, player_table[i].admlevel, player_table[i].ship, player_table[i].shiproom, player_table[i].played);
    }
  fprintf(index_file, "~\n");

  fclose(index_file);
}


void free_player_index(void)
{
  int tp;

  if (!player_table)
    return;

  for (tp = 0; tp <= top_of_p_table; tp++) {
    if (player_table[tp].name)
      free(player_table[tp].name);
  }

  free(player_table);
  player_table = NULL;
  top_of_p_table = 0;
}


long get_ptable_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!strcasecmp(player_table[i].name, name))
      return (i);

  return (-1);
}


long get_id_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!strcasecmp(player_table[i].name, name))
      return (player_table[i].id);

  return (-1);
}


char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name);

  return (NULL);
}


void load_follower_from_file(FILE *fl, struct char_data *ch)
{
  int nr;
  char line[MAX_INPUT_LENGTH + 1];
  struct char_data *newch;

  if (!get_line(fl, line))
    return;

  if (line[0] != '#' || !line[1])
    return;

  nr = atoi(line + 1);
  newch = create_char();
  newch->nr = real_mobile(nr);

  if (!parse_mobile_from_file(fl, newch)) {
    free(newch);
  } else {
    add_follower(newch, ch);
    newch->master_id = GET_IDNUM(ch);
    GET_POS(newch) = POS_STANDING;
  }
}


/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/


#define NUM_OF_SAVE_THROWS	3

/* new load_char reads ASCII Player Files */
/* Load a char, TRUE if loaded, FALSE if not */
int load_char(const char *name, struct char_data *ch)
{
  int id, i, num = 0, num2 = 0, num3 = 0;
  FILE *fl;
  char fname[READ_SIZE];
  char buf[128], buf2[128], line[MAX_INPUT_LENGTH + 1], tag[6];
  char f1[128], f2[128], f3[128], f4[128];

  if ((id = get_ptable_by_name(name)) < 0)
    return (-1);
  else {
    if (!get_filename(fname, sizeof(fname), PLR_FILE, player_table[id].name))
      return (-1);
    if (!(fl = fopen(fname, "r"))) {
      mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open player file %s", fname);
      return (-1);
    }

    /* character initializations */
    /* initializations necessary to keep some things straight */
    ch->affected = NULL;
    ch->affectedv = NULL;
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
      SET_SKILL(ch, i, 0);
      SET_SKILL_BONUS(ch, i, 0);
      SET_SKILL_PERF(ch, i, 0);
    }
    GET_SEX(ch) = PFDEF_SEX;
    ch->size = PFDEF_SIZE;
    ch->chclass = dbat::sensei::sensei_map[dbat::sensei::roshi];
    for (i = 0; i < NUM_CLASSES; i++) {
      GET_CLASS_NONEPIC(ch, i) = 0;
      GET_CLASS_EPIC(ch, i) = 0;
    }
    GET_LOG_USER(ch) = strdup("NOUSER");
    ch->race = dbat::race::find_race_map_id(PFDEF_RACE, dbat::race::race_map);
    GET_ADMLEVEL(ch) = PFDEF_LEVEL;
    GET_CLASS_LEVEL(ch) = PFDEF_LEVEL;
    GET_HITDICE(ch) = PFDEF_LEVEL;
	GET_RBANK(ch) = PFDEF_SKIN;
    GET_RP(ch) = PFDEF_SKIN;
    GET_TRP(ch) = PFDEF_SKIN;
    GET_SUPPRESS(ch) = PFDEF_SKIN;
    GET_SUPP(ch) = PFDEF_SKIN;
    GET_FURY(ch) = PFDEF_HAIRL;
    GET_CLAN(ch) = strdup("None.");
    GET_LEVEL_ADJ(ch) = PFDEF_LEVEL;
    GET_HOME(ch) = PFDEF_HOMETOWN;
    GET_HEIGHT(ch) = PFDEF_HEIGHT;
    GET_WEIGHT(ch) = PFDEF_WEIGHT;
    ch->basepl = PFDEF_BASEPL;
    ch->health = 1.0;
    GET_RELAXCOUNT(ch) = PFDEF_EYE;
    GET_BLESSLVL(ch) = PFDEF_HEIGHT;
    ch->life = 1.0;
    // GET_LIFEFORCE(ch) = PFDEF_BASEPL;
    GET_LIFEPERC(ch) = PFDEF_WEIGHT;
    GET_STUPIDKISS(ch) = 0;
    GET_POS(ch) = POS_STANDING;
    GET_MAJINIZED(ch) = PFDEF_BASEPL;
    GET_GAUNTLET(ch)   = PFDEF_GAUNTLET;
    ch->baseki = PFDEF_BASEKI;
    ch->energy = 1.0;
    ch->basest = PFDEF_BASEST;
    ch->stamina = 1.0;
    GET_HAIRL(ch) = PFDEF_HAIRL;
    GET_HAIRC(ch) = PFDEF_HAIRC;
    GET_SKIN(ch) = PFDEF_SKIN;
    GET_EYE(ch) = PFDEF_EYE;
    GET_HAIRS(ch) = PFDEF_HAIRS;
    GET_DISTFEA(ch) = PFDEF_DISTFEA;
    GET_RADAR1(ch) = PFDEF_RADAR1;
    GET_SHIP(ch) = PFDEF_SHIP;
    GET_LPLAY(ch) = PFDEF_LPLAY;
    GET_BOOSTS(ch) = PFDEF_DISTFEA;
    MAJINIZED(ch) = PFDEF_DISTFEA;
    GET_LINTEREST(ch) = PFDEF_LPLAY;
    GET_DTIME(ch) = PFDEF_LPLAY;
    GET_PHASE(ch) = PFDEF_EYE;
    ch->mimic = nullptr;
    GET_SLOTS(ch) = 0;
    GET_TGROWTH(ch) = 0;
    GET_TRAINSTR(ch) = PFDEF_EYE;
    GET_TRAINSPD(ch) = PFDEF_EYE;
    GET_TRAINWIS(ch) = PFDEF_EYE;
    GET_TRAINAGL(ch) = PFDEF_EYE;
    GET_TRAINCON(ch) = PFDEF_EYE;
    GET_TRAININT(ch) = PFDEF_EYE;
    GET_RTIME(ch) = PFDEF_LPLAY;
    GET_DCOUNT(ch) = PFDEF_EYE;
    GET_GENOME(ch, 0) = PFDEF_EYE;
    GET_PREFERENCE(ch) = PFDEF_EYE;
    GET_GENOME(ch, 1) = PFDEF_EYE;
    GET_AURA(ch) = PFDEF_SKIN;
    for (i = 0; i < 52; i++) {
     GET_BONUS(ch, i) = PFDEF_BOARD;
    }
    GET_GROUPKILLS(ch) = 0;
    GET_SONG(ch) = 0;
    GET_LIMBCOND(ch, 0) = 0;
    GET_LIMBCOND(ch, 1) = 0;
    GET_LIMBCOND(ch, 2) = 0;
    GET_LIMBCOND(ch, 3) = 0;
    GET_BOARD(ch, 0) = PFDEF_BOARD;
    GET_BOARD(ch, 1) = PFDEF_BOARD;
    GET_BOARD(ch, 2) = PFDEF_BOARD;
    GET_BOARD(ch, 3) = PFDEF_BOARD;
    GET_BOARD(ch, 4) = PFDEF_BOARD;
    GET_SHIPROOM(ch) = PFDEF_SHIPROOM;
    GET_RADAR2(ch) = PFDEF_RADAR2;
    GET_RADAR3(ch) = PFDEF_RADAR3;
    GET_DROOM(ch) = PFDEF_DROOM;
    GET_CRANK(ch) = PFDEF_CRANK;
    GET_ALIGNMENT(ch) = PFDEF_ALIGNMENT;
    GET_ETHIC_ALIGNMENT(ch) = PFDEF_ETHIC_ALIGNMENT;
    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = PFDEF_AFFFLAGS;
    for (i = 0; i < PM_ARRAY_MAX; i++)
      PLR_FLAGS(ch)[i] = PFDEF_PLRFLAGS;
    for (i = 0; i < PR_ARRAY_MAX; i++)
      PRF_FLAGS(ch)[i] = PFDEF_PREFFLAGS;
    for (i = 0; i < AD_ARRAY_MAX; i++)
      ADM_FLAGS(ch)[i] = 0;
    for (i = 0; i < NUM_OF_SAVE_THROWS; i++) {
      GET_SAVE_MOD(ch, i) = PFDEF_SAVETHROW;
      GET_SAVE_BASE(ch, i) = PFDEF_SAVETHROW;
    }
    GET_LOADROOM(ch) = PFDEF_LOADROOM;
    GET_INVIS_LEV(ch) = PFDEF_INVISLEV;
    GET_FREEZE_LEV(ch) = PFDEF_FREEZELEV;
    GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;
    GET_POWERATTACK(ch) = PFDEF_POWERATT;
    GET_COND(ch, HUNGER) = PFDEF_HUNGER;
    GET_COND(ch, THIRST) = PFDEF_THIRST;
    GET_COND(ch, DRUNK) = PFDEF_DRUNK;
    GET_BAD_PWS(ch) = PFDEF_BADPWS;
    GET_RACE_PRACTICES(ch) = PFDEF_PRACTICES;
    for (i = 0; i < NUM_CLASSES; i++)
      GET_PRACTICES(ch, i) = PFDEF_PRACTICES;
    GET_GOLD(ch) = PFDEF_GOLD;
    GET_BACKSTAB_COOL(ch) = 0;
    GET_COOLDOWN(ch) = 0;
	GET_SDCOOLDOWN(ch) = 0;
    GET_BANK_GOLD(ch) = PFDEF_BANK;
    GET_ABSORBS(ch) = PFDEF_BANK;
    GET_INGESTLEARNED(ch) = PFDEF_BANK;
    RACIAL_PREF(ch) = PFDEF_BANK;
    GET_UP(ch) = PFDEF_BANK;
    GET_FORGETING(ch) = PFDEF_BANK;
    GET_FORGET_COUNT(ch) = PFDEF_BANK;
    GET_KAIOKEN(ch) = PFDEF_BANK;
    GET_EXP(ch) = PFDEF_EXP;
    GET_TRANSCLASS(ch) = PFDEF_EXP;
    for (i = 0; i < 6; i++)
     GET_TRANSCOST(ch, i) = FALSE;
    GET_MOLT_EXP(ch) = PFDEF_EXP;
    GET_FISHD(ch) = PFDEF_ACCURACY;
    GET_POLE_BONUS(ch) = PFDEF_ACCURACY;
    GET_DAMAGE_MOD(ch) = PFDEF_DAMAGE;
    GET_ARMOR(ch) = PFDEF_AC;
    ch->real_abils.str = PFDEF_STR;
    ch->real_abils.dex = PFDEF_DEX;
    ch->real_abils.intel = PFDEF_INT;
    ch->real_abils.wis = PFDEF_WIS;
    ch->real_abils.con = PFDEF_CON;
    ch->real_abils.cha = PFDEF_CHA;
    //GET_HIT(ch) = PFDEF_HIT;
    //ch->max_hit = PFDEF_MAXHIT;
    //GET_MANA(ch) = PFDEF_MANA;
    //ch->max_mana = PFDEF_MAXMANA;
    //GET_MOVE(ch) = PFDEF_MOVE;
    //ch->max_move = PFDEF_MAXMOVE;
    GET_KI(ch) = PFDEF_KI;
    GET_MAX_KI(ch) = PFDEF_MAXKI;
    SPEAKING(ch) = PFDEF_SPEAKING;
    GET_OLC_ZONE(ch) = PFDEF_OLC;
    GET_HOST(ch) = NULL;
    for(i = 1; i < MAX_MEM; i++)
      GET_SPELLMEM(ch, i) = 0;
    for(i = 0; i < MAX_SPELL_LEVEL; i++)
      GET_SPELL_LEVEL(ch, i) = 0;
    GET_MEMCURSOR(ch) = 0;
    ch->time.birth = ch->time.created = ch->time.maxage = 0;
    ch->followers = NULL;
    GET_PAGE_LENGTH(ch) = PFDEF_PAGELENGTH;
    for (i = 0; i < NUM_COLOR; i++)
      ch->player_specials->color_choices[i] = NULL;

    while (get_line(fl, line)) {
      tag_argument(line, tag);

      switch (*tag) {
      case 'A':
             if (!strcmp(tag, "Ac  "))  GET_ARMOR(ch)           = atoi(line);
        else if (!strcmp(tag, "Act ")) {
          sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            PLR_FLAGS(ch)[0] = asciiflag_conv(f1);
            PLR_FLAGS(ch)[1] = asciiflag_conv(f2);
            PLR_FLAGS(ch)[2] = asciiflag_conv(f3);
            PLR_FLAGS(ch)[3] = asciiflag_conv(f4);
        } else if (!strcmp(tag, "Aff ")) {
          sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            AFF_FLAGS(ch)[0] = asciiflag_conv(f1);
            AFF_FLAGS(ch)[1] = asciiflag_conv(f2);
            AFF_FLAGS(ch)[2] = asciiflag_conv(f3);
            AFF_FLAGS(ch)[3] = asciiflag_conv(f4);
	} else if (!strcmp(tag, "Affs"))  load_affects(fl, ch, 0);
	else if (!strcmp(tag, "Affv"))  load_affects(fl, ch, 1);
	else if (!strcmp(tag, "AdmL"))  GET_ADMLEVEL(ch)	= atoi(line);
        else if (!strcmp(tag, "Abso"))  GET_ABSORBS(ch)         = atoi(line);
        else if (!strcmp(tag, "AdmF")) {
          sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
          ADM_FLAGS(ch)[0] = asciiflag_conv(f1);
          ADM_FLAGS(ch)[1] = asciiflag_conv(f2);
          ADM_FLAGS(ch)[2] = asciiflag_conv(f3);
          ADM_FLAGS(ch)[3] = asciiflag_conv(f4);
        } else if (!strcmp(tag, "Alin"))  GET_ALIGNMENT(ch)       = atoi(line);
          else if (!strcmp(tag, "Aura"))  GET_AURA(ch)            = atoi(line);
      break;

      case 'B':
             if (!strcmp(tag, "Badp"))  GET_BAD_PWS(ch)         = atoi(line);
        else if (!strcmp(tag, "Bank"))  GET_BANK_GOLD(ch)       = atoi(line);
        else if (!strcmp(tag, "Bki "))  load_BASE(ch, line, LOAD_MANA);
        else if (!strcmp(tag, "Blss"))  GET_BLESSLVL(ch)        = atoi(line);
        else if (!strcmp(tag, "Boam"))  GET_BOARD(ch, 0)        = atoi(line);
        else if (!strcmp(tag, "Boai"))  GET_BOARD(ch, 1)        = atoi(line);
        else if (!strcmp(tag, "Boac"))  GET_BOARD(ch, 2)        = atoi(line);
        else if (!strcmp(tag, "Boad"))  GET_BOARD(ch, 3)        = atoi(line);
        else if (!strcmp(tag, "Boab"))  GET_BOARD(ch, 4)        = atoi(line);
        else if (!strcmp(tag, "Bonu"))  load_bonuses(fl, ch, FALSE);
        else if (!strcmp(tag, "Boos"))  GET_BOOSTS(ch)          = atoi(line);
        else if (!strcmp(tag, "Bpl "))  load_BASE(ch, line, LOAD_HIT);
        else if (!strcmp(tag, "Brth"))  ch->time.birth          = atol(line);
        else if (!strcmp(tag, "Bst "))  load_BASE(ch, line, LOAD_MOVE);
      break;

      case 'C':
             if (!strcmp(tag, "Cha "))  ch->real_abils.cha      = atoi(line);
        else if (!strcmp(tag, "Clan"))  GET_CLAN(ch)            = strdup(line);
        else if (!strcmp(tag, "Clar"))  GET_CRANK(ch)           = atoi(line);
	else if (!strcmp(tag, "Clas"))  ch->chclass = dbat::sensei::find_sensei_map_id(atoi(line), dbat::sensei::sensei_map);
        else if (!strcmp(tag, "Colr"))  {
          sscanf(line, "%d %s", &num, buf2);
          ch->player_specials->color_choices[num] = strdup(buf2);
        }
        else if (!strcmp(tag, "Con "))  ch->real_abils.con      = atoi(line);
        else if (!strcmp(tag, "Cool"))  GET_COOLDOWN(ch)        = atoi(line);
        else if (!strcmp(tag, "Crtd"))  ch->time.created        = atol(line);
      break;

      case 'D':
             if (!strcmp(tag, "Deat"))  GET_DTIME(ch)     = atoi(line);
        else if (!strcmp(tag, "Deac"))  GET_DCOUNT(ch)    = atoi(line);
        else if (!strcmp(tag, "Desc"))  ch->description  = fread_string(fl, buf2);
        else if (!strcmp(tag, "Dex "))  ch->real_abils.dex      = atoi(line);
        else if (!strcmp(tag, "Drnk"))  GET_COND(ch, DRUNK)     = atoi(line);
        else if (!strcmp(tag, "Damg"))  GET_DAMAGE_MOD(ch)          = atoi(line);
        else if (!strcmp(tag, "Droo"))  GET_DROOM(ch)          = atoi(line);
      break;

      case 'E':
             if (!strcmp(tag, "Exp "))  GET_EXP(ch)             = atoi(line);
        else if (!strcmp(tag, "Eali"))  GET_ETHIC_ALIGNMENT(ch) = atoi(line);
        else if (!strcmp(tag, "Ecls"))  { sscanf(line, "%d=%d", &num, &num2);
                                          GET_CLASS_EPIC(ch, num) = num2; }
        else if (!strcmp(tag, "Eye "))  GET_EYE(ch)             = atoi(line);
      break;

      case 'F':
             if (!strcmp(tag, "Fisd"))  GET_FISHD(ch)           = atoi(line);
        else if (!strcmp(tag, "Frez"))  GET_FREEZE_LEV(ch)      = atoi(line);
        else if (!strcmp(tag, "Forc"))  GET_FORGET_COUNT(ch) = atoi(line);
        else if (!strcmp(tag, "Forg"))  GET_FORGETING(ch) = atoi(line);
        else if (!strcmp(tag, "Fury"))  GET_FURY(ch)      = atoi(line);
      break;

      case 'G':
             if (!strcmp(tag, "Gold"))  GET_GOLD(ch)            = atoi(line);
        else if (!strcmp(tag, "Gaun"))  GET_GAUNTLET(ch)        = atoi(line);
        else if (!strcmp(tag, "Geno"))  GET_GENOME(ch, 0)        = atoi(line);
        else if (!strcmp(tag, "Gen1"))  GET_GENOME(ch, 1)        = atoi(line);
      break;

      case 'H':
             if (!strcmp(tag, "Hit "))  load_HMVS(ch, line, LOAD_HIT);
        else if (!strcmp(tag, "HitD"))  GET_HITDICE(ch)         = atoi(line);
        else if (!strcmp(tag, "Hite"))  GET_HEIGHT(ch)          = atoi(line);
        else if (!strcmp(tag, "Home"))  GET_HOME(ch)            = atoi(line);
        else if (!strcmp(tag, "Host")) {
          if (GET_HOST(ch))
            free(GET_HOST(ch));
          GET_HOST(ch) = strdup(line);
        }
        else if (!strcmp(tag, "Hrc "))  GET_HAIRC(ch)           = atoi(line);
        else if (!strcmp(tag, "Hrl "))  GET_HAIRL(ch)           = atoi(line);
        else if (!strcmp(tag, "Hrs "))  GET_HAIRS(ch)           = atoi(line);
        else if (!strcmp(tag, "Hung"))  GET_COND(ch, HUNGER)      = atoi(line);
      break;

      case 'I':
             if (!strcmp(tag, "Id  "))  GET_IDNUM(ch)           = atol(line);
        else if (!strcmp(tag, "INGl"))  GET_INGESTLEARNED(ch) = atoi(line);
        else if (!strcmp(tag, "Int "))  ch->real_abils.intel    = atoi(line);
        else if (!strcmp(tag, "Invs"))  GET_INVIS_LEV(ch)       = atoi(line);
      break;

      case 'K':
             if (!strcmp(tag, "Ki  "))  load_HMVS(ch, line, LOAD_KI);
        else if (!strcmp(tag, "Kaio"))  GET_KAIOKEN(ch)     = atoi(line);
      break;

      case 'L':
             if (!strcmp(tag, "Last"))  ch->time.logon          = atol(line);
        else if (!strcmp(tag, "Lern"))  GET_PRACTICES(ch, GET_CLASS(ch)) = atoi(line);
        else if (!strcmp(tag, "Levl"))  GET_CLASS_LEVEL(ch)     = atoi(line);
       /* else if (!strcmp(tag, "LevD"))  read_level_data(ch, fl);*/
        else if (!strcmp(tag, "LF  "))  load_BASE(ch, line, LOAD_LIFE);
        else if (!strcmp(tag, "LFPC"))  GET_LIFEPERC(ch)      = atoi(line);
        else if (!strcmp(tag, "Lila"))  GET_LIMBCOND(ch, 2)      = atoi(line);
        else if (!strcmp(tag, "Lill"))  GET_LIMBCOND(ch, 4)      = atoi(line);
        else if (!strcmp(tag, "Lira"))  GET_LIMBCOND(ch, 1)      = atoi(line);
        else if (!strcmp(tag, "Lirl"))  GET_LIMBCOND(ch, 3)      = atoi(line);
        else if (!strcmp(tag, "Lint"))  GET_LINTEREST(ch)     = atoi(line);
        else if (!strcmp(tag, "Lpla"))  GET_LPLAY(ch)         = atoi(line);
        else if (!strcmp(tag, "LvlA"))  GET_LEVEL_ADJ(ch)     = atoi(line);
      break;

      case 'M':
             if (!strcmp(tag, "Mana"))  load_HMVS(ch, line, LOAD_MANA);
        else if (!strcmp(tag, "Mexp"))  load_molt(ch, line);
        else if (!strcmp(tag, "Mlvl"))  GET_MOLT_LEVEL(ch)      = atoi(line);
        else if (!strcmp(tag, "Move"))  load_HMVS(ch, line, LOAD_MOVE);
        else if (!strcmp(tag, "Mcls"))  { sscanf(line, "%d=%d", &num, &num2);
                                          GET_CLASS_NONEPIC(ch, num) = num2; }
        else if (!strcmp(tag, "Maji"))  MAJINIZED(ch)           = atoi(line);
        else if (!strcmp(tag, "Majm"))  load_majin(ch, line);
        else if (!strcmp(tag, "Mimi"))  ch->mimic = dbat::race::find_race_map_id(atoi(line), dbat::race::race_map);
        else if (!strcmp(tag, "MxAg"))  ch->time.maxage         = atol(line);
      break;

      case 'N':
             if (!strcmp(tag, "Name"))  GET_PC_NAME(ch)         = strdup(line);
      break;

      case 'O':
             if (!strcmp(tag, "Olc "))  GET_OLC_ZONE(ch) = atoi(line);
      break;

      case 'P':
            if (!strcmp(tag, "Page"))  GET_PAGE_LENGTH(ch) = atoi(line);
       else if (!strcmp(tag, "Phas"))  GET_DISTFEA(ch)            = atoi(line);
       else if (!strcmp(tag, "Phse"))  GET_PHASE(ch)            = atoi(line);
       else if (!strcmp(tag, "Plyd"))  ch->time.played          = atol(line);
#ifdef ASCII_SAVE_POOFS
       else if (!strcmp(tag, "PfIn"))  POOFIN(ch)               = strdup(line);
       else if (!strcmp(tag, "PfOt"))  POOFOUT(ch)              = strdup(line);
#endif
       else if (!strcmp(tag, "Pole"))  GET_POLE_BONUS(ch)       = atoi(line);
       else if (!strcmp(tag, "Posi"))  GET_POS(ch)              = atoi(line);
       else if (!strcmp(tag, "PwrA"))  GET_POWERATTACK(ch)      = atoi(line);
       else if (!strcmp(tag, "Pref")) {
         sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
         PRF_FLAGS(ch)[0] = asciiflag_conv(f1);
         PRF_FLAGS(ch)[1] = asciiflag_conv(f2);
         PRF_FLAGS(ch)[2] = asciiflag_conv(f3);
         PRF_FLAGS(ch)[3] = asciiflag_conv(f4);
       }
       else if (!strcmp(tag, "Prff"))  GET_PREFERENCE(ch)       = atoi(line);
      break;

      case 'R':
             if (!strcmp(tag, "Race"))  ch->race = dbat::race::find_race_map_id(atoi(line), dbat::race::race_map);
        else if (!strcmp(tag, "Raci"))  RACIAL_PREF(ch)         = atoi(line);
	else if (!strcmp(tag, "RBan"))  GET_RBANK(ch)           = atoi(line);
	else if (!strcmp(tag, "rDis"))  GET_RDISPLAY(ch)        = strdup(line);
        else if (!strcmp(tag, "Rela"))  GET_RELAXCOUNT(ch)      = atoi(line);
        else if (!strcmp(tag, "Rtim"))  GET_RTIME(ch)           = atoi(line);
        else if (!strcmp(tag, "Rad1"))  GET_RADAR1(ch)          = atoi(line);
        else if (!strcmp(tag, "Rad2"))  GET_RADAR2(ch)          = atoi(line);
        else if (!strcmp(tag, "Rad3"))  GET_RADAR3(ch)          = atoi(line);
        else if (!strcmp(tag, "Room"))  GET_LOADROOM(ch)        = atoi(line);
        else if (!strcmp(tag, "RPfe"))  GET_FEATURE(ch)         = strdup(line);
        else if (!strcmp(tag, "RPP "))  GET_RP(ch)        = atoi(line);
      break;

      case 'S':
             if (!strcmp(tag, "Sex "))  GET_SEX(ch)             = atoi(line);
        else if (!strcmp(tag, "Ship"))  GET_SHIP(ch)            = atoi(line);
		else if (!strcmp(tag, "Scoo"))  GET_SDCOOLDOWN(ch)      = atoi(line);
        else if (!strcmp(tag, "Shpr"))  GET_SHIPROOM(ch)        = atoi(line);
        else if (!strcmp(tag, "Skil"))  load_skills(fl, ch, FALSE);
        else if (!strcmp(tag, "Skn "))  GET_SKIN(ch)            = atoi(line);
        else if (!strcmp(tag, "Size"))  ch->size		= atoi(line);
        else if (!strcmp(tag, "SklB"))  load_skills(fl, ch, TRUE);
        else if (!strcmp(tag, "SkRc"))  GET_RACE_PRACTICES(ch)	= atoi(line);
        else if (!strcmp(tag, "SkCl")) {
          sscanf(line, "%d %d", &num2, &num3);
          GET_PRACTICES(ch, num2) = num3;
        }
        else if (!strcmp(tag, "Slot"))  ch->skill_slots         = atoi(line);
        else if (!strcmp(tag, "Spek"))  SPEAKING(ch)		= atoi(line);
        else if (!strcmp(tag, "Str "))  ch->real_abils.str	= atoi(line);
        else if (!strcmp(tag, "Stuk"))  ch->stupidkiss          = atoi(line);
        else if (!strcmp(tag, "Supp"))  GET_SUPPRESS(ch)        = atoi(line);
        else if (!strcmp(tag, "Sups"))  GET_SUPP(ch)        = atoi(line);
      break;

      case 'T':
             if (!strcmp(tag, "Tgro"))  GET_TGROWTH(ch)         = atoi(line);
        else if (!strcmp(tag, "Tcla"))  GET_TRANSCLASS(ch)      = atoi(line);
        else if (!strcmp(tag, "Tcos")) {
          sscanf(line, "%d %d", &num2, &num3);
          GET_TRANSCOST(ch, num2) = num3;
        }
        else if (!strcmp(tag, "Thir"))  GET_COND(ch, THIRST)    = atoi(line);
        else if (!strcmp(tag, "Thr1"))  GET_SAVE_MOD(ch, 0)     = atoi(line);
        else if (!strcmp(tag, "Thr2"))  GET_SAVE_MOD(ch, 1)     = atoi(line);
        else if (!strcmp(tag, "Thr3"))  GET_SAVE_MOD(ch, 2)     = atoi(line);
        else if (!strcmp(tag, "Thr4") || !strcmp(tag, "Thr5")); /* Discard extra saves */
        else if (!strcmp(tag, "ThB1"))  GET_SAVE_BASE(ch, 0)    = atoi(line);
        else if (!strcmp(tag, "ThB2"))  GET_SAVE_BASE(ch, 1)    = atoi(line);
        else if (!strcmp(tag, "ThB3"))  GET_SAVE_BASE(ch, 2)    = atoi(line);
        else if (!strcmp(tag, "Trns"))  GET_TRAINS(ch)          = atoi(line);
        else if (!strcmp(tag, "Trag"))  GET_TRAINAGL(ch)        = atoi(line);
        else if (!strcmp(tag, "Trco"))  GET_TRAINCON(ch)        = atoi(line);
        else if (!strcmp(tag, "Trin"))  GET_TRAININT(ch)        = atoi(line);
        else if (!strcmp(tag, "Trsp"))  GET_TRAINSPD(ch)        = atoi(line);
        else if (!strcmp(tag, "Trst"))  GET_TRAINSTR(ch)        = atoi(line);
        else if (!strcmp(tag, "Trwi"))  GET_TRAINWIS(ch)        = atoi(line);
        else if (!strcmp(tag, "Trp "))  GET_TRP(ch)             = atoi(line);
      break;
      case 'U':
             if (!strcmp(tag, "Upgr"))  GET_UP(ch)              = atoi(line);
        else if (!strcmp(tag, "User")) {
         if (GET_LOG_USER(ch)) {
          free(GET_LOG_USER(ch));
         }
         GET_LOG_USER(ch)        = strdup(line);
        }
      break;
      case 'V':
             if (!strcmp(tag, "Voic"))  GET_VOICE(ch)           = strdup(line);
      break;

      case 'W':
             if (!strcmp(tag, "Wate"))  GET_WEIGHT(ch)          = atoi(line);
        else if (!strcmp(tag, "Wimp"))  GET_WIMP_LEV(ch)        = atoi(line);
        else if (!strcmp(tag, "Wis "))  ch->real_abils.wis      = atoi(line);
      break;

      default:
	sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
      }
    }
  }

  if (! ch->time.created) {
    log("No creation timestamp for user %s, using current time", GET_NAME(ch));
    ch->time.created = time(0);
  }

  if (! ch->time.birth) {
    log("No birthday for user %s, using standard starting age determination", GET_NAME(ch));
    ch->time.birth = time(0) - birth_age(ch);
  }

  if (! ch->time.maxage) {
    log("No max age for user %s, using standard max age determination", GET_NAME(ch));
    ch->time.maxage = ch->time.birth + max_age(ch);
  }

  affect_total(ch);

  /* initialization for imms */
  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
    for (i = 1; i <= SKILL_TABLE_SIZE; i++)
      SET_SKILL(ch, i, 100);
    GET_COND(ch, HUNGER) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }

  if (CONFIG_IMC_ENABLED) {
    imc_initchar(ch);
    load_imc_pfile(ch);
  }

  if (IS_ANDROID(ch)) {
    GET_COND(ch, HUNGER) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }
  fclose(fl);
  return(id);
}


/* remove ^M's from file output */
/* There may be a similar function in Oasis (and I'm sure
   it's part of obuild).  Remove this if you get a
   multiple definition error or if it you want to use a
   substitute
*/
void kill_ems(char *str)
{
  char *ptr1, *ptr2, *tmp;

  tmp = str;
  ptr1 = str;
  ptr2 = str;

  while (*ptr1) {
    if ((*(ptr2++) = *(ptr1++)) == '\r')
      if (*ptr1 == '\r')
	ptr1++;
  }
  *ptr2 = '\0';
}


void save_char_pets(struct char_data *ch)
{
  struct follow_type *foll;
  char fname[40];
  FILE *fl;

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  if (!get_filename(fname, sizeof(fname), PET_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "w"))) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open pet file %s for write", fname);
    return;
  }

  for (foll = ch->followers; foll; foll = foll->next) {
        write_mobile_record(GET_MOB_VNUM(foll->follower), foll->follower, fl);
  }

  fclose(fl);
}


void load_char_pets(struct char_data *ch)
{
  char fname[40];
  FILE *fl;
  IDXTYPE load_room;
  struct follow_type *foll;

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  if (!get_filename(fname, sizeof(fname), PET_FILE, GET_NAME(ch)))
    return;

  if (!(fl = fopen(fname, "r")))
    return;

  while (!feof(fl))
    load_follower_from_file(fl, ch);
   /*load_room = IN_ROOM(ch);*/
 
  for (foll = ch->followers; foll; foll = foll->next) {
       load_room = real_room(1);
  if (load_room == NOWHERE) {
       load_room = real_room(IN_ROOM(ch));
  }
    char_to_room(foll->follower, load_room);
    act("You are joined by $N.", FALSE, ch, 0, foll->follower, TO_CHAR);
  }
}


/*
 * write the vital data of a player to the player file
 *
 * And that's it! No more fudging around with the load room.
 */
/* This is the ASCII Player Files save routine */
void save_char(struct char_data * ch)
{
  FILE *fl;
  char fname[40], buf[MAX_STRING_LENGTH];
  int i, id, save_index = FALSE;
  struct affected_type *aff, tmp_aff[MAX_AFFECT], tmp_affv[MAX_AFFECT];
  struct obj_data *char_eq[NUM_WEARS];
  char fbuf1[MAX_STRING_LENGTH], fbuf2[MAX_STRING_LENGTH];
  char fbuf3[MAX_STRING_LENGTH], fbuf4[MAX_STRING_LENGTH];

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  /*
   * If ch->desc is not null, then we need to update some session data
   * before saving.
   */
  if (ch->desc) {
    if (ch->desc->host && *ch->desc->host) {
      if (!GET_HOST(ch))
        GET_HOST(ch) = strdup(ch->desc->host);
      else if (GET_HOST(ch) && !strcmp(GET_HOST(ch), ch->desc->host)) {
        free(GET_HOST(ch));
      GET_HOST(ch) = strdup(ch->desc->host);
      }
    }

    /*
     * We only update the time.played and time.logon if the character
     * is playing.
     */
    if (STATE(ch->desc) == CON_PLAYING) {
      ch->time.played += time(0) - ch->time.logon;
      ch->time.logon = time(0);
    }
  }

  if (!get_filename(fname, sizeof(fname), PLR_FILE, GET_NAME(ch)))
    return;
  if (!(fl = fopen(fname, "w"))) {
    mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open player file %s for write", fname);
    return;
  }

  /* remove affects from eq and spells (from char_to_store) */
  /* Unaffect everything a character can be affected by */

  if (GET_TRP(ch) < GET_RP(ch)) {
   GET_TRP(ch) = GET_RP(ch);
  }
  if (ch->desc != NULL && ch->desc->user != NULL) {
   userWrite(ch->desc, 0, 0, 0, "index");
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      char_eq[i] = unequip_char(ch, i);
#ifndef NO_EXTRANEOUS_TRIGGERS
      remove_otrigger(char_eq[i], ch);
#endif
    else
      char_eq[i] = NULL;
  }

  for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (aff) {
      tmp_aff[i] = *aff;
      tmp_aff[i].next = 0;
      aff = aff->next;
    } else {
      tmp_aff[i].type = 0;	/* Zero signifies not used */
      tmp_aff[i].duration = 0;
      tmp_aff[i].modifier = 0;
      tmp_aff[i].specific = 0;
      tmp_aff[i].location = 0;
      tmp_aff[i].bitvector = 0;
      tmp_aff[i].next = 0;
    }
  }

  for (aff = ch->affectedv, i = 0; i < MAX_AFFECT; i++) {
    if (aff) {
      tmp_affv[i] = *aff;
      tmp_affv[i].next = 0;
      aff = aff->next;
    } else {
      tmp_affv[i].type = 0;      /* Zero signifies not used */
      tmp_affv[i].duration = 0;
      tmp_affv[i].modifier = 0;
      tmp_affv[i].location = 0;
      tmp_affv[i].specific = 0;
      tmp_affv[i].bitvector = 0;
      tmp_affv[i].next = 0;
    }
  }

  save_char_vars(ch);

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  while (ch->affectedv)
    affectv_remove(ch, ch->affectedv);

  if ((i >= MAX_AFFECT) && aff && aff->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  /* end char_to_store code */

  if (GET_NAME(ch))				fprintf(fl, "Name: %s\n", GET_NAME(ch));
  if (GET_USER(ch))                             fprintf(fl, "User: %s\n", GET_USER(ch));
  if (GET_VOICE(ch))                            fprintf(fl, "Voic: %s\n", GET_VOICE(ch));
  if (GET_CLAN(ch))                             fprintf(fl, "Clan: %s\n", GET_CLAN(ch));
  if (GET_FEATURE(ch))                          fprintf(fl, "RPfe: %s\n", GET_FEATURE(ch));
  if (GET_TRAINS(ch))				fprintf(fl, "Trns: %d\n", GET_TRAINS(ch));
  if (ch->description && *ch->description) {
    strcpy(buf, ch->description);
    kill_ems(buf);
    fprintf(fl, "Desc:\n%s~\n", buf);
  }
#ifdef ASCII_SAVE_POOFS
  if (POOFIN(ch))				fprintf(fl, "PfIn: %s\n", POOFIN(ch));
  if (POOFOUT(ch))				fprintf(fl, "PfOt: %s\n", POOFOUT(ch));
#endif
  if (GET_SEX(ch)	   != PFDEF_SEX)	fprintf(fl, "Sex : %d\n", GET_SEX(ch)); 
  if (ch->size		   != PFDEF_SIZE)	fprintf(fl, "Size: %d\n", ch->size); 
  if (GET_CLASS(ch)	   != PFDEF_CLASS)	fprintf(fl, "Clas: %d\n", GET_CLASS(ch)); 
  if (GET_RACE(ch)	   != PFDEF_RACE)	fprintf(fl, "Race: %d\n", GET_RACE(ch));
  if (RACIAL_PREF(ch)      != PFDEF_BANK)       fprintf(fl, "Raci: %d\n", RACIAL_PREF(ch));
  if (GET_ADMLEVEL(ch)	   != PFDEF_LEVEL)	fprintf(fl, "AdmL: %d\n", GET_ADMLEVEL(ch));
  if (GET_CLASS_LEVEL(ch)  != PFDEF_LEVEL)	fprintf(fl, "Levl: %d\n", GET_CLASS_LEVEL(ch));
  if (GET_HITDICE(ch)      != PFDEF_LEVEL)	fprintf(fl, "HitD: %d\n", GET_HITDICE(ch));
  if (GET_LEVEL_ADJ(ch)    != PFDEF_LEVEL)	fprintf(fl, "LvlA: %d\n", GET_LEVEL_ADJ(ch));
  if (GET_HOME(ch)	   != PFDEF_HOMETOWN)	fprintf(fl, "Home: %d\n", GET_HOME(ch));
  for (i = 0; i < NUM_CLASSES; i++) {
    if(GET_CLASS_RANKS(ch, i))	fprintf(fl, "Mcls: %d=%d\n", i, GET_CLASS_NONEPIC(ch, i));
    if(GET_CLASS_EPIC(ch, i))	fprintf(fl, "Ecls: %d=%d\n", i, GET_CLASS_EPIC(ch, i));
  }
  fprintf(fl, "Id  : %d\n", GET_IDNUM(ch));
  fprintf(fl, "Brth: %ld\n", ch->time.birth);
  fprintf(fl, "Crtd: %ld\n", ch->time.created);
  fprintf(fl, "MxAg: %ld\n", ch->time.maxage);
  fprintf(fl, "Plyd: %ld\n", ch->time.played);
  fprintf(fl, "Last: %ld\n", ch->time.logon);

  if (GET_HOST(ch))				fprintf(fl, "Host: %s\n", GET_HOST(ch));
  if (GET_HEIGHT(ch)	   != PFDEF_HEIGHT)	fprintf(fl, "Hite: %d\n", GET_HEIGHT(ch));
  if (GET_WEIGHT(ch)	   != PFDEF_HEIGHT)	fprintf(fl, "Wate: %d\n", GET_WEIGHT(ch));
  if (GET_ALIGNMENT(ch)	   != PFDEF_ALIGNMENT)	fprintf(fl, "Alin: %d\n", GET_ALIGNMENT(ch));
  if (GET_AURA(ch)         != PFDEF_SKIN)       fprintf(fl, "Aura: %d\n", GET_AURA(ch));
  if (GET_ETHIC_ALIGNMENT(ch)	   != PFDEF_ETHIC_ALIGNMENT)	fprintf(fl, "Eali: %d\n", GET_ETHIC_ALIGNMENT(ch));

  sprintascii(fbuf1, PLR_FLAGS(ch)[0]);
  sprintascii(fbuf2, PLR_FLAGS(ch)[1]);
  sprintascii(fbuf3, PLR_FLAGS(ch)[2]);
  sprintascii(fbuf4, PLR_FLAGS(ch)[3]);
  fprintf(fl, "Act : %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, AFF_FLAGS(ch)[0]);
  sprintascii(fbuf2, AFF_FLAGS(ch)[1]);
  sprintascii(fbuf3, AFF_FLAGS(ch)[2]);
  sprintascii(fbuf4, AFF_FLAGS(ch)[3]);
  fprintf(fl, "Aff : %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, PRF_FLAGS(ch)[0]);
  sprintascii(fbuf2, PRF_FLAGS(ch)[1]);
  sprintascii(fbuf3, PRF_FLAGS(ch)[2]);
  sprintascii(fbuf4, PRF_FLAGS(ch)[3]);
  fprintf(fl, "Pref: %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, ADM_FLAGS(ch)[0]);
  sprintascii(fbuf2, ADM_FLAGS(ch)[1]);
  sprintascii(fbuf3, ADM_FLAGS(ch)[2]);
  sprintascii(fbuf4, ADM_FLAGS(ch)[3]);
  fprintf(fl, "AdmF: %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);

  if (GET_SAVE_BASE(ch, 0)  != PFDEF_SAVETHROW)	fprintf(fl, "ThB1: %d\n", GET_SAVE_BASE(ch, 0));
  if (GET_SAVE_BASE(ch, 1)  != PFDEF_SAVETHROW)	fprintf(fl, "ThB2: %d\n", GET_SAVE_BASE(ch, 1));
  if (GET_SAVE_BASE(ch, 2)  != PFDEF_SAVETHROW)	fprintf(fl, "ThB3: %d\n", GET_SAVE_BASE(ch, 2));
  if (GET_SAVE_MOD(ch, 0)   != PFDEF_SAVETHROW)	fprintf(fl, "Thr1: %d\n", GET_SAVE_MOD(ch, 0));
  if (GET_SAVE_MOD(ch, 1)   != PFDEF_SAVETHROW)	fprintf(fl, "Thr2: %d\n", GET_SAVE_MOD(ch, 1));
  if (GET_SAVE_MOD(ch, 2)   != PFDEF_SAVETHROW)	fprintf(fl, "Thr3: %d\n", GET_SAVE_MOD(ch, 2));

  if (GET_WIMP_LEV(ch)	   != PFDEF_WIMPLEV)	fprintf(fl, "Wimp: %d\n", GET_WIMP_LEV(ch));
  if (GET_POWERATTACK(ch)  != PFDEF_POWERATT)	fprintf(fl, "PwrA: %d\n", GET_POWERATTACK(ch));
  if (GET_FREEZE_LEV(ch)   != PFDEF_FREEZELEV)	fprintf(fl, "Frez: %d\n", GET_FREEZE_LEV(ch));
  if (GET_INVIS_LEV(ch)	   != PFDEF_INVISLEV)	fprintf(fl, "Invs: %d\n", GET_INVIS_LEV(ch));
  if (GET_LOADROOM(ch)	   != PFDEF_LOADROOM)	fprintf(fl, "Room: %d\n", GET_LOADROOM(ch));

  if (GET_BAD_PWS(ch)	   != PFDEF_BADPWS)	fprintf(fl, "Badp: %d\n", GET_BAD_PWS(ch));

  if (GET_RACE_PRACTICES(ch)!= PFDEF_PRACTICES)	fprintf(fl, "SkRc: %d\n", GET_RACE_PRACTICES(ch));
  for (i = 0; i < 6; i++)
   if (GET_TRANSCOST(ch, i) != FALSE)
    fprintf(fl, "Tcos: %d %d\n", i, GET_TRANSCOST(ch, i));

  for (i = 0; i < NUM_CLASSES; i++)
    if (GET_PRACTICES(ch, i)!= PFDEF_PRACTICES)
      fprintf(fl, "SkCl: %d %d\n", i, GET_PRACTICES(ch, i));

  if (GET_COND(ch, HUNGER)   != PFDEF_HUNGER && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Hung: %d\n", GET_COND(ch, HUNGER));
  if (GET_COND(ch, THIRST) != PFDEF_THIRST && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Thir: %d\n", GET_COND(ch, THIRST));
  if (GET_COND(ch, DRUNK)  != PFDEF_DRUNK  && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Drnk: %d\n", GET_COND(ch, DRUNK));

  /*
  if (GET_HIT(ch)	   != PFDEF_HIT  || GET_MAX_HIT(ch)  != PFDEF_MAXHIT)
    fprintf(fl, "Hit : %" I64T "/%" I64T "\n", GET_HIT(ch),  GET_MAX_HIT(ch));
  if ((ch->getCurKI())	   != PFDEF_MANA || GET_MAX_MANA(ch) != PFDEF_MAXMANA)
    fprintf(fl, "Mana: %" I64T "/%" I64T "\n", (ch->getCurKI()), GET_MAX_MANA(ch));
  if ((ch->getCurST())	   != PFDEF_MOVE || GET_MAX_MOVE(ch) != PFDEF_MAXMOVE)
    fprintf(fl, "Move: %" I64T "/%" I64T "\n", (ch->getCurST()), GET_MAX_MOVE(ch));
  if (GET_KI(ch)	   != PFDEF_KI || GET_MAX_KI(ch) != PFDEF_MAXKI)
    fprintf(fl, "Ki  : %" I64T "/%" I64T "\n", GET_KI(ch), GET_MAX_KI(ch));
  */
  if (GET_STR(ch)	   != PFDEF_STR)        fprintf(fl, "Str : %d\n", GET_STR(ch));
  if (GET_INT(ch)	   != PFDEF_INT)	fprintf(fl, "Int : %d\n", GET_INT(ch));
  if (GET_WIS(ch)	   != PFDEF_WIS)	fprintf(fl, "Wis : %d\n", GET_WIS(ch));
  if (GET_DEX(ch)	   != PFDEF_DEX)	fprintf(fl, "Dex : %d\n", GET_DEX(ch));
  if (GET_CON(ch)	   != PFDEF_CON)	fprintf(fl, "Con : %d\n", GET_CON(ch));
  if (GET_CHA(ch)	   != PFDEF_CHA)	fprintf(fl, "Cha : %d\n", GET_CHA(ch));

  if (GET_COOLDOWN(ch)     != PFDEF_BANK)       fprintf(fl, "Cool: %d\n", GET_COOLDOWN(ch));
  if (GET_COOLDOWN(ch)     != PFDEF_BANK)       fprintf(fl, "Scoo: %d\n", GET_SDCOOLDOWN(ch));
  if (GET_ARMOR(ch)	       != PFDEF_AC)		    fprintf(fl, "Ac  : %d\n", GET_ARMOR(ch));
  if (GET_ABSORBS(ch)      != PFDEF_GOLD)       fprintf(fl, "Abso: %d\n", GET_ABSORBS(ch));
  if (GET_INGESTLEARNED(ch)      != PFDEF_GOLD)       fprintf(fl, "INGl: %d\n", GET_INGESTLEARNED(ch));
  if (GET_UP(ch)           != PFDEF_GOLD)       fprintf(fl, "Upgr: %d\n", GET_UP(ch));
  if (GET_FORGETING(ch)    != PFDEF_BANK)       fprintf(fl, "Forg: %d\n", GET_FORGETING(ch));
  if (GET_FORGET_COUNT(ch) != PFDEF_BANK)       fprintf(fl, "Forc: %d\n", GET_FORGET_COUNT(ch));
  if (GET_KAIOKEN(ch)      != PFDEF_GOLD)       fprintf(fl, "Kaio: %d\n", GET_KAIOKEN(ch));
  if (GET_GOLD(ch)	   != PFDEF_GOLD)	fprintf(fl, "Gold: %d\n", GET_GOLD(ch));
  if (GET_BANK_GOLD(ch)	   != PFDEF_BANK)	fprintf(fl, "Bank: %d\n", GET_BANK_GOLD(ch));
  if (GET_EXP(ch)	   != PFDEF_EXP)	fprintf(fl, "Exp : %" I64T "\n", GET_EXP(ch));
  if (GET_TRANSCLASS(ch)   != PFDEF_EXP)        fprintf(fl, "Tcla: %d\n", GET_TRANSCLASS(ch));
  if (GET_MOLT_EXP(ch)     != PFDEF_EXP)        fprintf(fl, "Mexp: %" I64T "\n", GET_MOLT_EXP(ch));
  if (GET_MAJINIZED(ch)    != PFDEF_EXP)        fprintf(fl, "Majm: %" I64T "\n", GET_MAJINIZED(ch));
  if (GET_MOLT_LEVEL(ch)   != PFDEF_EXP)        fprintf(fl, "Mlvl: %d\n", GET_MOLT_LEVEL(ch));
  if (GET_FISHD(ch)        != PFDEF_ACCURACY)	fprintf(fl, "Fisd: %d\n", GET_FISHD(ch));
  if (GET_POLE_BONUS(ch)   != PFDEF_ACCURACY)	fprintf(fl, "Pole: %d\n", GET_POLE_BONUS(ch));
  if (GET_PREFERENCE(ch)   != PFDEF_EYE)        fprintf(fl, "Prff: %d\n", GET_PREFERENCE(ch));
  if (GET_DAMAGE_MOD(ch)   != PFDEF_DAMAGE)	fprintf(fl, "Damg: %d\n", GET_DAMAGE_MOD(ch));
  if (SPEAKING(ch)	   != PFDEF_SPEAKING)	fprintf(fl, "Spek: %d\n", SPEAKING(ch));
  if (GET_OLC_ZONE(ch)	   != PFDEF_OLC)	fprintf(fl, "Olc : %d\n", GET_OLC_ZONE(ch));
  if (GET_PAGE_LENGTH(ch)  != PFDEF_PAGELENGTH)	fprintf(fl, "Page: %d\n", GET_PAGE_LENGTH(ch));
  if (GET_GAUNTLET(ch)     != PFDEF_GAUNTLET)   fprintf(fl, "Gaun: %d\n", GET_GAUNTLET(ch));
  if (GET_GENOME(ch, 0)    != PFDEF_EYE)        fprintf(fl, "Geno: %d\n", GET_GENOME(ch, 0));
  if (GET_GENOME(ch, 1)    != PFDEF_EYE)        fprintf(fl, "Gen1: %d\n", GET_GENOME(ch, 1));
  if (GET_POS(ch)          != POS_STANDING)     fprintf(fl, "Posi: %d\n", GET_POS(ch));
  //if ((ch->getCurLF())    != PFDEF_BASEPL)     fprintf(fl, "LF  : %" I64T "\n", (ch->getCurLF()));
  if (GET_LIFEPERC(ch)     != PFDEF_WEIGHT)     fprintf(fl, "LFPC: %d\n", GET_LIFEPERC(ch));
  if ((ch->getBasePL())      != PFDEF_BASEPL)     fprintf(fl, "Bpl : %" I64T "\n", (ch->getBasePL()));
  if ((ch->getBaseKI())      != PFDEF_BASEKI)     fprintf(fl, "Bki : %" I64T "\n", (ch->getBaseKI()));
  if ((ch->getBaseST())      != PFDEF_BASEST)     fprintf(fl, "Bst : %" I64T "\n", (ch->getBaseST()));
  if (GET_DROOM(ch)        != PFDEF_DROOM)      fprintf(fl, "Droo: %d\n", GET_DROOM(ch)); 
  if (GET_HAIRL(ch)        != PFDEF_HAIRL)      fprintf(fl, "Hrl : %d\n", GET_HAIRL(ch));
  if (GET_HAIRS(ch)        != PFDEF_HAIRS)      fprintf(fl, "Hrs : %d\n", GET_HAIRS(ch));
  if (GET_HAIRC(ch)        != PFDEF_HAIRC)      fprintf(fl, "Hrc : %d\n", GET_HAIRC(ch));
  if (GET_SKIN(ch)         != PFDEF_SKIN)       fprintf(fl, "Skn : %d\n", GET_SKIN(ch));
  if (GET_EYE(ch)          != PFDEF_EYE)        fprintf(fl, "Eye : %d\n", GET_EYE(ch));
  if (GET_DISTFEA(ch)      != PFDEF_DISTFEA)      fprintf(fl, "Phas: %d\n", GET_DISTFEA(ch));
  if (GET_FURY(ch)         != PFDEF_HAIRL)      fprintf(fl, "Fury: %d\n", GET_FURY(ch));
  if (GET_RADAR1(ch)       != PFDEF_RADAR1)     fprintf(fl, "Rad1: %d\n", GET_RADAR1(ch));
  if (GET_RADAR2(ch)       != PFDEF_RADAR2)     fprintf(fl, "Rad2: %d\n", GET_RADAR2(ch));
  if (GET_RADAR3(ch)       != PFDEF_RADAR3)     fprintf(fl, "Rad3: %d\n", GET_RADAR3(ch));
  if (GET_SHIP(ch)         != PFDEF_SHIP)       fprintf(fl, "Ship: %d\n", GET_SHIP(ch));
  if (GET_SHIPROOM(ch)     != PFDEF_SHIPROOM)   fprintf(fl, "Shpr: %d\n", GET_SHIPROOM(ch));
  if (GET_LPLAY(ch)        != PFDEF_LPLAY)      fprintf(fl, "Lpla: %ld\n", GET_LPLAY(ch));
  if (GET_LINTEREST(ch)    != PFDEF_LPLAY)      fprintf(fl, "Lint: %ld\n", GET_LINTEREST(ch));
  if (GET_DTIME(ch)        != PFDEF_LPLAY)      fprintf(fl, "Deat: %ld\n", GET_DTIME(ch));
  if (GET_RTIME(ch)        != PFDEF_LPLAY)      fprintf(fl, "Rtim: %ld\n", GET_RTIME(ch));
  if (GET_BOOSTS(ch)       != PFDEF_DISTFEA)     fprintf(fl, "Boos: %d\n",  GET_BOOSTS(ch));
  if (MAJINIZED(ch)        != PFDEF_LPLAY)      fprintf(fl, "Maji: %d\n",  MAJINIZED(ch));
  if (GET_BLESSLVL(ch)     != PFDEF_HEIGHT)     fprintf(fl, "Blss: %d\n", GET_BLESSLVL(ch));
  if (GET_BOARD(ch, 0)     != PFDEF_BOARD)      fprintf(fl, "Boam: %ld\n", GET_BOARD(ch, 0));
  if (GET_BOARD(ch, 1)     != PFDEF_BOARD)      fprintf(fl, "Boai: %ld\n", GET_BOARD(ch, 1));
  if (GET_BOARD(ch, 2)     != PFDEF_BOARD)      fprintf(fl, "Boac: %ld\n", GET_BOARD(ch, 2));
  if (GET_BOARD(ch, 3)     != PFDEF_BOARD)      fprintf(fl, "Boad: %ld\n", GET_BOARD(ch, 3));
  if (GET_BOARD(ch, 4)     != PFDEF_BOARD)      fprintf(fl, "Boab: %ld\n", GET_BOARD(ch, 4));
  if (GET_LIMBCOND(ch, 1)  != PFDEF_BOARD)      fprintf(fl, "Lira: %d\n", GET_LIMBCOND(ch, 1));
  if (GET_LIMBCOND(ch, 2)  != PFDEF_BOARD)      fprintf(fl, "Lila: %d\n", GET_LIMBCOND(ch, 2));
  if (GET_LIMBCOND(ch, 3)  != PFDEF_BOARD)      fprintf(fl, "Lirl: %d\n", GET_LIMBCOND(ch, 3));
  if (GET_LIMBCOND(ch, 4)  != PFDEF_BOARD)      fprintf(fl, "Lill: %d\n", GET_LIMBCOND(ch, 4));
  if (GET_CRANK(ch)        != PFDEF_CRANK)      fprintf(fl, "Clar: %d\n", GET_CRANK(ch));
  if (GET_RP(ch)           != PFDEF_SKIN)       fprintf(fl, "RPP : %d\n", GET_RP(ch));
  if (GET_RBANK(ch)        != PFDEF_SKIN)       fprintf(fl, "RBan: %d\n", GET_RBANK(ch));
  if (GET_SUPPRESS(ch)     != PFDEF_SKIN)       fprintf(fl, "Supp: %" I64T "\n", GET_SUPPRESS(ch));
  if (GET_SUPP(ch)         != PFDEF_SKIN)       fprintf(fl, "Sups: %" I64T "\n", GET_SUPP(ch));
  if (GET_TRP(ch)          != PFDEF_SKIN)       fprintf(fl, "Trp : %d\n", GET_TRP(ch));
  if (GET_DCOUNT(ch)       != PFDEF_EYE)        fprintf(fl, "Deac: %d\n", GET_DCOUNT(ch));
  if (GET_TRAINAGL(ch)     != PFDEF_EYE)        fprintf(fl, "Trag: %d\n", GET_TRAINAGL(ch));
  if (GET_TRAINCON(ch)     != PFDEF_EYE)        fprintf(fl, "Trco: %d\n", GET_TRAINCON(ch));
  if (GET_TRAININT(ch)     != PFDEF_EYE)        fprintf(fl, "Trin: %d\n", GET_TRAININT(ch));
  if (GET_TRAINSPD(ch)     != PFDEF_EYE)        fprintf(fl, "Trsp: %d\n", GET_TRAINSPD(ch));
  if (GET_TRAINSTR(ch)     != PFDEF_EYE)        fprintf(fl, "Trst: %d\n", GET_TRAINSTR(ch));
  if (GET_TRAINWIS(ch)     != PFDEF_EYE)        fprintf(fl, "Trwi: %d\n", GET_TRAINWIS(ch));
  if (GET_PHASE(ch)        != PFDEF_EYE)        fprintf(fl, "Phse: %d\n", GET_PHASE(ch));
  if (GET_MIMIC(ch))        fprintf(fl, "Mimi: %d\n", GET_MIMIC(ch));
  if (GET_SLOTS(ch)        != PFDEF_EYE)        fprintf(fl, "Slot: %d\n", GET_SLOTS(ch));
  if (GET_TGROWTH(ch)      != PFDEF_EYE)        fprintf(fl, "Tgro: %d\n", GET_TGROWTH(ch));
  if (GET_STUPIDKISS(ch)   != PFDEF_EYE)        fprintf(fl, "Stuk: %d\n", GET_STUPIDKISS(ch));
  if (GET_RDISPLAY(ch)     != PFDEF_EYE)        fprintf(fl, "rDis: %s\n", GET_RDISPLAY(ch));
  if (GET_RELAXCOUNT(ch)   != PFDEF_EYE)        fprintf(fl, "Rela: %d\n", GET_RELAXCOUNT(ch));

   /* Save skills */
  if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
    fprintf(fl, "Skil:\n");
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
     if (GET_SKILL_BASE(ch, i))
	fprintf(fl, "%d %d %d\n", i, GET_SKILL_BASE(ch, i), GET_SKILL_PERF(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

  /* Save Bonuses/Negatives */
  char buff[200];
  fprintf(fl, "Bonu:\n");
  for (i = 0; i < 52; i++) {
   if (GET_BONUS(ch, i) && i == 0)
    sprintf(buff, "%d", GET_BONUS(ch, i));
   else if (GET_BONUS(ch, i) && i != 0)
    sprintf(buff+strlen(buff), " %d", GET_BONUS(ch, i));
   else if (i == 0)
    sprintf(buff, "0");
   else
    sprintf(buff+strlen(buff), " 0");
  }
  fprintf(fl, "%s\n", buff);
  *buff = '\0';

  /* Save skill bonuses */
  if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
    fprintf(fl, "SklB:\n");
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
     if (GET_SKILL_BONUS(ch, i))
	fprintf(fl, "%d %d %d\n", i, GET_SKILL_BONUS(ch, i), GET_SKILL_PERF(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

  /* Save feats 
   *  fprintf(fl, "Feat:\n");
   *  for (i = 1; i <= NUM_FEATS_DEFINED; i++) {
   *   if (HAS_FEAT(ch, i))
   *     fprintf(fl, "%d %d\n", i, HAS_FEAT(ch, i));
   *  }
   *  fprintf(fl, "0 0\n"); */

  /* Save affects */
  fprintf(fl, "Affs:\n");
  for (i = 0; i < MAX_AFFECT; i++) {
    aff = &tmp_aff[i];
    if (aff->type)
      fprintf(fl, "%d %d %d %d %d %d\n", aff->type, aff->duration,
      aff->modifier, aff->location, (int)aff->bitvector, aff->specific);
  }
  fprintf(fl, "0 0 0 0 0 0\n");
  fprintf(fl, "Affv:\n");
  for (i = 0; i < MAX_AFFECT; i++) {
    aff = &tmp_affv[i];
    if (aff->type)
      fprintf(fl, "%d %d %d %d %d %d\n", aff->type, aff->duration,
        aff->modifier, aff->location, (int)aff->bitvector, aff->specific);
  }
  fprintf(fl, "0 0 0 0 0 0\n");

  /*fprintf(fl, "LevD:\n");
  write_level_data(ch, fl);*/

  for (i = 0; i < NUM_COLOR; i++)
    if (ch->player_specials->color_choices[i]) {
      fprintf(fl, "Colr: %d %s\r\n", i, ch->player_specials->color_choices[i]);
    }

  fclose(fl);

  /* more char_to_store code to restore affects */

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_aff[i].type)
      affect_to_char(ch, &tmp_aff[i]);
  }
  if (CONFIG_IMC_ENABLED) {
    save_imc_pfile(ch);
  }

  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_affv[i].type)
      affectv_to_char(ch, &tmp_affv[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
#ifndef NO_EXTRANEOUS_TRIGGERS
        if (wear_otrigger(char_eq[i], ch, i))
#endif
      equip_char(ch, char_eq[i], i);
#ifndef NO_EXTRANEOUS_TRIGGERS
          else
          obj_to_char(char_eq[i], ch);
#endif
  }

  /* end char_to_store code */
 
  if ((id = get_ptable_by_name(GET_NAME(ch))) < 0)
    return;

  /* update the player in the player index */
  if (player_table[id].level != GET_LEVEL(ch)) {
    save_index = TRUE;
    player_table[id].level = GET_LEVEL(ch);
  }
  if (player_table[id].admlevel != GET_ADMLEVEL(ch)) {
    save_index = TRUE;
    player_table[id].admlevel = GET_ADMLEVEL(ch);
  }
  if (player_table[id].last != ch->time.logon) {
    save_index = TRUE;
    player_table[id].last = ch->time.logon;
  }
  if (player_table[id].played != GET_LPLAY(ch)) {
    save_index = TRUE;
    player_table[id].played = GET_LPLAY(ch);
  }
  if (GET_CLAN(ch) != NULL && player_table[id].clan != GET_CLAN(ch)) {
    save_index = TRUE;
    player_table[id].clan = strdup(GET_CLAN(ch));
  }
  if (GET_CLAN(ch) == NULL) {
    save_index = TRUE;
    player_table[id].clan = strdup("None.");
  }
  if (player_table[id].ship != GET_SHIP(ch)) {
    save_index = TRUE;
    player_table[id].ship = GET_SHIP(ch);
  }
  if (player_table[id].shiproom != GET_SHIPROOM(ch)) {
    save_index = TRUE;
    player_table[id].shiproom = GET_SHIPROOM(ch);
  }
  i = player_table[id].flags;
  if (PLR_FLAGGED(ch, PLR_DELETED))
    SET_BIT(player_table[id].flags, PINDEX_DELETED);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_DELETED);
  if (PLR_FLAGGED(ch, PLR_NODELETE) || PLR_FLAGGED(ch, PLR_CRYO))
    SET_BIT(player_table[id].flags, PINDEX_NODELETE);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NODELETE);

  if (PLR_FLAGGED(ch, PLR_FROZEN) || PLR_FLAGGED(ch, PLR_NOWIZLIST))
    SET_BIT(player_table[id].flags, PINDEX_NOWIZLIST);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NOWIZLIST);

  if (player_table[id].flags != i || save_index)
    save_player_index();
}

/*
 * Grab one word, ignoring preceding whitespace. Will
 * eat a single whitespace immediately after the word.
 */

void save_etext(struct char_data *ch)
{
/* this will be really cool soon */
}


/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for (i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';
  
  while (*tmp == ':' || *tmp == ' ')
    tmp++;

  while (*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}


void load_affects(FILE *fl, struct char_data *ch, int violence)
{
  int num, num2, num3, num4, num5, num6, i;
  char line[MAX_INPUT_LENGTH + 1];
  struct affected_type af;

  i = 0;
  do {
    get_line(fl, line);
    num = num2 = num3 = num4 = num5 = num6 = 0;
    sscanf(line, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
    if (num != 0) {
      af.type = num;
      af.duration = num2;
      af.modifier = num3;
      af.location = num4;
      af.bitvector = num5;
      af.specific = num6;
      if (violence)
        affectv_to_char(ch, &af);
      else
        affect_to_char(ch, &af);
      i++;
    }
  } while (num != 0);
}


void load_skills(FILE *fl, struct char_data *ch, bool mods)
{
  int num = 0, num2 = 0, num3 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    sscanf(line, "%d %d %d", &num, &num2, &num3);
    if (num != 0) {
      if (mods) {
        SET_SKILL_BONUS(ch, num, num2);
      }
      else {
        SET_SKILL(ch, num, num2);
      }
     SET_SKILL_PERF(ch, num, num3);
    }
  } while (num != 0);
}

void load_bonuses(FILE *fl, struct char_data *ch, bool mods)
{
  int num[52] = {0}, i;
  char line[MAX_INPUT_LENGTH + 1];

  get_line(fl, line);
  sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &num[0], &num[1], &num[2], &num[3], &num[4], &num[5], &num[6], &num[7], &num[8], &num[9], &num[10], &num[11], &num[12], &num[13], &num[14], &num[15], &num[16], &num[17], &num[18], &num[19], &num[20], &num[21], &num[22], &num[23], &num[24], &num[25], &num[26], &num[27], &num[28], &num[29], &num[30], &num[31], &num[32], &num[33], &num[34], &num[35], &num[36], &num[37], &num[38], &num[39], &num[40], &num[41], &num[42], &num[43], &num[44], &num[45], &num[46], &num[47], &num[48], &num[49], &num[50], &num[51]);
  for (i = 0; i < 52; i++) {
   if (num[i] > 0) {
    GET_BONUS(ch, i) = num[i];
   }
  }
}

void load_feats(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
      if (num != 0)
	HAS_FEAT(ch, num) = num2;
  } while (num != 0);
}

void load_HMVS(struct char_data *ch, const char *line, int mode)
{
  int64_t num = 0, num2 = 0;

  sscanf(line, "%" I64T "/%" I64T "", &num, &num2);

  switch (mode) {
  case LOAD_HIT:
    //GET_HIT(ch) = num;
    ch->max_hit = num2;
    break;

  case LOAD_MANA:
    //GET_MANA(ch) = num;
    ch->max_mana = num2;
    break;

  case LOAD_MOVE:
    //GET_MOVE(ch) = num;
    ch->max_move = num2;
    break;

  case LOAD_KI:
    //GET_KI(ch) = num;
    ch->max_ki = num2;
    break;
  }
}

void load_BASE(struct char_data *ch, const char *line, int mode)
{
  int64_t num = 0;

  sscanf(line, "%" I64T "", &num);

  switch (mode) {
  case LOAD_HIT:
    ch->basepl = num;
    break;

  case LOAD_MANA:
      ch->baseki = num;
    break;

  case LOAD_MOVE:
      ch->basest = num;
    break;
  
  case LOAD_LIFE:
    //GET_LIFEFORCE(ch) = num;
    break;
  }
}

void load_majin(struct char_data *ch, const char *line)
{
  int64_t num = 0;

  sscanf(line, "%" I64T "", &num);
  GET_MAJINIZED(ch) = num;

}

void load_molt(struct char_data *ch, const char *line)
{
  int64_t num = 0;

  sscanf(line, "%" I64T "", &num);
  GET_MOLT_EXP(ch) = num;

}


/*************************************************************************
*  stuff related to the player file cleanup system			 *
*************************************************************************/


/*
 * remove_player() removes all files associated with a player who is
 * self-deleted, deleted by an immortal, or deleted by the auto-wipe
 * system (if enabled).
 */ 
void remove_player(int pfilepos)
{
  char fname[40];
  int i;

  if (!*player_table[pfilepos].name)
    return;

  /* Unlink all player-owned files */
  for (i = 0; i < MAX_FILES; i++) {
    if (get_filename(fname, sizeof(fname), i, player_table[pfilepos].name))
      unlink(fname);
    if (get_filename(fname, sizeof(fname), i, CAP(player_table[pfilepos].name)))
      unlink(fname);
  }

  log("PCLEAN: %s Lev: %d Last: %s",
	player_table[pfilepos].name, player_table[pfilepos].level,
	asctime(localtime(&player_table[pfilepos].last)));
  player_table[pfilepos].name[0] = '\0';
  save_player_index();
}


void clean_pfiles(void)
{
  int i, ci;

  for (i = 0; i <= top_of_p_table; i++) {
    /*
     * We only want to go further if the player isn't protected
     * from deletion and hasn't already been deleted.
     */
    if (!IS_SET(player_table[i].flags, PINDEX_NODELETE) &&
        *player_table[i].name) {
      /*
       * If the player is already flagged for deletion, then go
       * ahead and get rid of him.
       */
      if (IS_SET(player_table[i].flags, PINDEX_DELETED)) {
	remove_player(i);
      } else {
        /*
         * Now we check to see if the player has overstayed his
         * welcome based on level.
         */
	for (ci = 0; pclean_criteria[ci].level > -1; ci++) {
          if (player_table[i].admlevel > 1)
           continue;
	  else if (player_table[i].level <= pclean_criteria[ci].level && ((time(0) - player_table[i].last) >= (pclean_criteria[ci].days * SECS_PER_REAL_DAY))) {
	    remove_player(i);
	    break;
	  }
	}
	/*
         * If we got this far and the players hasn't been kicked out,
         * then he can stay a little while longer.
         */
      }
    }
  }
  /*
   * After everything is done, we should rebuild player_index and
   * remove the entries of the players that were just deleted.
   */
}
