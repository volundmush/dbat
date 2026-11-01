/* ************************************************************************
*   File: players.c                                     Part of CircleMUD *
*  Usage: Player loading/saving and utility routines                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/players.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/pfdefaults.h"
#include "dbat/feats.h"
#include "dbat/dg_scripts.h"
#include "dbat/comm.h"
#include "dbat/genmob.h"
#include "dbat/class.h"
#include "dbat/config.h"
#include "dbat/fileop.h"
#include "dbat/affect.h"
#include "dbat/interpreter.h"

#include <unistd.h>

#define LOAD_HIT  0
#define LOAD_MANA 1
#define LOAD_MOVE 2
#define LOAD_KI   3
#define LOAD_LIFE 4

/* local functions */
void build_player_index(void);
int sprintascii(char *out, bitvector_t bits);
static void tag_argument(char *argument, char *tag);
static void load_affects(FILE *fl, struct char_data *ch, int violence);
static void load_bonuses(FILE *fl, struct char_data *ch, bool mods);
static void load_skills(FILE *fl, struct char_data *ch, bool mods);
static void load_HMVS(struct char_data *ch, const char *line, int mode);
static void load_BASE(struct char_data *ch, const char *line, int mode);
static void load_molt(struct char_data *ch, const char *line);
static void load_majin(struct char_data *ch, const char *line);

/* 'global' vars */
struct player_index_element *player_table = NULL; /* index to plr file	 */
int top_of_p_table                        = 0;    /* ref to top of table		 */
static int top_of_p_file                  = 0;    /* ref of size of p file	 */
long top_idnum                            = 0;    /* highest idnum in use		 */

/* external ASCII Player Files vars */

/* ASCII Player Files - set this TRUE if you want poofin/poofout
   strings saved in the pfiles
 */
