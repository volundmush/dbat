/* ************************************************************************
*   File: players.c                                     Part of CircleMUD *
*  Usage: Player loading/saving and utility routines                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <stdckdint.h>
#include <unistd.h>

#include "dbat/arrays.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/handler.hpp"
#include "dbat/pfdefaults.hpp"
#include "dbat/feats.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/comm.hpp"
#include "dbat/genmob.hpp"
#include "dbat/class.hpp"
#include "dbat/config.hpp"
#include "dbat/fileop.hpp"
#include "dbat/affect.hpp"
#include "dbat/genolc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

constexpr int LOAD_HIT  = 0;
constexpr int LOAD_MANA = 1;
constexpr int LOAD_MOVE = 2;
constexpr int LOAD_KI   = 3;
constexpr int LOAD_LIFE = 4;

/* local functions */
static void tag_argument(char *argument, char *tag);
static void load_affects(GameWorld *g, FILE *fl, struct char_data *ch);
static void skip_affects(FILE *fl);
static void load_bonuses(FILE *fl, struct char_data *ch, bool mods);
static void load_skills(FILE *fl, struct char_data *ch, bool mods);
static void load_HMVS(struct char_data *ch, const char *line, int mode);
static void load_BASE(struct char_data *ch, const char *line, int mode);
static void load_molt(struct char_data *ch, const char *line);
static void load_majin(struct char_data *ch, const char *line);

/* 'global' vars */

/* external ASCII Player Files vars */

/* ASCII Player Files - set this TRUE if you want poofin/poofout
   strings saved in the pfiles
 */
#define ASCII_SAVE_POOFS false

/* new version to build player index for ASCII Player Files */
/* generate index table for the player file */
void build_player_index(GameWorld *g)
{
  int rec_count = 0, i;
  FILE *plr_index;
  std::string index_name;
  char line[256], bits[64];
  char arg2[80];

  index_name = str_format("{}{}", LIB_PLRFILES, INDEX_FILE);
  if (!(plr_index = fopen(index_name.c_str(), "r"))) {
    g->top_of_p_table = -1;
    mud_log("No player index file!  First new char will be IMP!");
    return;
  }

  while (get_line(plr_index, line))
    if (*line != '~')
      rec_count++;
  rewind(plr_index);

  if (rec_count == 0) {
    g->player_table   = nullptr;
    g->top_of_p_table = -1;
    return;
  }

  g->player_table = new player_index_element[rec_count]();
  for (i = 0; i < rec_count; i++) {
    get_line(plr_index, line);
    g->player_table[i].admlevel = ADMLVL_NONE; /* In case they're not in the index yet */
    long long last = 0, played = 0;

    sscanf(line, "%ld %79s %d %63s %lld %d %d %d %lld", &g->player_table[i].id, arg2,
           &g->player_table[i].level, bits, &last, &g->player_table[i].admlevel, &g->player_table[i].ship, &g->player_table[i].shiproom, &played);
    g->player_table[i].last   = last;
    g->player_table[i].played = played;
    g->player_table[i].name   = strdup(arg2);
    g->player_table[i].flags  = asciiflag_conv(bits);
    g->top_idnum              = MAX(g->top_idnum, g->player_table[i].id);
  }

  fclose(plr_index);
  g->top_of_p_file = g->top_of_p_table = i - 1;
}

/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(GameWorld *g, const char *name)
{
  int i, pos;

  if (g->top_of_p_table == -1) { /* no table */
    g->player_table = new player_index_element[1]();
    pos = g->top_of_p_table = 0;
  } else if ((pos = get_ptable_by_name(g, name)) == -1) { /* new name */
    i = ++g->top_of_p_table + 1;

    resize_array(g->player_table, i - 1, i);
    pos = g->top_of_p_table;
  } else
    free(g->player_table[pos].name);

  /* a reused slot still holds the old character's values */
  g->player_table[pos] = player_index_element{};
  g->player_table[pos].name = str_alloc(strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (g->player_table[pos].name[i] = tolower(name[i])); i++)
    /* Nothing */;

  return (pos);
}

/* This function necessary to save a seperate ASCII player index */
void save_player_index(GameWorld *g)
{
  struct atomic_file af;
  int i;
  std::string index_name;
  char bits[64];
  FILE *index_file;

  index_name = str_format("{}{}", LIB_PLRFILES, INDEX_FILE);
  if (!(index_file = atomic_open(&af, index_name.c_str(), true))) {
    mud_log("SYSERR: Could not write player index file");
    return;
  }

  for (i = 0; i <= g->top_of_p_table; i++)
    if (*g->player_table[i].name) {
      sprintascii(bits, g->player_table[i].flags);
      file_print(index_file, "{:d} {} {:d} {} {:d} {:d} {:d} {:d} {:d}\n", g->player_table[i].id,
                 g->player_table[i].name, g->player_table[i].level, *bits ? bits : "0",
                 (long long)g->player_table[i].last, g->player_table[i].admlevel, g->player_table[i].ship, g->player_table[i].shiproom, (long long)g->player_table[i].played);
    }
  file_print(index_file, "~\n");

  atomic_commit(&af);
}

void free_player_index(GameWorld *g)
{
  int tp;

  if (!g->player_table)
    return;

  for (tp = 0; tp <= g->top_of_p_table; tp++) {
    if (g->player_table[tp].name)
      free(g->player_table[tp].name);
  }

  delete[] g->player_table;
  g->player_table   = nullptr;
  g->top_of_p_table = 0;
}

long get_ptable_by_name(GameWorld *g, const char *name)
{
  int i;

  for (i = 0; i <= g->top_of_p_table; i++)
    if (!strcasecmp(g->player_table[i].name, name))
      return (i);

  return (-1);
}

long get_id_by_name(GameWorld *g, const char *name)
{
  int i;

  for (i = 0; i <= g->top_of_p_table; i++)
    if (!strcasecmp(g->player_table[i].name, name))
      return (g->player_table[i].id);

  return (-1);
}

char *get_name_by_id(GameWorld *g, long id)
{
  int i;

  for (i = 0; i <= g->top_of_p_table; i++)
    if (g->player_table[i].id == id)
      return (g->player_table[i].name);

  return (nullptr);
}

/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/

/* new load_char reads ASCII Player Files */
/* Load a char, TRUE if loaded, FALSE if not */
int load_char(GameWorld *g, const char *name, struct char_data *ch)
{
  int id, i, num = 0, num2 = 0, num3 = 0;
  FILE *fl;
  char fname[READ_SIZE];
  std::string buf;
  char buf2[128], line[MAX_INPUT_LENGTH + 1], tag[6];
  char f1[128], f2[128], f3[128], f4[128];

  if ((id = get_ptable_by_name(g, name)) < 0)
    return (-1);
  else {
    if (!get_filename(fname, sizeof(fname), PLR_FILE, g->player_table[id].name))
      return (-1);
    if (!(fl = fopen(fname, "r"))) {
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: Couldn't open player file {}", fname);
      return (-1);
    }

    /* character initializations */
    /* initializations necessary to keep some things straight */
    char_affects(ch).clear();

    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
      char_set_skills_base(ch, i, 0);
      char_set_skills_mod(ch, i, 0);
      char_set_skills_perf(ch, i, 0);
    }
    char_set_sex(ch, PFDEF_SEX);
    char_set_size(ch, PFDEF_SIZE);
    char_set_class(ch, CLASS_ROSHI);
    char_set_loguser(ch, "NOUSER");
    char_set_race(ch, PFDEF_RACE);
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
    char_set_basepl(ch, PFDEF_BASEPL);
    char_set_health(ch, 1.0);
    char_set_relax_count(ch, PFDEF_EYE);
    char_set_blesslvl(ch, PFDEF_HEIGHT);
    char_set_life(ch, 1.0);
    // GET_LIFEFORCE(ch) = PFDEF_BASEPL;
    char_set_lifeperc(ch, PFDEF_WEIGHT);
    char_set_position(ch, POS_STANDING);
    char_set_majinizer(ch, PFDEF_BASEPL);
    char_set_gauntlet(g, ch, PFDEF_GAUNTLET);
    char_set_baseki(ch, PFDEF_BASEKI);
    char_set_energy(ch, 1.0);
    char_set_basest(ch, PFDEF_BASEST);
    char_set_stamina(ch, 1.0);
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
    char_set_mimic(ch, 0);
    char_set_skill_slots(ch, 0);
    char_set_tail_growth(ch, 0);
    char_set_trainstr(g, ch, PFDEF_EYE);
    char_set_trainspd(g, ch, PFDEF_EYE);
    char_set_trainwis(g, ch, PFDEF_EYE);
    char_set_trainagl(g, ch, PFDEF_EYE);
    char_set_traincon(g, ch, PFDEF_EYE);
    char_set_trainint(g, ch, PFDEF_EYE);
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
      char_get_pref(g, ch)
          [i] = PFDEF_PREFFLAGS;
    for (i = 0; i < AD_ARRAY_MAX; i++)
      char_get_admflags(ch)[i] = 0;
    char_set_load_room(g, ch, PFDEF_LOADROOM);
    char_set_invis_level(g, ch, PFDEF_INVISLEV);
    char_set_freeze_level(g, ch, PFDEF_FREEZELEV);
    char_set_wimp_level(g, ch, PFDEF_WIMPLEV);
    char_set_powerattack(ch, PFDEF_POWERATT);
    char_set_conditions(g, ch, HUNGER, PFDEF_HUNGER);
    char_set_conditions(g, ch, THIRST, PFDEF_THIRST);
    char_set_conditions(g, ch, DRUNK, PFDEF_DRUNK);
    char_set_bad_pws(g, ch, PFDEF_BADPWS);
    for (i = 0; i < NUM_CLASSES; i++)
      char_set_practice_points(g, ch, PFDEF_PRACTICES);
    char_set_gold(ch, PFDEF_GOLD);
    char_set_backstabcool(ch, 0);
    char_set_con_cooldown(ch, 0);
    char_set_con_sdcooldown(ch, 0);
    char_set_bank_gold(ch, PFDEF_BANK);
    char_set_absorbs(ch, PFDEF_BANK);
    char_set_ingestLearned(ch, PFDEF_BANK);
    char_set_racial_pref(g, ch, PFDEF_BANK);
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
    char_set_real_abils_str(ch, PFDEF_STR);
    char_set_real_abils_dex(ch, PFDEF_DEX);
    char_set_real_abils_intel(ch, PFDEF_INT);
    char_set_real_abils_wis(ch, PFDEF_WIS);
    char_set_real_abils_con(ch, PFDEF_CON);
    char_set_real_abils_cha(ch, PFDEF_CHA);
    //GET_HIT(ch) = PFDEF_HIT;
    //ch->max_hit = PFDEF_MAXHIT;
    //GET_MANA(ch) = PFDEF_MANA;
    //ch->max_mana = PFDEF_MAXMANA;
    //GET_MOVE(ch) = PFDEF_MOVE;
    //ch->max_move = PFDEF_MAXMOVE;
    char_set_ki(ch, PFDEF_KI);
    char_set_max_ki(ch, PFDEF_MAXKI);
    char_set_speaking(ch, PFDEF_SPEAKING);
    char_set_olc_zone(g, ch, PFDEF_OLC);
    char_set_host(g, ch, nullptr);

    char_set_time_maxage(ch, 0);
    char_set_time_created(ch, 0);
    char_set_time_birth(ch, 0);
    char_followers_slot(ch)->clear();
    for (i = 0; i < NUM_COLOR; i++)
      char_get_player_specials(ch)->color_choices[i].clear();

    while (get_line(fl, line)) {
      tag_argument(line, tag);

      switch (*tag) {
        case 'A':
          if (!strcmp(tag, "Ac  ")) char_set_armor(ch, atoi(line));
          else if (!strcmp(tag, "Act ")) {
            sscanf(line, "%127s %127s %127s %127s", f1, f2, f3, f4);
            char_get_act(ch)[0] = asciiflag_conv(f1);
            char_get_act(ch)[1] = asciiflag_conv(f2);
            char_get_act(ch)[2] = asciiflag_conv(f3);
            char_get_act(ch)[3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Aff ")) {
            sscanf(line, "%127s %127s %127s %127s", f1, f2, f3, f4);
            char_get_affected_by(ch)[0] = asciiflag_conv(f1);
            char_get_affected_by(ch)[1] = asciiflag_conv(f2);
            char_get_affected_by(ch)[2] = asciiflag_conv(f3);
            char_get_affected_by(ch)[3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Affs")) load_affects(g, fl, ch);
          else if (!strcmp(tag, "Affv")) skip_affects(fl);
          else if (!strcmp(tag, "AdmL")) char_set_admlevel(ch, atoi(line));
          else if (!strcmp(tag, "Abso")) char_set_absorbs(ch, atoi(line));
          else if (!strcmp(tag, "AdmF")) {
            sscanf(line, "%127s %127s %127s %127s", f1, f2, f3, f4);
            char_get_admflags(ch)[0] = asciiflag_conv(f1);
            char_get_admflags(ch)[1] = asciiflag_conv(f2);
            char_get_admflags(ch)[2] = asciiflag_conv(f3);
            char_get_admflags(ch)[3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Alin")) char_set_alignment(ch, atoi(line));
          else if (!strcmp(tag, "Aura")) char_set_aura(ch, atoi(line));
          break;

        case 'B':
          if (!strcmp(tag, "Badp")) char_set_bad_pws(g, ch, atoi(line));
          else if (!strcmp(tag, "Bank")) char_set_bank_gold(ch, atoi(line));
          else if (!strcmp(tag, "Bki ")) load_BASE(ch, line, LOAD_MANA);
          else if (!strcmp(tag, "Blss")) char_set_blesslvl(ch, atoi(line));
          else if (!strcmp(tag, "Boam")) char_set_lboard(ch, 0, atoll(line));
          else if (!strcmp(tag, "Boai")) char_set_lboard(ch, 1, atoll(line));
          else if (!strcmp(tag, "Boac")) char_set_lboard(ch, 2, atoll(line));
          else if (!strcmp(tag, "Boad")) char_set_lboard(ch, 3, atoll(line));
          else if (!strcmp(tag, "Boab")) char_set_lboard(ch, 4, atoll(line));
          else if (!strcmp(tag, "Bonu")) load_bonuses(fl, ch, false);
          else if (!strcmp(tag, "Boos")) char_set_boosts(ch, atoi(line));
          else if (!strcmp(tag, "Bpl ")) load_BASE(ch, line, LOAD_HIT);
          else if (!strcmp(tag, "Brth")) char_set_time_birth(ch, atoll(line));
          else if (!strcmp(tag, "Bst ")) load_BASE(ch, line, LOAD_MOVE);
          break;

        case 'C':
          if (!strcmp(tag, "Cha ")) char_set_real_abils_cha(ch, atoi(line));
          else if (!strcmp(tag, "Clan"))
            ;
          else if (!strcmp(tag, "Clar"))
            ;
          else if (!strcmp(tag, "Clas")) char_set_class(ch, atoi(line));
          else if (!strcmp(tag, "Colr")) {
            if (sscanf(line, "%d %127s", &num, buf2) == 2 && num >= 0 && num < NUM_COLOR) {
              char_get_player_specials(ch)->color_choices[num] = buf2;
            }
          } else if (!strcmp(tag, "Con ")) char_set_real_abils_con(ch, atoi(line));
          else if (!strcmp(tag, "Cool")) char_set_con_cooldown(ch, atoi(line));
          else if (!strcmp(tag, "Crtd")) char_set_time_created(ch, atoll(line));
          break;

        case 'D':
          if (!strcmp(tag, "Deat")) char_set_deathtime(ch, atoll(line));
          else if (!strcmp(tag, "Deac")) char_set_dcount(ch, atoi(line));
          else if (!strcmp(tag, "Desc")) char_set_ddesc(ch, take_string(fread_string(fl, buf2)));
          else if (!strcmp(tag, "Dex ")) char_set_real_abils_dex(ch, atoi(line));
          else if (!strcmp(tag, "Drnk")) char_set_conditions(g, ch, DRUNK, atoi(line));
          else if (!strcmp(tag, "Damg"))
            ; /* obsolete damage mod: ignored */
          else if (!strcmp(tag, "Droo")) char_set_droom(ch, atoi(line));
          break;

        case 'E':
          if (!strcmp(tag, "Exp ")) char_set_exp(ch, atoll(line));
          else if (!strcmp(tag, "Eali"))
            ;
          else if (!strcmp(tag, "Ecls")) {
            sscanf(line, "%d=%d", &num, &num2);
          } else if (!strcmp(tag, "Eye ")) char_set_eye(ch, atoi(line));
          break;

        case 'F':
          if (!strcmp(tag, "Fisd")) char_set_accuracy_mod(ch, atoi(line));
          else if (!strcmp(tag, "Frez")) char_set_freeze_level(g, ch, atoi(line));
          else if (!strcmp(tag, "Forc")) char_set_forgetcount(ch, atoi(line));
          else if (!strcmp(tag, "Forg")) char_set_forgeting(ch, atoi(line));
          else if (!strcmp(tag, "Fury")) char_set_fury(ch, atoi(line));
          break;

        case 'G':
          if (!strcmp(tag, "Gold")) char_set_gold(ch, atoi(line));
          else if (!strcmp(tag, "Gaun")) char_set_gauntlet(g, ch, atoi(line));
          else if (!strcmp(tag, "Geno")) char_set_genome(ch, 0, atoi(line));
          else if (!strcmp(tag, "Gen1")) char_set_genome(ch, 1, atoi(line));
          break;

        case 'H':
          if (!strcmp(tag, "Hit ")) load_HMVS(ch, line, LOAD_HIT);
          else if (!strcmp(tag, "HitD")) char_set_race_level(ch, atoi(line));
          else if (!strcmp(tag, "Hite")) char_set_height(ch, atoi(line));
          else if (!strcmp(tag, "Home")) char_set_hometown(ch, atoi(line));
          else if (!strcmp(tag, "Host")) char_set_host(g, ch, line);
          else if (!strcmp(tag, "Hrc ")) char_set_hairc(ch, atoi(line));
          else if (!strcmp(tag, "Hrl ")) char_set_hairl(ch, atoi(line));
          else if (!strcmp(tag, "Hrs ")) char_set_hairs(ch, atoi(line));
          else if (!strcmp(tag, "Hung")) char_set_conditions(g, ch, HUNGER, atoi(line));
          break;

        case 'I':
          if (!strcmp(tag, "Id  ")) char_set_idnum(ch, atol(line));
          else if (!strcmp(tag, "INGl")) char_set_ingestLearned(ch, atoi(line));
          else if (!strcmp(tag, "Int ")) char_set_real_abils_intel(ch, atoi(line));
          else if (!strcmp(tag, "Invs")) char_set_invis_level(g, ch, atoi(line));
          break;

        case 'K':
          if (!strcmp(tag, "Ki  ")) load_HMVS(ch, line, LOAD_KI);
          else if (!strcmp(tag, "Kaio")) char_set_kaioken(ch, atoi(line));
          break;

        case 'L':
          if (!strcmp(tag, "Last")) char_set_time_logon(ch, atoll(line));
          else if (!strcmp(tag, "Lern")) char_set_practice_points(g, ch, atoi(line));
          else if (!strcmp(tag, "Levl")) char_set_class_level(ch, atoi(line));
          /* else if (!strcmp(tag, "LevD"))  read_level_data(ch, fl);*/
          else if (!strcmp(tag, "LF  ")) load_BASE(ch, line, LOAD_LIFE);
          else if (!strcmp(tag, "LFPC")) char_set_lifeperc(ch, atoi(line));
          else if (!strcmp(tag, "Lila")) char_set_limb_condition(ch, (2) - 1, atoi(line));
          else if (!strcmp(tag, "Lill")) char_set_limb_condition(ch, (4) - 1, atoi(line));
          else if (!strcmp(tag, "Lira")) char_set_limb_condition(ch, (1) - 1, atoi(line));
          else if (!strcmp(tag, "Lirl")) char_set_limb_condition(ch, (3) - 1, atoi(line));
          else if (!strcmp(tag, "Lint")) char_set_lastint(ch, atoll(line));
          else if (!strcmp(tag, "Lpla")) char_set_lastpl(ch, atoll(line));
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
          else if (!strcmp(tag, "Mimi")) char_set_mimic(ch, atoi(line));
          else if (!strcmp(tag, "MxAg")) char_set_time_maxage(ch, atoll(line));
          break;

        case 'N':
          if (!strcmp(tag, "Name")) char_set_pc_name(ch, line);
          break;

        case 'O':
          if (!strcmp(tag, "Olc ")) char_set_olc_zone(g, ch, atoi(line));
          break;

        case 'P':
          if (!strcmp(tag, "Page")) { /* old page-length setting: no longer used */
          } else if (!strcmp(tag, "Phas")) char_set_distfea(ch, atoi(line));
          else if (!strcmp(tag, "Phse")) char_set_starphase(ch, atoi(line));
          else if (!strcmp(tag, "Plyd")) char_set_time_played(ch, atoll(line));
#ifdef ASCII_SAVE_POOFS
          else if (!strcmp(tag, "PfIn")) char_set_poofin(g, ch, line);
          else if (!strcmp(tag, "PfOt")) char_set_poofout(g, ch, line);
#endif
          else if (!strcmp(tag, "Pole")) char_set_accuracy(ch, atoi(line));
          else if (!strcmp(tag, "Posi")) char_set_position(ch, atoi(line));
          else if (!strcmp(tag, "PwrA")) char_set_powerattack(ch, atoi(line));
          else if (!strcmp(tag, "Pref")) {
            sscanf(line, "%127s %127s %127s %127s", f1, f2, f3, f4);
            char_get_pref(g, ch)
                [0] = asciiflag_conv(f1);
            char_get_pref(g, ch)
                [1] = asciiflag_conv(f2);
            char_get_pref(g, ch)
                [2] = asciiflag_conv(f3);
            char_get_pref(g, ch)
                [3] = asciiflag_conv(f4);
          } else if (!strcmp(tag, "Prff")) char_set_preference(ch, atoi(line));
          break;

        case 'r':
          if (!strcmp(tag, "rDis")) char_set_rdisplay(ch, line);
          break;

        case 'R':
          if (!strcmp(tag, "Race")) char_set_race(ch, atoi(line));
          else if (!strcmp(tag, "Raci")) char_set_racial_pref(g, ch, atoi(line));
          else if (!strcmp(tag, "RBan")) char_set_rbank(ch, atoi(line));
          else if (!strcmp(tag, "Rela")) char_set_relax_count(ch, atoi(line));
          else if (!strcmp(tag, "Rtim")) char_set_rewtime(ch, atoll(line));
          else if (!strcmp(tag, "Rad1")) char_set_radar1(ch, atoi(line));
          else if (!strcmp(tag, "Rad2")) char_set_radar2(ch, atoi(line));
          else if (!strcmp(tag, "Rad3")) char_set_radar3(ch, atoi(line));
          else if (!strcmp(tag, "Room")) char_set_load_room(g, ch, atoi(line));
          else if (!strcmp(tag, "RPfe")) char_set_feature(ch, line);
          else if (!strcmp(tag, "RPP ")) char_set_rp(ch, atoi(line));
          break;

        case 'S':
          if (!strcmp(tag, "Sex ")) char_set_sex(ch, atoi(line));
          else if (!strcmp(tag, "Ship"))
            ; /* obsolete ship: ignored */
          else if (!strcmp(tag, "Scoo")) char_set_con_sdcooldown(ch, atoi(line));
          else if (!strcmp(tag, "Shpr"))
            ; /* obsolete ship room: ignored */
          else if (!strcmp(tag, "Skil")) load_skills(fl, ch, false);
          else if (!strcmp(tag, "Skn ")) char_set_skin(ch, atoi(line));
          else if (!strcmp(tag, "Size")) char_set_size(ch, atoi(line));
          else if (!strcmp(tag, "SklB")) load_skills(fl, ch, true);
          else if (!strcmp(tag, "SkRc"))
            ; /* obsolete race practices: ignored */
          else if (!strcmp(tag, "SkCl")) {
            sscanf(line, "%d %d", &num2, &num3);
            char_set_practice_points(g, ch, num3);
          } else if (!strcmp(tag, "Slot")) char_set_skill_slots(ch, atoi(line));
          else if (!strcmp(tag, "Spek")) char_set_speaking(ch, atoi(line));
          else if (!strcmp(tag, "Str ")) char_set_real_abils_str(ch, atoi(line));
          else if (!strcmp(tag, "Stuk"))
            ;
          else if (!strcmp(tag, "Supp")) char_set_suppression(ch, atoi(line));
          else if (!strcmp(tag, "Sups")) char_set_suppressed(ch, atoi(line));
          break;

        case 'T':
          if (!strcmp(tag, "Tgro")) char_set_tail_growth(ch, atoi(line));
          else if (!strcmp(tag, "Tcla")) char_set_transclass(ch, atoi(line));
          else if (!strcmp(tag, "Tcos")) {
            if (sscanf(line, "%d %d", &num2, &num3) == 2 && num2 >= 0 && num2 < 6)
              char_set_transcost(ch, num2, num3);
          } else if (!strcmp(tag, "Thir")) char_set_conditions(g, ch, THIRST, atoi(line));
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
          else if (!strcmp(tag, "Trns")) char_set_ability_trains(g, ch, atoi(line));
          else if (!strcmp(tag, "Trag")) char_set_trainagl(g, ch, atoi(line));
          else if (!strcmp(tag, "Trco")) char_set_traincon(g, ch, atoi(line));
          else if (!strcmp(tag, "Trin")) char_set_trainint(g, ch, atoi(line));
          else if (!strcmp(tag, "Trsp")) char_set_trainspd(g, ch, atoi(line));
          else if (!strcmp(tag, "Trst")) char_set_trainstr(g, ch, atoi(line));
          else if (!strcmp(tag, "Trwi")) char_set_trainwis(g, ch, atoi(line));
          else if (!strcmp(tag, "Trp ")) char_set_trp(ch, atoi(line));
          break;
        case 'U':
          if (!strcmp(tag, "Upgr")) char_set_upgrade(ch, atoi(line));
          else if (!strcmp(tag, "User")) char_set_loguser(ch, line);
          break;
        case 'V':
          if (!strcmp(tag, "Voic")) char_set_voice(ch, line);
          break;

        case 'W':
          if (!strcmp(tag, "Wate")) char_set_weight(ch, atoi(line));
          else if (!strcmp(tag, "Wimp")) char_set_wimp_level(g, ch, atoi(line));
          else if (!strcmp(tag, "Wis ")) char_set_real_abils_wis(ch, atoi(line));
          break;

        default:
          buf = str_format("SYSERR: Unknown tag {} in pfile {}", tag, name);
      }
    }
  }

  if (!char_get_time_created(ch)) {
    mud_log("No creation timestamp for user {}, using current time", char_get_name(ch));
    char_set_time_created(ch, game_now(g));
  }

  if (!char_get_time_birth(ch)) {
    mud_log("No birthday for user {}, using standard starting age determination", char_get_name(ch));
    char_set_time_birth(ch, game_now(g) - birth_age(g, ch));
  }

  if (!char_get_time_maxage(ch)) {
    mud_log("No max age for user {}, using standard max age determination", char_get_name(ch));
    time_t maxage;
    if (ckd_add(&maxage, char_get_time_birth(ch), max_age(ch)))
      maxage = char_get_time_birth(ch);
    char_set_time_maxage(ch, maxage);
  }

  affect_total(g, ch);

  /* initialization for imms */
  if (char_get_admlevel(ch) >= ADMLVL_IMMORT) {
    for (i = 0; i < SKILL_TABLE_SIZE; i++)
      char_set_skills_base(ch, i, 100);
    char_set_conditions(g, ch, HUNGER, -1);
    char_set_conditions(g, ch, THIRST, -1);
    char_set_conditions(g, ch, DRUNK, -1);
  }

  if (char_is_android(ch)) {
    char_set_conditions(g, ch, HUNGER, -1);
    char_set_conditions(g, ch, THIRST, -1);
    char_set_conditions(g, ch, DRUNK, -1);
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
static void kill_ems(std::string &str)
{
  std::string out;

  for (size_t i = 0; i < str.size(); ++i) {
    out += str[i];
    if (str[i] == '\r' && i + 1 < str.size() && str[i + 1] == '\r')
      ++i; /* "\r\r" keeps one */
  }
  str = std::move(out);
}

/*
 * write the vital data of a player to the player file
 *
 * And that's it! No more fudging around with the load room.
 */
/* This is the ASCII Player Files save routine */
void save_char(GameWorld *g, struct char_data *ch)
{
  struct atomic_file af;
  FILE *fl;
  char fname[40];
  std::string buf;
  int i, id, save_index = false;
  struct affected_type tmp_aff[MAX_AFFECT];
  struct obj_data *char_eq[NUM_WEARS];
  char fbuf1[MAX_STRING_LENGTH], fbuf2[MAX_STRING_LENGTH];
  char fbuf3[MAX_STRING_LENGTH], fbuf4[MAX_STRING_LENGTH];

  if (char_is_npc(ch) || char_get_pfilepos(ch) < 0)
    return;

  /*
   * If ch->desc is not null, then we need to update some session data
   * before saving.
   */
  if (char_get_desc(ch)) {
    if (*char_get_desc(ch)->host) {
      if (!char_get_host(g, ch))
        char_set_host(g, ch, char_get_desc(ch)->host);
      else if (char_get_host(g, ch) && !strcmp(char_get_host(g, ch), char_get_desc(ch)->host))
        char_set_host(g, ch, char_get_desc(ch)->host);
    }

    /*
     * We only update the time.played and time.logon if the character
     * is playing.
     */
    if (desc_get_connected(char_get_desc(ch)) == CON_PLAYING) {
      char_set_time_played(ch, char_get_time_played(ch) + (game_now(g) - char_get_time_logon(ch)));
      char_set_time_logon(ch, game_now(g));
    }
  }

  if (!get_filename(fname, sizeof(fname), PLR_FILE, char_get_name(ch)))
    return;
  if (!(fl = atomic_open(&af, fname, true))) {
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: Couldn't open player file {} for write", fname);
    return;
  }

  /* remove affects from eq and spells (from char_to_store) */
  /* Unaffect everything a character can be affected by */

  if (char_get_trp(ch) < char_get_rp(ch)) {
    char_set_trp(ch, char_get_rp(ch));
  }
  if (char_get_desc(ch) != nullptr && !char_get_desc(ch)->user.empty()) {
    userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i))
      char_eq[i] = unequip_char(g, ch, i);
    else
      char_eq[i] = nullptr;
  }

  auto aff = char_affects(ch).begin();
  for (i = 0; i < MAX_AFFECT; i++) {
    if (aff != char_affects(ch).end()) {
      tmp_aff[i] = *aff;
      ++aff;
    } else {
      tmp_aff[i].type      = 0; /* Zero signifies not used */
      tmp_aff[i].duration  = 0;
      tmp_aff[i].modifier  = 0;
      tmp_aff[i].specific  = 0;
      tmp_aff[i].location  = 0;
      tmp_aff[i].bitvector = 0;
    }
  }

  save_char_vars(g, ch);

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  if ((i >= MAX_AFFECT) && aff != char_affects(ch).end() && std::next(aff) != char_affects(ch).end())
    mud_log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  while (!char_affects(ch).empty())
    affect_remove(g, ch, &char_affects(ch).front());

  char_reset_aff_abils(ch);

  /* end char_to_store code */

  if (char_get_name(ch)) file_print(fl, "Name: {}\n", char_get_name(ch));
  if (char_get_user(ch)) file_print(fl, "User: {}\n", char_get_user(ch));
  if (char_get_voice(ch)) file_print(fl, "Voic: {}\n", char_get_voice(ch));
  if (char_get_feature(ch)) file_print(fl, "RPfe: {}\n", char_get_feature(ch));
  if (char_get_ability_trains(g, ch)) file_print(fl, "Trns: {:d}\n", char_get_ability_trains(g, ch));
  if (char_get_ddesc(ch) && *char_get_ddesc(ch)) {
    buf = char_get_ddesc(ch);
    kill_ems(buf);
    file_print(fl, "Desc:\n{}~\n", buf);
  }
#ifdef ASCII_SAVE_POOFS
  if (char_get_poofin(g, ch)) file_print(fl, "PfIn: {}\n", char_get_poofin(g, ch));
  if (char_get_poofout(g, ch)) file_print(fl, "PfOt: {}\n", char_get_poofout(g, ch));
#endif
  if (char_get_sex(ch) != PFDEF_SEX) file_print(fl, "Sex : {:d}\n", char_get_sex(ch));
  if (char_get_size(ch) != PFDEF_SIZE) file_print(fl, "Size: {:d}\n", char_get_size(ch));
  if (char_get_class(ch) != PFDEF_CLASS) file_print(fl, "Clas: {:d}\n", char_get_class(ch));
  if (char_get_race(ch) != PFDEF_RACE) file_print(fl, "Race: {:d}\n", char_get_race(ch));
  if (char_get_racial_pref(ch) != PFDEF_BANK) file_print(fl, "Raci: {:d}\n", char_get_racial_pref(ch));
  if (char_get_admlevel(ch) != PFDEF_LEVEL) file_print(fl, "AdmL: {:d}\n", char_get_admlevel(ch));
  if (char_get_class_level(ch) != PFDEF_LEVEL) file_print(fl, "Levl: {:d}\n", char_get_class_level(ch));
  if (char_get_race_level(ch) != PFDEF_LEVEL) file_print(fl, "HitD: {:d}\n", char_get_race_level(ch));
  if (char_get_level_adj(ch) != PFDEF_LEVEL) file_print(fl, "LvlA: {:d}\n", char_get_level_adj(ch));
  if (char_get_hometown(ch) != PFDEF_HOMETOWN) file_print(fl, "Home: {:d}\n", char_get_hometown(ch));
  file_print(fl, "Id  : {:d}\n", char_get_idnum(ch));
  file_print(fl, "Brth: {:d}\n", (long long)char_get_time_birth(ch));
  file_print(fl, "Crtd: {:d}\n", (long long)char_get_time_created(ch));
  file_print(fl, "MxAg: {:d}\n", (long long)char_get_time_maxage(ch));
  file_print(fl, "Plyd: {:d}\n", (long long)char_get_time_played(ch));
  file_print(fl, "Last: {:d}\n", (long long)char_get_time_logon(ch));

  if (char_get_host(g, ch)) file_print(fl, "Host: {}\n", char_get_host(g, ch));
  if (char_get_height(ch) != PFDEF_HEIGHT) file_print(fl, "Hite: {:d}\n", char_get_height(ch));
  if (char_get_weight(ch) != PFDEF_HEIGHT) file_print(fl, "Wate: {:d}\n", char_get_weight(ch));
  if (char_get_alignment(ch) != PFDEF_ALIGNMENT) file_print(fl, "Alin: {:d}\n", char_get_alignment(ch));
  if (char_get_aura(ch) != PFDEF_SKIN) file_print(fl, "Aura: {:d}\n", char_get_aura(ch));

  sprintascii(fbuf1, char_get_act(ch)[0]);
  sprintascii(fbuf2, char_get_act(ch)[1]);
  sprintascii(fbuf3, char_get_act(ch)[2]);
  sprintascii(fbuf4, char_get_act(ch)[3]);
  file_print(fl, "Act : {} {} {} {}\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, char_get_affected_by(ch)[0]);
  sprintascii(fbuf2, char_get_affected_by(ch)[1]);
  sprintascii(fbuf3, char_get_affected_by(ch)[2]);
  sprintascii(fbuf4, char_get_affected_by(ch)[3]);
  file_print(fl, "Aff : {} {} {} {}\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, char_get_pref(g, ch)[0]);
  sprintascii(fbuf2, char_get_pref(g, ch)[1]);
  sprintascii(fbuf3, char_get_pref(g, ch)[2]);
  sprintascii(fbuf4, char_get_pref(g, ch)[3]);
  file_print(fl, "Pref: {} {} {} {}\n", fbuf1, fbuf2, fbuf3, fbuf4);
  sprintascii(fbuf1, char_get_admflags(ch)[0]);
  sprintascii(fbuf2, char_get_admflags(ch)[1]);
  sprintascii(fbuf3, char_get_admflags(ch)[2]);
  sprintascii(fbuf4, char_get_admflags(ch)[3]);
  file_print(fl, "AdmF: {} {} {} {}\n", fbuf1, fbuf2, fbuf3, fbuf4);

  if (char_get_wimp_level(g, ch) != PFDEF_WIMPLEV) file_print(fl, "Wimp: {:d}\n", char_get_wimp_level(g, ch));
  if (char_get_powerattack(ch) != PFDEF_POWERATT) file_print(fl, "PwrA: {:d}\n", char_get_powerattack(ch));
  if (char_get_freeze_level(g, ch) != PFDEF_FREEZELEV) file_print(fl, "Frez: {:d}\n", char_get_freeze_level(g, ch));
  if (char_get_invis_level(g, ch) != PFDEF_INVISLEV) file_print(fl, "Invs: {:d}\n", char_get_invis_level(g, ch));
  if (char_get_load_room(g, ch) != PFDEF_LOADROOM) file_print(fl, "Room: {:d}\n", char_get_load_room(g, ch));

  if (char_get_bad_pws(g, ch) != PFDEF_BADPWS) file_print(fl, "Badp: {:d}\n", char_get_bad_pws(g, ch));

  for (i = 0; i < 6; i++)
    if (char_get_transcost(ch, i) != false)
      file_print(fl, "Tcos: {:d} {:d}\n", i, char_get_transcost(ch, i));

  for (i = 0; i < NUM_CLASSES; i++)
    if (char_get_practice_points(g, ch) != PFDEF_PRACTICES)
      file_print(fl, "SkCl: {:d} {:d}\n", i, char_get_practice_points(g, ch));

  if (char_get_conditions(g, ch, HUNGER) != PFDEF_HUNGER && char_get_admlevel(ch) < ADMLVL_IMMORT)
    file_print(fl, "Hung: {:d}\n", char_get_conditions(g, ch, HUNGER));
  if (char_get_conditions(g, ch, THIRST) != PFDEF_THIRST && char_get_admlevel(ch) < ADMLVL_IMMORT)
    file_print(fl, "Thir: {:d}\n", char_get_conditions(g, ch, THIRST));
  if (char_get_conditions(g, ch, DRUNK) != PFDEF_DRUNK && char_get_admlevel(ch) < ADMLVL_IMMORT)
    file_print(fl, "Drnk: {:d}\n", char_get_conditions(g, ch, DRUNK));

  /*
  if (GET_HIT(ch)	   != PFDEF_HIT  || GET_MAX_HIT(ch)  != PFDEF_MAXHIT)
    fprintf(fl, "Hit : %" PRId64 "/%" PRId64 "\n", GET_HIT(ch),  GET_MAX_HIT(ch));
  if ((getCurKI(ch))	   != PFDEF_MANA || GET_MAX_MANA(ch) != PFDEF_MAXMANA)
    fprintf(fl, "Mana: %" PRId64 "/%" PRId64 "\n", (getCurKI(ch)), GET_MAX_MANA(ch));
  if ((getCurST(ch))	   != PFDEF_MOVE || GET_MAX_MOVE(ch) != PFDEF_MAXMOVE)
    fprintf(fl, "Move: %" PRId64 "/%" PRId64 "\n", (getCurST(ch)), GET_MAX_MOVE(ch));
  if (GET_KI(ch)	   != PFDEF_KI || GET_MAX_KI(ch) != PFDEF_MAXKI)
    fprintf(fl, "Ki  : %" PRId64 "/%" PRId64 "\n", GET_KI(ch), GET_MAX_KI(ch));
  */
  if (char_get_aff_abils_str(ch) != PFDEF_STR) file_print(fl, "Str : {:d}\n", char_get_aff_abils_str(ch));
  if (char_get_aff_abils_intel(ch) != PFDEF_INT) file_print(fl, "Int : {:d}\n", char_get_aff_abils_intel(ch));
  if (char_get_aff_abils_wis(ch) != PFDEF_WIS) file_print(fl, "Wis : {:d}\n", char_get_aff_abils_wis(ch));
  if (char_get_aff_abils_dex(ch) != PFDEF_DEX) file_print(fl, "Dex : {:d}\n", char_get_aff_abils_dex(ch));
  if (char_get_aff_abils_con(ch) != PFDEF_CON) file_print(fl, "Con : {:d}\n", char_get_aff_abils_con(ch));
  if (char_get_aff_abils_cha(ch) != PFDEF_CHA) file_print(fl, "Cha : {:d}\n", char_get_aff_abils_cha(ch));

  if (char_get_con_cooldown(ch) != PFDEF_BANK) file_print(fl, "Cool: {:d}\n", char_get_con_cooldown(ch));
  if (char_get_con_cooldown(ch) != PFDEF_BANK) file_print(fl, "Scoo: {:d}\n", char_get_con_sdcooldown(ch));
  if (char_get_armor(ch) != PFDEF_AC) file_print(fl, "Ac  : {:d}\n", char_get_armor(ch));
  if (char_get_absorbs(ch) != PFDEF_GOLD) file_print(fl, "Abso: {:d}\n", char_get_absorbs(ch));
  if (char_get_ingestLearned(ch) != PFDEF_GOLD) file_print(fl, "INGl: {:d}\n", char_get_ingestLearned(ch));
  if (char_get_upgrade(ch) != PFDEF_GOLD) file_print(fl, "Upgr: {:d}\n", char_get_upgrade(ch));
  if (char_get_forgeting(ch) != PFDEF_BANK) file_print(fl, "Forg: {:d}\n", char_get_forgeting(ch));
  if (char_get_forgetcount(ch) != PFDEF_BANK) file_print(fl, "Forc: {:d}\n", char_get_forgetcount(ch));
  if (char_get_kaioken(ch) != PFDEF_GOLD) file_print(fl, "Kaio: {:d}\n", char_get_kaioken(ch));
  if (char_get_gold(ch) != PFDEF_GOLD) file_print(fl, "Gold: {:d}\n", char_get_gold(ch));
  if (char_get_bank_gold(ch) != PFDEF_BANK) file_print(fl, "Bank: {:d}\n", char_get_bank_gold(ch));
  if (char_get_exp(ch) != PFDEF_EXP) file_print(fl, "Exp : {:d}\n", char_get_exp(ch));
  if (char_get_transclass(ch) != PFDEF_EXP) file_print(fl, "Tcla: {:d}\n", char_get_transclass(ch));
  if (char_get_moltexp(ch) != PFDEF_EXP) file_print(fl, "Mexp: {:d}\n", char_get_moltexp(ch));
  if (char_get_majinizer(ch) != PFDEF_EXP) file_print(fl, "Majm: {:d}\n", char_get_majinizer(ch));
  if (char_get_moltlevel(ch) != PFDEF_EXP) file_print(fl, "Mlvl: {:d}\n", char_get_moltlevel(ch));
  if (char_get_accuracy_mod(ch) != PFDEF_ACCURACY) file_print(fl, "Fisd: {:d}\n", char_get_accuracy_mod(ch));
  if (char_get_accuracy(ch) != PFDEF_ACCURACY) file_print(fl, "Pole: {:d}\n", char_get_accuracy(ch));
  if (char_get_preference(ch) != PFDEF_EYE) file_print(fl, "Prff: {:d}\n", char_get_preference(ch));
  if (char_get_speaking(ch) != PFDEF_SPEAKING) file_print(fl, "Spek: {:d}\n", char_get_speaking(ch));
  if (char_get_olc_zone(g, ch) != PFDEF_OLC) file_print(fl, "Olc : {:d}\n", char_get_olc_zone(g, ch));
  if (char_get_gauntlet(g, ch) != PFDEF_GAUNTLET) file_print(fl, "Gaun: {:d}\n", char_get_gauntlet(g, ch));
  if (char_get_genome(ch, 0) != PFDEF_EYE) file_print(fl, "Geno: {:d}\n", char_get_genome(ch, 0));
  if (char_get_genome(ch, 1) != PFDEF_EYE) file_print(fl, "Gen1: {:d}\n", char_get_genome(ch, 1));
  if (char_get_position(ch) != POS_STANDING) file_print(fl, "Posi: {:d}\n", char_get_position(ch));
  //if ((getCurLF(ch))    != PFDEF_BASEPL)     fprintf(fl, "LF  : %" PRId64 "\n", (getCurLF(ch)));
  if (char_get_lifeperc(ch) != PFDEF_WEIGHT) file_print(fl, "LFPC: {:d}\n", char_get_lifeperc(ch));
  if (getBasePL(ch) != PFDEF_BASEPL) file_print(fl, "Bpl : {:d}\n", getBasePL(ch));
  if (getBaseKI(ch) != PFDEF_BASEKI) file_print(fl, "Bki : {:d}\n", getBaseKI(ch));
  if (getBaseST(ch) != PFDEF_BASEST) file_print(fl, "Bst : {:d}\n", getBaseST(ch));
  if (char_get_droom(ch) != PFDEF_DROOM) file_print(fl, "Droo: {:d}\n", char_get_droom(ch));
  if (char_get_hairl(ch) != PFDEF_HAIRL) file_print(fl, "Hrl : {:d}\n", char_get_hairl(ch));
  if (char_get_hairs(ch) != PFDEF_HAIRS) file_print(fl, "Hrs : {:d}\n", char_get_hairs(ch));
  if (char_get_hairc(ch) != PFDEF_HAIRC) file_print(fl, "Hrc : {:d}\n", char_get_hairc(ch));
  if (char_get_skin(ch) != PFDEF_SKIN) file_print(fl, "Skn : {:d}\n", char_get_skin(ch));
  if (char_get_eye(ch) != PFDEF_EYE) file_print(fl, "Eye : {:d}\n", char_get_eye(ch));
  if (char_get_distfea(ch) != PFDEF_DISTFEA) file_print(fl, "Phas: {:d}\n", char_get_distfea(ch));
  if (char_get_fury(ch) != PFDEF_HAIRL) file_print(fl, "Fury: {:d}\n", char_get_fury(ch));
  if (char_get_radar1(ch) != PFDEF_RADAR1) file_print(fl, "Rad1: {:d}\n", char_get_radar1(ch));
  if (char_get_radar2(ch) != PFDEF_RADAR2) file_print(fl, "Rad2: {:d}\n", char_get_radar2(ch));
  if (char_get_radar3(ch) != PFDEF_RADAR3) file_print(fl, "Rad3: {:d}\n", char_get_radar3(ch));
  if (char_get_lastpl(ch) != PFDEF_LPLAY) file_print(fl, "Lpla: {:d}\n", (long long)char_get_lastpl(ch));
  if (char_get_lastint(ch) != PFDEF_LPLAY) file_print(fl, "Lint: {:d}\n", (long long)char_get_lastint(ch));
  if (char_get_deathtime(ch) != PFDEF_LPLAY) file_print(fl, "Deat: {:d}\n", (long long)char_get_deathtime(ch));
  if (char_get_rewtime(ch) != PFDEF_LPLAY) file_print(fl, "Rtim: {:d}\n", (long long)char_get_rewtime(ch));
  if (char_get_boosts(ch) != PFDEF_DISTFEA) file_print(fl, "Boos: {:d}\n", char_get_boosts(ch));
  if (char_get_majinize(ch) != PFDEF_LPLAY) file_print(fl, "Maji: {:d}\n", char_get_majinize(ch));
  if (char_get_blesslvl(ch) != PFDEF_HEIGHT) file_print(fl, "Blss: {:d}\n", char_get_blesslvl(ch));
  if (char_get_lboard(ch, 0) != PFDEF_BOARD) file_print(fl, "Boam: {:d}\n", (long long)char_get_lboard(ch, 0));
  if (char_get_lboard(ch, 1) != PFDEF_BOARD) file_print(fl, "Boai: {:d}\n", (long long)char_get_lboard(ch, 1));
  if (char_get_lboard(ch, 2) != PFDEF_BOARD) file_print(fl, "Boac: {:d}\n", (long long)char_get_lboard(ch, 2));
  if (char_get_lboard(ch, 3) != PFDEF_BOARD) file_print(fl, "Boad: {:d}\n", (long long)char_get_lboard(ch, 3));
  if (char_get_lboard(ch, 4) != PFDEF_BOARD) file_print(fl, "Boab: {:d}\n", (long long)char_get_lboard(ch, 4));
  if (char_get_limb_condition(ch, (1) - 1) != PFDEF_BOARD) file_print(fl, "Lira: {:d}\n", char_get_limb_condition(ch, (1) - 1));
  if (char_get_limb_condition(ch, (2) - 1) != PFDEF_BOARD) file_print(fl, "Lila: {:d}\n", char_get_limb_condition(ch, (2) - 1));
  if (char_get_limb_condition(ch, (3) - 1) != PFDEF_BOARD) file_print(fl, "Lirl: {:d}\n", char_get_limb_condition(ch, (3) - 1));
  if (char_get_limb_condition(ch, (4) - 1) != PFDEF_BOARD) file_print(fl, "Lill: {:d}\n", char_get_limb_condition(ch, (4) - 1));
  if (char_get_rp(ch) != PFDEF_SKIN) file_print(fl, "RPP : {:d}\n", char_get_rp(ch));
  if (char_get_rbank(ch) != PFDEF_SKIN) file_print(fl, "RBan: {:d}\n", char_get_rbank(ch));
  if (char_get_suppression(ch) != PFDEF_SKIN) file_print(fl, "Supp: {:d}\n", char_get_suppression(ch));
  if (char_get_suppressed(ch) != PFDEF_SKIN) file_print(fl, "Sups: {:d}\n", char_get_suppressed(ch));
  if (char_get_trp(ch) != PFDEF_SKIN) file_print(fl, "Trp : {:d}\n", char_get_trp(ch));
  if (char_get_dcount(ch) != PFDEF_EYE) file_print(fl, "Deac: {:d}\n", char_get_dcount(ch));
  if (char_get_trainagl(g, ch) != PFDEF_EYE) file_print(fl, "Trag: {:d}\n", char_get_trainagl(g, ch));
  if (char_get_traincon(g, ch) != PFDEF_EYE) file_print(fl, "Trco: {:d}\n", char_get_traincon(g, ch));
  if (char_get_trainint(g, ch) != PFDEF_EYE) file_print(fl, "Trin: {:d}\n", char_get_trainint(g, ch));
  if (char_get_trainspd(g, ch) != PFDEF_EYE) file_print(fl, "Trsp: {:d}\n", char_get_trainspd(g, ch));
  if (char_get_trainstr(g, ch) != PFDEF_EYE) file_print(fl, "Trst: {:d}\n", char_get_trainstr(g, ch));
  if (char_get_trainwis(g, ch) != PFDEF_EYE) file_print(fl, "Trwi: {:d}\n", char_get_trainwis(g, ch));
  if (char_get_starphase(ch) != PFDEF_EYE) file_print(fl, "Phse: {:d}\n", char_get_starphase(ch));
  if (char_get_mimic(ch)) file_print(fl, "Mimi: {:d}\n", char_get_mimic(ch));
  if (char_get_skill_slots(ch) != PFDEF_EYE) file_print(fl, "Slot: {:d}\n", char_get_skill_slots(ch));
  if (char_get_tail_growth(ch) != PFDEF_EYE) file_print(fl, "Tgro: {:d}\n", char_get_tail_growth(ch));
  if (char_get_rdisplay(ch)) file_print(fl, "rDis: {}\n", char_get_rdisplay(ch));
  if (char_get_relax_count(ch) != PFDEF_EYE) file_print(fl, "Rela: {:d}\n", char_get_relax_count(ch));

  /* Save skills */
  if (char_get_admlevel(ch) < ADMLVL_IMMORT) {
    file_print(fl, "Skil:\n");
    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
      if (char_get_skills_base(ch, i))
        file_print(fl, "{:d} {:d} {:d}\n", i, char_get_skills_base(ch, i), char_get_skills_perf(ch, i));
    }
    file_print(fl, "0 0\n");
  }

  /* Save Bonuses/Negatives */
  std::string buff;
  file_print(fl, "Bonu:\n");
  for (i = 0; i < 52; i++) {
    if (char_get_bonuses(ch, i) && i == 0)
      buff = str_format("{:d}", char_get_bonuses(ch, i));
    else if (char_get_bonuses(ch, i) && i != 0)
      buff += str_format(" {:d}", char_get_bonuses(ch, i));
    else if (i == 0)
      buff = "0";
    else
      buff += " 0";
  }
  file_print(fl, "{}\n", buff);
  buff.clear();

  /* Save skill bonuses */
  if (char_get_admlevel(ch) < ADMLVL_IMMORT) {
    file_print(fl, "SklB:\n");
    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
      if (char_get_skills_mod(ch, i))
        file_print(fl, "{:d} {:d} {:d}\n", i, char_get_skills_mod(ch, i), char_get_skills_perf(ch, i));
    }
    file_print(fl, "0 0\n");
  }

  /* Save feats 
   *  fprintf(fl, "Feat:\n");
   *  for (i = 1; i <= NUM_FEATS_DEFINED; i++) {
   *   if (HAS_FEAT(ch, i))
   *     fprintf(fl, "%d %d\n", i, HAS_FEAT(ch, i));
   *  }
   *  fprintf(fl, "0 0\n"); */

  /* Save affects */
  file_print(fl, "Affs:\n");
  for (i = 0; i < MAX_AFFECT; i++) {
    const struct affected_type *saved = &tmp_aff[i];
    if (saved->type)
      file_print(fl, "{:d} {:d} {:d} {:d} {:d} {:d}\n", saved->type, saved->duration,
                 saved->modifier, saved->location, (int)saved->bitvector, saved->specific);
  }
  file_print(fl, "0 0 0 0 0 0\n");

  /*fprintf(fl, "LevD:\n");
  write_level_data(ch, fl);*/

  for (i = 0; i < NUM_COLOR; i++)
    if (!char_get_player_specials(ch)->color_choices[i].empty()) {
      file_print(fl, "Colr: {:d} {}\r\n", i, char_get_player_specials(ch)->color_choices[i]);
    }

  atomic_commit(&af);

  /* more char_to_store code to restore affects */

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (tmp_aff[i].type)
      affect_to_char(g, ch, &tmp_aff[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
      equip_char(g, ch, char_eq[i], i);
  }

  /* end char_to_store code */

  if ((id = get_ptable_by_name(g, char_get_name(ch))) < 0)
    return;

  /* update the player in the player index */
  if (g->player_table[id].level != char_get_level(ch)) {
    save_index             = true;
    g->player_table[id].level = char_get_level(ch);
  }
  if (g->player_table[id].admlevel != char_get_admlevel(ch)) {
    save_index                = true;
    g->player_table[id].admlevel = char_get_admlevel(ch);
  }
  if (g->player_table[id].last != char_get_time_logon(ch)) {
    save_index            = true;
    g->player_table[id].last = char_get_time_logon(ch);
  }
  if (g->player_table[id].played != char_get_lastpl(ch)) {
    save_index              = true;
    g->player_table[id].played = char_get_lastpl(ch);
  }
  i = g->player_table[id].flags;
  if (char_plr_flagged(ch, PLR_DELETED))
    SET_BIT(g->player_table[id].flags, PINDEX_DELETED);
  else
    REMOVE_BIT(g->player_table[id].flags, PINDEX_DELETED);
  if (char_plr_flagged(ch, PLR_NODELETE) || char_plr_flagged(ch, PLR_CRYO))
    SET_BIT(g->player_table[id].flags, PINDEX_NODELETE);
  else
    REMOVE_BIT(g->player_table[id].flags, PINDEX_NODELETE);

  if (char_plr_flagged(ch, PLR_FROZEN) || char_plr_flagged(ch, PLR_NOWIZLIST))
    SET_BIT(g->player_table[id].flags, PINDEX_NOWIZLIST);
  else
    REMOVE_BIT(g->player_table[id].flags, PINDEX_NOWIZLIST);

  if (g->player_table[id].flags != i || save_index)
    save_player_index(g);
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

static void load_affects(GameWorld *g, FILE *fl, struct char_data *ch)
{
  int num, num2, num3, num4, num5, num6;
  char line[MAX_INPUT_LENGTH + 1];
  struct affected_type af = {};

  do {
    if (!get_line(fl, line))
      break;
    num = num2 = num3 = num4 = num5 = num6 = 0;
    sscanf(line, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
    if (num != 0) {
      af.type      = num;
      af.duration  = num2;
      af.modifier  = num3;
      af.location  = num4;
      af.bitvector = num5;
      af.specific  = affect_checked_specific(num4, num6);
      if (affect_is_loadable(&af))
        affect_to_char(g, ch, &af);
      else
        mud_log("SYSERR: {} has an impossible saved affect: {}", char_get_name(ch), line);
    }
  } while (num != 0);
}

/* Older player files still carry the retired "Affv" list; read past it up to its "0" terminator. */
static void skip_affects(FILE *fl)
{
  char line[MAX_INPUT_LENGTH + 1];

  while (get_line(fl, line) && atoi(line) != 0)
    ;
}

static void load_skills(FILE *fl, struct char_data *ch, bool mods)
{
  int num = 0, num2 = 0, num3 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    if (!get_line(fl, line))
      break;
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

  sscanf(line, "%" SCNd64 "/%" SCNd64 "", &num, &num2);

  switch (mode) {
    case LOAD_HIT:
      //GET_HIT(ch) = num;
      char_set_max_hit(ch, num2);
      break;

    case LOAD_MANA:
      //GET_MANA(ch) = num;
      char_set_max_mana(ch, num2);
      break;

    case LOAD_MOVE:
      //GET_MOVE(ch) = num;
      char_set_max_move(ch, num2);
      break;

    case LOAD_KI:
      //GET_KI(ch) = num;
      char_set_max_ki(ch, num2);
      break;
  }
}

static void load_BASE(struct char_data *ch, const char *line, int mode)
{
  int64_t num = 0;

  sscanf(line, "%" SCNd64 "", &num);

  switch (mode) {
    case LOAD_HIT:
      char_set_basepl(ch, num);
      break;

    case LOAD_MANA:
      char_set_baseki(ch, num);
      break;

    case LOAD_MOVE:
      char_set_basest(ch, num);
      break;

    case LOAD_LIFE:
      //GET_LIFEFORCE(ch) = num;
      break;
  }
}

static void load_majin(struct char_data *ch, const char *line)
{
  int64_t num = 0;

  sscanf(line, "%" SCNd64 "", &num);
  char_set_majinizer(ch, num);
}

static void load_molt(struct char_data *ch, const char *line)
{
  int64_t num = 0;

  sscanf(line, "%" SCNd64 "", &num);
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
void remove_player(GameWorld *g, int pfilepos)
{
  char fname[40];
  int i;

  if (!*g->player_table[pfilepos].name)
    return;

  /* Unlink all player-owned files */
  for (i = 0; i < MAX_FILES; i++) {
    if (get_filename(fname, sizeof(fname), i, g->player_table[pfilepos].name))
      unlink(fname);
    if (get_filename(fname, sizeof(fname), i, CAP(g->player_table[pfilepos].name)))
      unlink(fname);
  }

  std::string last = asctime(localtime(&g->player_table[pfilepos].last));
  if (!last.empty() && last.back() == '\n')
    last.pop_back();
  mud_log("PCLEAN: {} Lev: {:d} Last: {}", g->player_table[pfilepos].name, g->player_table[pfilepos].level, last);
  g->player_table[pfilepos].name[0] = '\0';
  save_player_index(g);
}

void clean_pfiles(GameWorld *g)
{
  int i, ci;

  for (i = 0; i <= g->top_of_p_table; i++) {
    /*
     * We only want to go further if the player isn't protected
     * from deletion and hasn't already been deleted.
     */
    if (!IS_SET(g->player_table[i].flags, PINDEX_NODELETE) &&
        *g->player_table[i].name) {
      /*
       * If the player is already flagged for deletion, then go
       * ahead and get rid of him.
       */
      if (IS_SET(g->player_table[i].flags, PINDEX_DELETED)) {
        remove_player(g, i);
      } else {
        /*
         * Now we check to see if the player has overstayed his
         * welcome based on level.
         */
        for (ci = 0; pclean_criteria[ci].level > -1; ci++) {
          if (g->player_table[i].admlevel > 1)
            continue;
          else if (g->player_table[i].level <= pclean_criteria[ci].level && ((game_now(g) - g->player_table[i].last) >= (pclean_criteria[ci].days * SECS_PER_REAL_DAY))) {
            remove_player(g, i);
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