#define ASCII_SAVE_POOFS FALSE

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
    mud_log("No player index file!  First new char will be IMP!");
    return;
  }

  while (get_line(plr_index, line))
    if (*line != '~')
      rec_count++;
  rewind(plr_index);

  if (rec_count == 0) {
    player_table   = NULL;
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
    top_idnum             = MAX(top_idnum, player_table[i].id);
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

  if (top_of_p_table == -1) { /* no table */
    CREATE(player_table, struct player_index_element, 1);
    pos = top_of_p_table = 0;
  } else if ((pos = get_ptable_by_name(name)) == -1) { /* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table[pos].name[i] = tolower(name[i])); i++)
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
    mud_log("SYSERR: Could not write player index file");
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
  player_table   = NULL;
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

/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/

#define NUM_OF_SAVE_THROWS 3

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
      mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open player file %s", fname);
      return (-1);
    }

    /* character initializations */
    /* initializations necessary to keep some things straight */
    ch->affected = NULL;

    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
      char_set_skills_base(ch, i, 0);
      char_set_skills_mod(ch, i, 0);
      char_set_skills_perf(ch, i, 0);
    }
    char_set_sex(ch, PFDEF_SEX);
    ch->size    = PFDEF_SIZE;
    ch->chclass = CLASS_ROSHI;
    char_set_loguser(ch, strdup("NOUSER"));
    ch->race = PFDEF_RACE;
    char_set_admlevel(ch, PFDEF_LEVEL);
    char_set_class_level(ch, PFDEF_LEVEL);
    char_set_race_level(ch, PFDEF_LEVEL);
    char_set_rbank(ch, PFDEF_SKIN);
    char_set_rp(ch, PFDEF_SKIN);
    char_set_trp(ch, PFDEF_SKIN);
    char_set_suppression(ch, PFDEF_SKIN);
    char_set_suppressed(ch, PFDEF_SKIN);
    char_set_fury(ch, PFDEF_HAIRL);
    char_set_level_adj(ch, PFDEF_LEVEL);
    char_set_hometown(ch, PFDEF_HOMETOWN);
    char_set_height(ch, PFDEF_HEIGHT);
    char_set_weight(ch, PFDEF_WEIGHT);
    ch->basepl = PFDEF_BASEPL;
    ch->health = 1.0;
    char_set_relax_count(ch, PFDEF_EYE);
    char_set_blesslvl(ch, PFDEF_HEIGHT);
    ch->life = 1.0;
    // GET_LIFEFORCE(ch) = PFDEF_BASEPL;
    char_set_lifeperc(ch, PFDEF_WEIGHT);
    char_set_position(ch, POS_STANDING);
    char_set_majinizer(ch, PFDEF_BASEPL);
    char_set_gauntlet(ch, PFDEF_GAUNTLET);
    ch->baseki  = PFDEF_BASEKI;
    ch->energy  = 1.0;
    ch->basest  = PFDEF_BASEST;
    ch->stamina = 1.0;
    char_set_hairl(ch, PFDEF_HAIRL);
    char_set_hairc(ch, PFDEF_HAIRC);
    char_set_skin(ch, PFDEF_SKIN);
    char_set_eye(ch, PFDEF_EYE);
    char_set_hairs(ch, PFDEF_HAIRS);
    char_set_distfea(ch, PFDEF_DISTFEA);
    char_set_radar1(ch, PFDEF_RADAR1);
    char_set_lastpl(ch, PFDEF_LPLAY);
    char_set_boosts(ch, PFDEF_DISTFEA);
    char_set_majinize(ch, PFDEF_DISTFEA);
    char_set_lastint(ch, PFDEF_LPLAY);
    char_set_deathtime(ch, PFDEF_LPLAY);
    char_set_starphase(ch, PFDEF_EYE);
    ch->mimic = 0;
    char_set_skill_slots(ch, 0);
    char_set_tail_growth(ch, 0);
    char_set_trainstr(ch, PFDEF_EYE);
    char_set_trainspd(ch, PFDEF_EYE);
    char_set_trainwis(ch, PFDEF_EYE);
    char_set_trainagl(ch, PFDEF_EYE);
    char_set_traincon(ch, PFDEF_EYE);
    char_set_trainint(ch, PFDEF_EYE);
    char_set_rewtime(ch, PFDEF_LPLAY);
    char_set_dcount(ch, PFDEF_EYE);
    char_set_genome(ch, 0, PFDEF_EYE);
    char_set_preference(ch, PFDEF_EYE);
    char_set_genome(ch, 1, PFDEF_EYE);
    char_set_aura(ch, PFDEF_SKIN);
    for (i = 0; i < 52; i++) {
      char_set_bonuses(ch, i, PFDEF_BOARD);
    }
    char_set_combatexpertise(ch, 0);
    char_set_powerattack(ch, 0);
    char_set_limb_condition(ch, (1) - 1, 0);
    char_set_limb_condition(ch, (2) - 1, 0);
    char_set_limb_condition(ch, (3) - 1, 0);
    char_set_limb_condition(ch, (4) - 1, 0);
    char_set_lboard(ch, 0, PFDEF_BOARD);
    char_set_lboard(ch, 1, PFDEF_BOARD);
    char_set_lboard(ch, 2, PFDEF_BOARD);
    char_set_lboard(ch, 3, PFDEF_BOARD);
    char_set_lboard(ch, 4, PFDEF_BOARD);
    char_set_radar2(ch, PFDEF_RADAR2);
    char_set_radar3(ch, PFDEF_RADAR3);
    char_set_droom(ch, PFDEF_DROOM);
    char_set_alignment(ch, PFDEF_ALIGNMENT);
    for (i = 0; i < AF_ARRAY_MAX; i++)
      char_get_affected_by(ch)[i] = PFDEF_AFFFLAGS;
    for (i = 0; i < PM_ARRAY_MAX; i++)
      char_get_act(ch)[i] = PFDEF_PLRFLAGS;
    for (i = 0; i < PR_ARRAY_MAX; i++)
      char_get_pref(ch)
          [i] = PFDEF_PREFFLAGS;
    for (i = 0; i < AD_ARRAY_MAX; i++)
      char_get_admflags(ch)[i] = 0;
    char_set_load_room(ch, PFDEF_LOADROOM);
    char_set_invis_level(ch, PFDEF_INVISLEV);
    char_set_freeze_level(ch, PFDEF_FREEZELEV);
    char_set_wimp_level(ch, PFDEF_WIMPLEV);
    char_set_powerattack(ch, PFDEF_POWERATT);
    char_set_conditions(ch, HUNGER, PFDEF_HUNGER);
    char_set_conditions(ch, THIRST, PFDEF_THIRST);
    char_set_conditions(ch, DRUNK, PFDEF_DRUNK);
    char_set_bad_pws(ch, PFDEF_BADPWS);
    for (i = 0; i < NUM_CLASSES; i++)
      char_set_practice_points(ch, PFDEF_PRACTICES);
    char_set_gold(ch, PFDEF_GOLD);
    char_set_backstabcool(ch, 0);
    char_set_con_cooldown(ch, 0);
    char_set_con_sdcooldown(ch, 0);
    char_set_bank_gold(ch, PFDEF_BANK);
    char_set_absorbs(ch, PFDEF_BANK);
    char_set_ingestLearned(ch, PFDEF_BANK);
    char_set_racial_pref(ch, PFDEF_BANK);
    char_set_upgrade(ch, PFDEF_BANK);
    char_set_forgeting(ch, PFDEF_BANK);
    char_set_forgetcount(ch, PFDEF_BANK);
    char_set_kaioken(ch, PFDEF_BANK);
    char_set_exp(ch, PFDEF_EXP);
    char_set_transclass(ch, PFDEF_EXP);
    char_set_moltexp(ch, PFDEF_EXP);
    char_set_accuracy_mod(ch, PFDEF_ACCURACY);
    char_set_accuracy(ch, PFDEF_ACCURACY);
    char_set_armor(ch, PFDEF_AC);
    ch->real_abils.str   = PFDEF_STR;
    ch->real_abils.dex   = PFDEF_DEX;
    ch->real_abils.intel = PFDEF_INT;
    ch->real_abils.wis   = PFDEF_WIS;
    ch->real_abils.con   = PFDEF_CON;
    ch->real_abils.cha   = PFDEF_CHA;
    //GET_HIT(ch) = PFDEF_HIT;
    //ch->max_hit = PFDEF_MAXHIT;
    //GET_MANA(ch) = PFDEF_MANA;
    //ch->max_mana = PFDEF_MAXMANA;
    //GET_MOVE(ch) = PFDEF_MOVE;
    //ch->max_move = PFDEF_MAXMOVE;
    char_set_ki(ch, PFDEF_KI);
    char_set_max_ki(ch, PFDEF_MAXKI);
    char_set_speaking(ch, PFDEF_SPEAKING);
    char_set_olc_zone(ch, PFDEF_OLC);
    char_set_host(ch, NULL);

    ch->time.birth = ch->time.created = ch->time.maxage = 0;
    ch->followers                                       = NULL;
    char_set_page_length(ch, PFDEF_PAGELENGTH);
    for (i = 0; i < NUM_COLOR; i++)
      ch->player_specials->color_choices[i] = NULL;

    while (get_line(fl, line)) {
      tag_argument(line, tag);

      switch (*tag) {
        case 'A':
          if (!strcmp(tag, "Ac  ")) char_set_armor(ch, atoi(line));
          else if (!strcmp(tag, "Act ")) {
            sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            char_get_act(ch)[0] = asciiflag_conv(f1);
            char_get_act(ch)[1] = asciiflag_conv(f2);
            char_get_act(ch)[2] = asciiflag_conv(f3);
            char_get_act(ch)[3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Aff ")) {
            sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            char_get_affected_by(ch)[0] = asciiflag_conv(f1);
            char_get_affected_by(ch)[1] = asciiflag_conv(f2);
            char_get_affected_by(ch)[2] = asciiflag_conv(f3);
            char_get_affected_by(ch)[3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Affs")) load_affects(fl, ch, 0);
          else if (!strcmp(tag, "Affv")) load_affects(fl, ch, 1);
          else if (!strcmp(tag, "AdmL")) char_set_admlevel(ch, atoi(line));
          else if (!strcmp(tag, "Abso")) char_set_absorbs(ch, atoi(line));
          else if (!strcmp(tag, "AdmF")) {
            sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            char_get_admflags(ch)[0] = asciiflag_conv(f1);
            char_get_admflags(ch)[1] = asciiflag_conv(f2);
            char_get_admflags(ch)[2] = asciiflag_conv(f3);
            char_get_admflags(ch)[3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Alin")) char_set_alignment(ch, atoi(line));
          else if (!strcmp(tag, "Aura")) char_set_aura(ch, atoi(line));
          break;

        case 'B':
          if (!strcmp(tag, "Badp")) char_set_bad_pws(ch, atoi(line));
          else if (!strcmp(tag, "Bank")) char_set_bank_gold(ch, atoi(line));
          else if (!strcmp(tag, "Bki ")) load_BASE(ch, line, LOAD_MANA);
          else if (!strcmp(tag, "Blss")) char_set_blesslvl(ch, atoi(line));
          else if (!strcmp(tag, "Boam")) char_set_lboard(ch, 0, atoi(line));
          else if (!strcmp(tag, "Boai")) char_set_lboard(ch, 1, atoi(line));
          else if (!strcmp(tag, "Boac")) char_set_lboard(ch, 2, atoi(line));
          else if (!strcmp(tag, "Boad")) char_set_lboard(ch, 3, atoi(line));
          else if (!strcmp(tag, "Boab")) char_set_lboard(ch, 4, atoi(line));
          else if (!strcmp(tag, "Bonu")) load_bonuses(fl, ch, FALSE);
          else if (!strcmp(tag, "Boos")) char_set_boosts(ch, atoi(line));
          else if (!strcmp(tag, "Bpl ")) load_BASE(ch, line, LOAD_HIT);
          else if (!strcmp(tag, "Brth")) ch->time.birth = atol(line);
          else if (!strcmp(tag, "Bst ")) load_BASE(ch, line, LOAD_MOVE);
          break;

        case 'C':
          if (!strcmp(tag, "Cha ")) ch->real_abils.cha = atoi(line);
          else if (!strcmp(tag, "Clan"))
            ;
          else if (!strcmp(tag, "Clar"))
            ;
          else if (!strcmp(tag, "Clas")) ch->chclass = atoi(line);
          else if (!strcmp(tag, "Colr")) {
            sscanf(line, "%d %s", &num, buf2);
            ch->player_specials->color_choices[num] = strdup(buf2);
          } else if (!strcmp(tag, "Con ")) ch->real_abils.con = atoi(line);
          else if (!strcmp(tag, "Cool")) char_set_con_cooldown(ch, atoi(line));
          else if (!strcmp(tag, "Crtd")) ch->time.created = atol(line);
          break;

        case 'D':
          if (!strcmp(tag, "Deat")) char_set_deathtime(ch, atoi(line));
          else if (!strcmp(tag, "Deac")) char_set_dcount(ch, atoi(line));
          else if (!strcmp(tag, "Desc")) ch->description = fread_string(fl, buf2);
          else if (!strcmp(tag, "Dex ")) ch->real_abils.dex = atoi(line);
          else if (!strcmp(tag, "Drnk")) char_set_conditions(ch, DRUNK, atoi(line));
          else if (!strcmp(tag, "Damg"))
            ; /* obsolete damage mod: ignored */
          else if (!strcmp(tag, "Droo")) char_set_droom(ch, atoi(line));
          break;

        case 'E':
          if (!strcmp(tag, "Exp ")) char_set_exp(ch, atoi(line));
          else if (!strcmp(tag, "Eali"))
            ;
          else if (!strcmp(tag, "Ecls")) {
            sscanf(line, "%d=%d", &num, &num2);
          } else if (!strcmp(tag, "Eye ")) char_set_eye(ch, atoi(line));
          break;

        case 'F':
          if (!strcmp(tag, "Fisd")) char_set_accuracy_mod(ch, atoi(line));
          else if (!strcmp(tag, "Frez")) char_set_freeze_level(ch, atoi(line));
          else if (!strcmp(tag, "Forc")) char_set_forgetcount(ch, atoi(line));
          else if (!strcmp(tag, "Forg")) char_set_forgeting(ch, atoi(line));
          else if (!strcmp(tag, "Fury")) char_set_fury(ch, atoi(line));
          break;

        case 'G':
          if (!strcmp(tag, "Gold")) char_set_gold(ch, atoi(line));
          else if (!strcmp(tag, "Gaun")) char_set_gauntlet(ch, atoi(line));
          else if (!strcmp(tag, "Geno")) char_set_genome(ch, 0, atoi(line));
          else if (!strcmp(tag, "Gen1")) char_set_genome(ch, 1, atoi(line));
          break;

        case 'H':
          if (!strcmp(tag, "Hit ")) load_HMVS(ch, line, LOAD_HIT);
          else if (!strcmp(tag, "HitD")) char_set_race_level(ch, atoi(line));
          else if (!strcmp(tag, "Hite")) char_set_height(ch, atoi(line));
          else if (!strcmp(tag, "Home")) char_set_hometown(ch, atoi(line));
          else if (!strcmp(tag, "Host")) {
            if (char_get_host(ch))
              free(char_get_host(ch));
            char_set_host(ch, strdup(line));
          } else if (!strcmp(tag, "Hrc ")) char_set_hairc(ch, atoi(line));
          else if (!strcmp(tag, "Hrl ")) char_set_hairl(ch, atoi(line));
          else if (!strcmp(tag, "Hrs ")) char_set_hairs(ch, atoi(line));
          else if (!strcmp(tag, "Hung")) char_set_conditions(ch, HUNGER, atoi(line));
          break;

        case 'I':
          if (!strcmp(tag, "Id  ")) char_set_idnum(ch, atol(line));
          else if (!strcmp(tag, "INGl")) char_set_ingestLearned(ch, atoi(line));
          else if (!strcmp(tag, "Int ")) ch->real_abils.intel = atoi(line);
          else if (!strcmp(tag, "Invs")) char_set_invis_level(ch, atoi(line));
          break;

        case 'K':
          if (!strcmp(tag, "Ki  ")) load_HMVS(ch, line, LOAD_KI);
          else if (!strcmp(tag, "Kaio")) char_set_kaioken(ch, atoi(line));
          break;

        case 'L':
          if (!strcmp(tag, "Last")) ch->time.logon = atol(line);
          else if (!strcmp(tag, "Lern")) char_set_practice_points(ch, atoi(line));
          else if (!strcmp(tag, "Levl")) char_set_class_level(ch, atoi(line));
          /* else if (!strcmp(tag, "LevD"))  read_level_data(ch, fl);*/
          else if (!strcmp(tag, "LF  ")) load_BASE(ch, line, LOAD_LIFE);
          else if (!strcmp(tag, "LFPC")) char_set_lifeperc(ch, atoi(line));
          else if (!strcmp(tag, "Lila")) char_set_limb_condition(ch, (2) - 1, atoi(line));
          else if (!strcmp(tag, "Lill")) char_set_limb_condition(ch, (4) - 1, atoi(line));
          else if (!strcmp(tag, "Lira")) char_set_limb_condition(ch, (1) - 1, atoi(line));
          else if (!strcmp(tag, "Lirl")) char_set_limb_condition(ch, (3) - 1, atoi(line));
          else if (!strcmp(tag, "Lint")) char_set_lastint(ch, atoi(line));
          else if (!strcmp(tag, "Lpla")) char_set_lastpl(ch, atoi(line));
          else if (!strcmp(tag, "LvlA")) char_set_level_adj(ch, atoi(line));
          break;

        case 'M':
          if (!strcmp(tag, "Mana")) load_HMVS(ch, line, LOAD_MANA);
          else if (!strcmp(tag, "Mexp")) load_molt(ch, line);
          else if (!strcmp(tag, "Mlvl")) char_set_moltlevel(ch, atoi(line));
          else if (!strcmp(tag, "Move")) load_HMVS(ch, line, LOAD_MOVE);
          else if (!strcmp(tag, "Mcls")) {
            sscanf(line, "%d=%d", &num, &num2);
          } else if (!strcmp(tag, "Maji")) char_set_majinize(ch, atoi(line));
          else if (!strcmp(tag, "Majm")) load_majin(ch, line);
          else if (!strcmp(tag, "Mimi")) ch->mimic = atoi(line);
          else if (!strcmp(tag, "MxAg")) ch->time.maxage = atol(line);
          break;

        case 'N':
          if (!strcmp(tag, "Name")) char_set_pc_name(ch, strdup(line));
          break;

        case 'O':
          if (!strcmp(tag, "Olc ")) char_set_olc_zone(ch, atoi(line));
          break;

        case 'P':
          if (!strcmp(tag, "Page")) char_set_page_length(ch, atoi(line));
          else if (!strcmp(tag, "Phas")) char_set_distfea(ch, atoi(line));
          else if (!strcmp(tag, "Phse")) char_set_starphase(ch, atoi(line));
          else if (!strcmp(tag, "Plyd")) ch->time.played = atol(line);
#ifdef ASCII_SAVE_POOFS
          else if (!strcmp(tag, "PfIn")) char_set_poofin(ch, strdup(line));
          else if (!strcmp(tag, "PfOt")) char_set_poofout(ch, strdup(line));
#endif
          else if (!strcmp(tag, "Pole")) char_set_accuracy(ch, atoi(line));
          else if (!strcmp(tag, "Posi")) char_set_position(ch, atoi(line));
          else if (!strcmp(tag, "PwrA")) char_set_powerattack(ch, atoi(line));
          else if (!strcmp(tag, "Pref")) {
            sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
            char_get_pref(ch)
                [0] = asciiflag_conv(f1);
            char_get_pref(ch)
                [1] = asciiflag_conv(f2);
            char_get_pref(ch)
                [2] = asciiflag_conv(f3);
            char_get_pref(ch)
                [3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Prff")) char_set_preference(ch, atoi(line));
          break;

        case 'R':
          if (!strcmp(tag, "Race")) ch->race = atoi(line);
          else if (!strcmp(tag, "Raci")) char_set_racial_pref(ch, atoi(line));
          else if (!strcmp(tag, "RBan")) char_set_rbank(ch, atoi(line));
          else if (!strcmp(tag, "rDis")) char_set_rdisplay(ch, strdup(line));
          else if (!strcmp(tag, "Rela")) char_set_relax_count(ch, atoi(line));
          else if (!strcmp(tag, "Rtim")) char_set_rewtime(ch, atoi(line));
          else if (!strcmp(tag, "Rad1")) char_set_radar1(ch, atoi(line));
          else if (!strcmp(tag, "Rad2")) char_set_radar2(ch, atoi(line));
          else if (!strcmp(tag, "Rad3")) char_set_radar3(ch, atoi(line));
          else if (!strcmp(tag, "Room")) char_set_load_room(ch, atoi(line));
          else if (!strcmp(tag, "RPfe")) char_set_feature(ch, strdup(line));
          else if (!strcmp(tag, "RPP ")) char_set_rp(ch, atoi(line));
          break;

        case 'S':
          if (!strcmp(tag, "Sex ")) char_set_sex(ch, atoi(line));
          else if (!strcmp(tag, "Ship"))
            ; /* obsolete ship: ignored */
          else if (!strcmp(tag, "Scoo")) char_set_con_sdcooldown(ch, atoi(line));
          else if (!strcmp(tag, "Shpr"))
            ; /* obsolete ship room: ignored */
          else if (!strcmp(tag, "Skil")) load_skills(fl, ch, FALSE);
          else if (!strcmp(tag, "Skn ")) char_set_skin(ch, atoi(line));
          else if (!strcmp(tag, "Size")) ch->size = atoi(line);
          else if (!strcmp(tag, "SklB")) load_skills(fl, ch, TRUE);
          else if (!strcmp(tag, "SkRc"))
            ; /* obsolete race practices: ignored */
          else if (!strcmp(tag, "SkCl")) {
            sscanf(line, "%d %d", &num2, &num3);
            char_set_practice_points(ch, num3);
          } else if (!strcmp(tag, "Slot")) ch->skill_slots = atoi(line);
          else if (!strcmp(tag, "Spek")) char_set_speaking(ch, atoi(line));
          else if (!strcmp(tag, "Str ")) ch->real_abils.str = atoi(line);
          else if (!strcmp(tag, "Stuk"))
            ;
          else if (!strcmp(tag, "Supp")) char_set_suppression(ch, atoi(line));
          else if (!strcmp(tag, "Sups")) char_set_suppressed(ch, atoi(line));
          break;

        case 'T':
          if (!strcmp(tag, "Tgro")) char_set_tail_growth(ch, atoi(line));
          else if (!strcmp(tag, "Tcla")) char_set_transclass(ch, atoi(line));
          else if (!strcmp(tag, "Tcos")) {
            sscanf(line, "%d %d", &num2, &num3);
            ch->transcost[num2] = num3;
          } else if (!strcmp(tag, "Thir")) char_set_conditions(ch, THIRST, atoi(line));
          else if (!strcmp(tag, "Thr1"))
            ;
          else if (!strcmp(tag, "Thr2"))
            ;
          else if (!strcmp(tag, "Thr3"))
            ;
          else if (!strcmp(tag, "Thr4") || !strcmp(tag, "Thr5"))
            ; /* Discard extra saves */
          else if (!strcmp(tag, "ThB1"))
            ;
          else if (!strcmp(tag, "ThB2"))
            ;
          else if (!strcmp(tag, "ThB3"))
            ;
          else if (!strcmp(tag, "Trns")) char_set_ability_trains(ch, atoi(line));
          else if (!strcmp(tag, "Trag")) char_set_trainagl(ch, atoi(line));
          else if (!strcmp(tag, "Trco")) char_set_traincon(ch, atoi(line));
          else if (!strcmp(tag, "Trin")) char_set_trainint(ch, atoi(line));
          else if (!strcmp(tag, "Trsp")) char_set_trainspd(ch, atoi(line));
          else if (!strcmp(tag, "Trst")) char_set_trainstr(ch, atoi(line));
          else if (!strcmp(tag, "Trwi")) char_set_trainwis(ch, atoi(line));
          else if (!strcmp(tag, "Trp ")) char_set_trp(ch, atoi(line));
          break;
        case 'U':
          if (!strcmp(tag, "Upgr")) char_set_upgrade(ch, atoi(line));
          else if (!strcmp(tag, "User")) {
            if (char_get_loguser(ch)) {
              free(char_get_loguser(ch));
            }
            char_set_loguser(ch, strdup(line));
          }
          break;
        case 'V':
          if (!strcmp(tag, "Voic")) char_set_voice(ch, strdup(line));
          break;

        case 'W':
          if (!strcmp(tag, "Wate")) char_set_weight(ch, atoi(line));
          else if (!strcmp(tag, "Wimp")) char_set_wimp_level(ch, atoi(line));
          else if (!strcmp(tag, "Wis ")) ch->real_abils.wis = atoi(line);
          break;

        default:
          sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
      }
    }
  }

  if (!ch->time.created) {
    mud_log("No creation timestamp for user %s, using current time", char_get_name(ch));
    ch->time.created = time(0);
  }

  if (!ch->time.birth) {
    mud_log("No birthday for user %s, using standard starting age determination", char_get_name(ch));
    ch->time.birth = time(0) - birth_age(ch);
  }

  if (!ch->time.maxage) {
    mud_log("No max age for user %s, using standard max age determination", char_get_name(ch));
    ch->time.maxage = ch->time.birth + max_age(ch);
  }

  affect_total(ch);

  /* initialization for imms */
  if (char_get_admlevel(ch) >= ADMLVL_IMMORT) {
    for (i = 0; i < SKILL_TABLE_SIZE; i++)
      char_set_skills_base(ch, i, 100);
    char_set_conditions(ch, HUNGER, -1);
    char_set_conditions(ch, THIRST, -1);
    char_set_conditions(ch, DRUNK, -1);
  }

  if (char_is_android(ch)) {
    char_set_conditions(ch, HUNGER, -1);
    char_set_conditions(ch, THIRST, -1);
    char_set_conditions(ch, DRUNK, -1);
  }
  fclose(fl);
  return (id);
}

/* remove ^M's from file output */
/* There may be a similar function in Oasis (and I'm sure
   it's part of obuild).  Remove this if you get a
   multiple definition error or if it you want to use a
   substitute
*/
static void kill_ems(char *str)
{
  char *ptr1, *ptr2;

  ptr1 = str;
  ptr2 = str;

  while (*ptr1) {
    if ((*(ptr2++) = *(ptr1++)) == '\r')
      if (*ptr1 == '\r')
        ptr1++;
  }
  *ptr2 = '\0';
}

/*
 * write the vital data of a player to the player file
 *
 * And that's it! No more fudging around with the load room.
 */
/* This is the ASCII Player Files save routine */
void save_char(struct char_data *ch)
{
  FILE *fl;
  char fname[40], buf[MAX_STRING_LENGTH];
  int i, id, save_index = FALSE;
  struct affected_type *aff, tmp_aff[MAX_AFFECT], tmp_affv[MAX_AFFECT];
  struct obj_data *char_eq[NUM_WEARS];
  char fbuf1[MAX_STRING_LENGTH], fbuf2[MAX_STRING_LENGTH];
  char fbuf3[MAX_STRING_LENGTH], fbuf4[MAX_STRING_LENGTH];

  if (char_is_npc(ch) || char_get_pfilepos(ch) < 0)
    return;

  /*
   * If ch->desc is not null, then we need to update some session data
   * before saving.
   */
  if (ch->desc) {
    if (*ch->desc->host) {
      if (!char_get_host(ch))
        char_set_host(ch, strdup(ch->desc->host));
      else if (char_get_host(ch) && !strcmp(char_get_host(ch), ch->desc->host)) {
        free(char_get_host(ch));
        char_set_host(ch, strdup(ch->desc->host));
      }
    }

    /*
     * We only update the time.played and time.logon if the character
     * is playing.
     */
    if (desc_get_connected(ch->desc) == CON_PLAYING) {
      ch->time.played += time(0) - ch->time.logon;
      ch->time.logon = time(0);
    }
  }

  if (!get_filename(fname, sizeof(fname), PLR_FILE, char_get_name(ch)))
    return;
  if (!(fl = fopen(fname, "w"))) {
    mudlog(LOG_NRM, ADMLVL_GOD, TRUE, "SYSERR: Couldn't open player file %s for write", fname);
    return;
  }

  /* remove affects from eq and spells (from char_to_store) */
  /* Unaffect everything a character can be affected by */

  if (char_get_trp(ch) < char_get_rp(ch)) {
    char_set_trp(ch, char_get_rp(ch));
  }
  if (ch->desc != NULL && ch->desc->user != NULL) {
    userWrite(ch->desc, 0, 0, 0, "index");
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i))
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = NULL;
  }

  for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (aff) {
      tmp_aff[i]      = *aff;
      tmp_aff[i].next = 0;
      aff             = aff->next;
    } else {
      tmp_aff[i].type      = 0; /* Zero signifies not used */
      tmp_aff[i].duration  = 0;
      tmp_aff[i].modifier  = 0;
      tmp_aff[i].specific  = 0;
      tmp_aff[i].location  = 0;
      tmp_aff[i].bitvector = 0;
      tmp_aff[i].next      = 0;
    }
  }

  save_char_vars(ch);

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  if ((i >= MAX_AFFECT) && aff && aff->next)
    mud_log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  /* end char_to_store code */

  if (char_get_name(ch)) fprintf(fl, "Name: %s\n", char_get_name(ch));
  if (char_get_user(ch)) fprintf(fl, "User: %s\n", char_get_user(ch));
  if (char_get_voice(ch)) fprintf(fl, "Voic: %s\n", char_get_voice(ch));
  if (char_get_feature(ch)) fprintf(fl, "RPfe: %s\n", char_get_feature(ch));
  if (char_get_ability_trains(ch)) fprintf(fl, "Trns: %d\n", char_get_ability_trains(ch));
  if (ch->description && *ch->description) {
    strcpy(buf, ch->description);
    kill_ems(buf);
    fprintf(fl, "Desc:\n%s~\n", buf);
  }
#ifdef ASCII_SAVE_POOFS
  if (char_get_poofin(ch)) fprintf(fl, "PfIn: %s\n", char_get_poofin(ch));
  if (char_get_poofout(ch)) fprintf(fl, "PfOt: %s\n", char_get_poofout(ch));
#endif
  if (char_get_sex(ch) != PFDEF_SEX) fprintf(fl, "Sex : %d\n", char_get_sex(ch));
  if (ch->size != PFDEF_SIZE) fprintf(fl, "Size: %d\n", ch->size);
  if (char_get_class(ch) != PFDEF_CLASS) fprintf(fl, "Clas: %d\n", char_get_class(ch));
  if (char_get_race(ch) != PFDEF_RACE) fprintf(fl, "Race: %d\n", char_get_race(ch));
  if (char_get_racial_pref(ch) != PFDEF_BANK) fprintf(fl, "Raci: %d\n", char_get_racial_pref(ch));
  if (char_get_admlevel(ch) != PFDEF_LEVEL) fprintf(fl, "AdmL: %d\n", char_get_admlevel(ch));
  if (char_get_class_level(ch) != PFDEF_LEVEL) fprintf(fl, "Levl: %d\n", char_get_class_level(ch));
  if (char_get_race_level(ch) != PFDEF_LEVEL) fprintf(fl, "HitD: %d\n", char_get_race_level(ch));
  if (char_get_level_adj(ch) != PFDEF_LEVEL) fprintf(fl, "LvlA: %d\n", char_get_level_adj(ch));
  if (char_get_hometown(ch) != PFDEF_HOMETOWN) fprintf(fl, "Home: %d\n", char_get_hometown(ch));
  fprintf(fl, "Id  : %d\n", char_get_idnum(ch));
  fprintf(fl, "Brth: %ld\n", ch->time.birth);
  fprintf(fl, "Crtd: %ld\n", ch->time.created);
  fprintf(fl, "MxAg: %ld\n", ch->time.maxage);
  fprintf(fl, "Plyd: %ld\n", ch->time.played);
  fprintf(fl, "Last: %ld\n", ch->time.logon);

  if (char_get_host(ch)) fprintf(fl, "Host: %s\n", char_get_host(ch));
  if (char_get_height(ch) != PFDEF_HEIGHT) fprintf(fl, "Hite: %d\n", char_get_height(ch));
  if (char_get_weight(ch) != PFDEF_HEIGHT) fprintf(fl, "Wate: %d\n", char_get_weight(ch));
  if (char_get_alignment(ch) != PFDEF_ALIGNMENT) fprintf(fl, "Alin: %d\n", char_get_alignment(ch));
  if (char_get_aura(ch) != PFDEF_SKIN) fprintf(fl, "Aura: %d\n", char_get_aura(ch));

  sprintascii(fbuf1, char_get_act(ch)[0]);
  sprintascii(fbuf2, char_get_act(ch)[1]);
  sprintascii(fbuf3, char_get_act(ch)[2]);
  sprintascii(fbuf4, char_get_act(ch)[3]);
  fprintf(fl, "Act : %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, char_get_affected_by(ch)[0]);
  sprintascii(fbuf2, char_get_affected_by(ch)[1]);
  sprintascii(fbuf3, char_get_affected_by(ch)[2]);
  sprintascii(fbuf4, char_get_affected_by(ch)[3]);
  fprintf(fl, "Aff : %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, char_get_pref(ch)[0]);
  sprintascii(fbuf2, char_get_pref(ch)[1]);
  sprintascii(fbuf3, char_get_pref(ch)[2]);
  sprintascii(fbuf4, char_get_pref(ch)[3]);
  fprintf(fl, "Pref: %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, char_get_admflags(ch)[0]);
  sprintascii(fbuf2, char_get_admflags(ch)[1]);
  sprintascii(fbuf3, char_get_admflags(ch)[2]);
  sprintascii(fbuf4, char_get_admflags(ch)[3]);
  fprintf(fl, "AdmF: %s %s %s %s\n", fbuf1, fbuf2, fbuf3, fbuf4);

  if (char_get_wimp_level(ch) != PFDEF_WIMPLEV) fprintf(fl, "Wimp: %d\n", char_get_wimp_level(ch));
  if (char_get_powerattack(ch) != PFDEF_POWERATT) fprintf(fl, "PwrA: %d\n", char_get_powerattack(ch));
  if (char_get_freeze_level(ch) != PFDEF_FREEZELEV) fprintf(fl, "Frez: %d\n", char_get_freeze_level(ch));
  if (char_get_invis_level(ch) != PFDEF_INVISLEV) fprintf(fl, "Invs: %d\n", char_get_invis_level(ch));
  if (char_get_load_room(ch) != PFDEF_LOADROOM) fprintf(fl, "Room: %d\n", char_get_load_room(ch));

  if (char_get_bad_pws(ch) != PFDEF_BADPWS) fprintf(fl, "Badp: %d\n", char_get_bad_pws(ch));

  for (i = 0; i < 6; i++)
    if (ch->transcost[i] != FALSE)
      fprintf(fl, "Tcos: %d %d\n", i, ch->transcost[i]);

  for (i = 0; i < NUM_CLASSES; i++)
    if (char_get_practice_points(ch) != PFDEF_PRACTICES)
      fprintf(fl, "SkCl: %d %d\n", i, char_get_practice_points(ch));

  if (char_get_conditions(ch, HUNGER) != PFDEF_HUNGER && char_get_admlevel(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Hung: %d\n", char_get_conditions(ch, HUNGER));
  if (char_get_conditions(ch, THIRST) != PFDEF_THIRST && char_get_admlevel(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Thir: %d\n", char_get_conditions(ch, THIRST));
  if (char_get_conditions(ch, DRUNK) != PFDEF_DRUNK && char_get_admlevel(ch) < ADMLVL_IMMORT)
    fprintf(fl, "Drnk: %d\n", char_get_conditions(ch, DRUNK));

  /*
  if (GET_HIT(ch)	   != PFDEF_HIT  || GET_MAX_HIT(ch)  != PFDEF_MAXHIT)
    fprintf(fl, "Hit : %" I64T "/%" I64T "\n", GET_HIT(ch),  GET_MAX_HIT(ch));
  if ((getCurKI(ch))	   != PFDEF_MANA || GET_MAX_MANA(ch) != PFDEF_MAXMANA)
    fprintf(fl, "Mana: %" I64T "/%" I64T "\n", (getCurKI(ch)), GET_MAX_MANA(ch));
  if ((getCurST(ch))	   != PFDEF_MOVE || GET_MAX_MOVE(ch) != PFDEF_MAXMOVE)
    fprintf(fl, "Move: %" I64T "/%" I64T "\n", (getCurST(ch)), GET_MAX_MOVE(ch));
  if (GET_KI(ch)	   != PFDEF_KI || GET_MAX_KI(ch) != PFDEF_MAXKI)
    fprintf(fl, "Ki  : %" I64T "/%" I64T "\n", GET_KI(ch), GET_MAX_KI(ch));
  */
  if (char_get_aff_abils_str(ch) != PFDEF_STR) fprintf(fl, "Str : %d\n", char_get_aff_abils_str(ch));
  if (char_get_aff_abils_intel(ch) != PFDEF_INT) fprintf(fl, "Int : %d\n", char_get_aff_abils_intel(ch));
  if (char_get_aff_abils_wis(ch) != PFDEF_WIS) fprintf(fl, "Wis : %d\n", char_get_aff_abils_wis(ch));
  if (char_get_aff_abils_dex(ch) != PFDEF_DEX) fprintf(fl, "Dex : %d\n", char_get_aff_abils_dex(ch));
  if (char_get_aff_abils_con(ch) != PFDEF_CON) fprintf(fl, "Con : %d\n", char_get_aff_abils_con(ch));
  if (char_get_aff_abils_cha(ch) != PFDEF_CHA) fprintf(fl, "Cha : %d\n", char_get_aff_abils_cha(ch));

  if (char_get_con_cooldown(ch) != PFDEF_BANK) fprintf(fl, "Cool: %d\n", char_get_con_cooldown(ch));
  if (char_get_con_cooldown(ch) != PFDEF_BANK) fprintf(fl, "Scoo: %d\n", char_get_con_sdcooldown(ch));
  if (char_get_armor(ch) != PFDEF_AC) fprintf(fl, "Ac  : %d\n", char_get_armor(ch));
  if (char_get_absorbs(ch) != PFDEF_GOLD) fprintf(fl, "Abso: %d\n", char_get_absorbs(ch));
  if (char_get_ingestLearned(ch) != PFDEF_GOLD) fprintf(fl, "INGl: %d\n", char_get_ingestLearned(ch));
  if (char_get_upgrade(ch) != PFDEF_GOLD) fprintf(fl, "Upgr: %d\n", char_get_upgrade(ch));
  if (char_get_forgeting(ch) != PFDEF_BANK) fprintf(fl, "Forg: %d\n", char_get_forgeting(ch));
  if (char_get_forgetcount(ch) != PFDEF_BANK) fprintf(fl, "Forc: %d\n", char_get_forgetcount(ch));
  if (char_get_kaioken(ch) != PFDEF_GOLD) fprintf(fl, "Kaio: %d\n", char_get_kaioken(ch));
  if (char_get_gold(ch) != PFDEF_GOLD) fprintf(fl, "Gold: %d\n", char_get_gold(ch));
  if (char_get_bank_gold(ch) != PFDEF_BANK) fprintf(fl, "Bank: %d\n", char_get_bank_gold(ch));
  if (char_get_exp(ch) != PFDEF_EXP) fprintf(fl, "Exp : %" I64T "\n", char_get_exp(ch));
  if (char_get_transclass(ch) != PFDEF_EXP) fprintf(fl, "Tcla: %d\n", char_get_transclass(ch));
  if (char_get_moltexp(ch) != PFDEF_EXP) fprintf(fl, "Mexp: %" I64T "\n", char_get_moltexp(ch));
  if (char_get_majinizer(ch) != PFDEF_EXP) fprintf(fl, "Majm: %" I64T "\n", char_get_majinizer(ch));
  if (char_get_moltlevel(ch) != PFDEF_EXP) fprintf(fl, "Mlvl: %d\n", char_get_moltlevel(ch));
  if (char_get_accuracy_mod(ch) != PFDEF_ACCURACY) fprintf(fl, "Fisd: %d\n", char_get_accuracy_mod(ch));
  if (char_get_accuracy(ch) != PFDEF_ACCURACY) fprintf(fl, "Pole: %d\n", char_get_accuracy(ch));
  if (char_get_preference(ch) != PFDEF_EYE) fprintf(fl, "Prff: %d\n", char_get_preference(ch));
  if (char_get_speaking(ch) != PFDEF_SPEAKING) fprintf(fl, "Spek: %d\n", char_get_speaking(ch));
  if (char_get_olc_zone(ch) != PFDEF_OLC) fprintf(fl, "Olc : %d\n", char_get_olc_zone(ch));
  if (char_get_page_length(ch) != PFDEF_PAGELENGTH) fprintf(fl, "Page: %d\n", char_get_page_length(ch));
  if (char_get_gauntlet(ch) != PFDEF_GAUNTLET) fprintf(fl, "Gaun: %d\n", char_get_gauntlet(ch));
  if (char_get_genome(ch, 0) != PFDEF_EYE) fprintf(fl, "Geno: %d\n", char_get_genome(ch, 0));
  if (char_get_genome(ch, 1) != PFDEF_EYE) fprintf(fl, "Gen1: %d\n", char_get_genome(ch, 1));
  if (char_get_position(ch) != POS_STANDING) fprintf(fl, "Posi: %d\n", char_get_position(ch));
  //if ((getCurLF(ch))    != PFDEF_BASEPL)     fprintf(fl, "LF  : %" I64T "\n", (getCurLF(ch)));
  if (char_get_lifeperc(ch) != PFDEF_WEIGHT) fprintf(fl, "LFPC: %d\n", char_get_lifeperc(ch));
  if (getBasePL(ch) != PFDEF_BASEPL) fprintf(fl, "Bpl : %" I64T "\n", getBasePL(ch));
  if (getBaseKI(ch) != PFDEF_BASEKI) fprintf(fl, "Bki : %" I64T "\n", getBaseKI(ch));
  if (getBaseST(ch) != PFDEF_BASEST) fprintf(fl, "Bst : %" I64T "\n", getBaseST(ch));
  if (char_get_droom(ch) != PFDEF_DROOM) fprintf(fl, "Droo: %d\n", char_get_droom(ch));
  if (char_get_hairl(ch) != PFDEF_HAIRL) fprintf(fl, "Hrl : %d\n", char_get_hairl(ch));
  if (char_get_hairs(ch) != PFDEF_HAIRS) fprintf(fl, "Hrs : %d\n", char_get_hairs(ch));
  if (char_get_hairc(ch) != PFDEF_HAIRC) fprintf(fl, "Hrc : %d\n", char_get_hairc(ch));
  if (char_get_skin(ch) != PFDEF_SKIN) fprintf(fl, "Skn : %d\n", char_get_skin(ch));
  if (char_get_eye(ch) != PFDEF_EYE) fprintf(fl, "Eye : %d\n", char_get_eye(ch));
  if (char_get_distfea(ch) != PFDEF_DISTFEA) fprintf(fl, "Phas: %d\n", char_get_distfea(ch));
  if (char_get_fury(ch) != PFDEF_HAIRL) fprintf(fl, "Fury: %d\n", char_get_fury(ch));
  if (char_get_radar1(ch) != PFDEF_RADAR1) fprintf(fl, "Rad1: %d\n", char_get_radar1(ch));
  if (char_get_radar2(ch) != PFDEF_RADAR2) fprintf(fl, "Rad2: %d\n", char_get_radar2(ch));
  if (char_get_radar3(ch) != PFDEF_RADAR3) fprintf(fl, "Rad3: %d\n", char_get_radar3(ch));
  if (char_get_lastpl(ch) != PFDEF_LPLAY) fprintf(fl, "Lpla: %ld\n", char_get_lastpl(ch));
  if (char_get_lastint(ch) != PFDEF_LPLAY) fprintf(fl, "Lint: %ld\n", char_get_lastint(ch));
  if (char_get_deathtime(ch) != PFDEF_LPLAY) fprintf(fl, "Deat: %ld\n", char_get_deathtime(ch));
  if (char_get_rewtime(ch) != PFDEF_LPLAY) fprintf(fl, "Rtim: %ld\n", char_get_rewtime(ch));
  if (char_get_boosts(ch) != PFDEF_DISTFEA) fprintf(fl, "Boos: %d\n", char_get_boosts(ch));
  if (char_get_majinize(ch) != PFDEF_LPLAY) fprintf(fl, "Maji: %d\n", char_get_majinize(ch));
  if (char_get_blesslvl(ch) != PFDEF_HEIGHT) fprintf(fl, "Blss: %d\n", char_get_blesslvl(ch));
  if (char_get_lboard(ch, 0) != PFDEF_BOARD) fprintf(fl, "Boam: %ld\n", char_get_lboard(ch, 0));
  if (char_get_lboard(ch, 1) != PFDEF_BOARD) fprintf(fl, "Boai: %ld\n", char_get_lboard(ch, 1));
  if (char_get_lboard(ch, 2) != PFDEF_BOARD) fprintf(fl, "Boac: %ld\n", char_get_lboard(ch, 2));
  if (char_get_lboard(ch, 3) != PFDEF_BOARD) fprintf(fl, "Boad: %ld\n", char_get_lboard(ch, 3));
  if (char_get_lboard(ch, 4) != PFDEF_BOARD) fprintf(fl, "Boab: %ld\n", char_get_lboard(ch, 4));
  if (char_get_limb_condition(ch, (1) - 1) != PFDEF_BOARD) fprintf(fl, "Lira: %d\n", char_get_limb_condition(ch, (1) - 1));
  if (char_get_limb_condition(ch, (2) - 1) != PFDEF_BOARD) fprintf(fl, "Lila: %d\n", char_get_limb_condition(ch, (2) - 1));
  if (char_get_limb_condition(ch, (3) - 1) != PFDEF_BOARD) fprintf(fl, "Lirl: %d\n", char_get_limb_condition(ch, (3) - 1));
  if (char_get_limb_condition(ch, (4) - 1) != PFDEF_BOARD) fprintf(fl, "Lill: %d\n", char_get_limb_condition(ch, (4) - 1));
  if (char_get_rp(ch) != PFDEF_SKIN) fprintf(fl, "RPP : %d\n", char_get_rp(ch));
  if (char_get_rbank(ch) != PFDEF_SKIN) fprintf(fl, "RBan: %d\n", char_get_rbank(ch));
  if (char_get_suppression(ch) != PFDEF_SKIN) fprintf(fl, "Supp: %" I64T "\n", char_get_suppression(ch));
  if (char_get_suppressed(ch) != PFDEF_SKIN) fprintf(fl, "Sups: %" I64T "\n", char_get_suppressed(ch));
  if (char_get_trp(ch) != PFDEF_SKIN) fprintf(fl, "Trp : %d\n", char_get_trp(ch));
  if (char_get_dcount(ch) != PFDEF_EYE) fprintf(fl, "Deac: %d\n", char_get_dcount(ch));
  if (char_get_trainagl(ch) != PFDEF_EYE) fprintf(fl, "Trag: %d\n", char_get_trainagl(ch));
  if (char_get_traincon(ch) != PFDEF_EYE) fprintf(fl, "Trco: %d\n", char_get_traincon(ch));
  if (char_get_trainint(ch) != PFDEF_EYE) fprintf(fl, "Trin: %d\n", char_get_trainint(ch));
  if (char_get_trainspd(ch) != PFDEF_EYE) fprintf(fl, "Trsp: %d\n", char_get_trainspd(ch));
  if (char_get_trainstr(ch) != PFDEF_EYE) fprintf(fl, "Trst: %d\n", char_get_trainstr(ch));
  if (char_get_trainwis(ch) != PFDEF_EYE) fprintf(fl, "Trwi: %d\n", char_get_trainwis(ch));
  if (char_get_starphase(ch) != PFDEF_EYE) fprintf(fl, "Phse: %d\n", char_get_starphase(ch));
  if (char_get_mimic(ch)) fprintf(fl, "Mimi: %d\n", char_get_mimic(ch));
  if (char_get_skill_slots(ch) != PFDEF_EYE) fprintf(fl, "Slot: %d\n", char_get_skill_slots(ch));
  if (char_get_tail_growth(ch) != PFDEF_EYE) fprintf(fl, "Tgro: %d\n", char_get_tail_growth(ch));
  if (char_get_rdisplay(ch) != PFDEF_EYE) fprintf(fl, "rDis: %s\n", char_get_rdisplay(ch));
  if (char_get_relax_count(ch) != PFDEF_EYE) fprintf(fl, "Rela: %d\n", char_get_relax_count(ch));

  /* Save skills */
  if (char_get_admlevel(ch) < ADMLVL_IMMORT) {
    fprintf(fl, "Skil:\n");
    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
      if (char_get_skills_base(ch, i))
        fprintf(fl, "%d %d %d\n", i, char_get_skills_base(ch, i), char_get_skills_perf(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

  /* Save Bonuses/Negatives */
  char buff[200];
  fprintf(fl, "Bonu:\n");
  for (i = 0; i < 52; i++) {
    if (char_get_bonuses(ch, i) && i == 0)
      sprintf(buff, "%d", char_get_bonuses(ch, i));
    else if (char_get_bonuses(ch, i) && i != 0)
      sprintf(buff + strlen(buff), " %d", char_get_bonuses(ch, i));
    else if (i == 0)
      sprintf(buff, "0");
    else
      sprintf(buff + strlen(buff), " 0");
  }
  fprintf(fl, "%s\n", buff);
  *buff = '\0';

  /* Save skill bonuses */
  if (char_get_admlevel(ch) < ADMLVL_IMMORT) {
    fprintf(fl, "SklB:\n");
    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
      if (char_get_skills_mod(ch, i))
        fprintf(fl, "%d %d %d\n", i, char_get_skills_mod(ch, i), char_get_skills_perf(ch, i));
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

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);
  }

  /* end char_to_store code */

  if ((id = get_ptable_by_name(char_get_name(ch))) < 0)
    return;

  /* update the player in the player index */
  if (player_table[id].level != char_get_level(ch)) {
    save_index             = TRUE;
    player_table[id].level = char_get_level(ch);
  }
  if (player_table[id].admlevel != char_get_admlevel(ch)) {
    save_index                = TRUE;
    player_table[id].admlevel = char_get_admlevel(ch);
  }
  if (player_table[id].last != ch->time.logon) {
    save_index            = TRUE;
    player_table[id].last = ch->time.logon;
  }
  if (player_table[id].played != char_get_lastpl(ch)) {
    save_index              = TRUE;
    player_table[id].played = char_get_lastpl(ch);
  }
  player_table[id].clan = strdup("None.");
  i                     = player_table[id].flags;
  if (char_plr_flagged(ch, PLR_DELETED))
    SET_BIT(player_table[id].flags, PINDEX_DELETED);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_DELETED);
  if (char_plr_flagged(ch, PLR_NODELETE) || char_plr_flagged(ch, PLR_CRYO))
    SET_BIT(player_table[id].flags, PINDEX_NODELETE);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NODELETE);

  if (char_plr_flagged(ch, PLR_FROZEN) || char_plr_flagged(ch, PLR_NOWIZLIST))
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

/* Separate a 4-character id tag from the data it precedes */
static void tag_argument(char *argument, char *tag)
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

static void load_affects(FILE *fl, struct char_data *ch, int violence)
{
  int num, num2, num3, num4, num5, num6;
  char line[MAX_INPUT_LENGTH + 1];
  struct affected_type af;

  do {
    get_line(fl, line);
    num = num2 = num3 = num4 = num5 = num6 = 0;
    sscanf(line, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
    if (num != 0) {
      af.type      = num;
      af.duration  = num2;
      af.modifier  = num3;
      af.location  = num4;
      af.bitvector = num5;
      af.specific  = num6;
      if (!violence)
        affect_to_char(ch, &af);
    }
  } while (num != 0);
}

static void load_skills(FILE *fl, struct char_data *ch, bool mods)
{
  int num = 0, num2 = 0, num3 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    sscanf(line, "%d %d %d", &num, &num2, &num3);
    if (num > -1 && num < SKILL_TABLE_SIZE) {
      if (mods) {
        char_set_skills_mod(ch, num, num2);
      } else {
        char_set_skills_base(ch, num, num2);
      }
      char_set_skills_perf(ch, num, num3);
    }
  } while (num != 0);
}

static void load_bonuses(FILE *fl, struct char_data *ch, bool mods)
{
  int num[52] = {0}, i;
  char line[MAX_INPUT_LENGTH + 1];

  get_line(fl, line);
  sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &num[0], &num[1], &num[2], &num[3], &num[4], &num[5], &num[6], &num[7], &num[8], &num[9], &num[10], &num[11], &num[12], &num[13], &num[14], &num[15], &num[16], &num[17], &num[18], &num[19], &num[20], &num[21], &num[22], &num[23], &num[24], &num[25], &num[26], &num[27], &num[28], &num[29], &num[30], &num[31], &num[32], &num[33], &num[34], &num[35], &num[36], &num[37], &num[38], &num[39], &num[40], &num[41], &num[42], &num[43], &num[44], &num[45], &num[46], &num[47], &num[48], &num[49], &num[50], &num[51]);
  for (i = 0; i < 52; i++) {
    if (num[i] > 0) {
      char_set_bonuses(ch, i, num[i]);
    }
  }
}

static void load_HMVS(struct char_data *ch, const char *line, int mode)
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

static void load_BASE(struct char_data *ch, const char *line, int mode)
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

static void load_majin(struct char_data *ch, const char *line)
{
  int64_t num = 0;

  sscanf(line, "%" I64T "", &num);
  char_set_majinizer(ch, num);
}

static void load_molt(struct char_data *ch, const char *line)
{
  int64_t num = 0;

  sscanf(line, "%" I64T "", &num);
  char_set_moltexp(ch, num);
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

  mud_log("PCLEAN: %s Lev: %d Last: %s",
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

int top_of_p_table;
struct player_index_element *player_table;
