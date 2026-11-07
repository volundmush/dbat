/************************************************************************
 * Generic OLC Library - Mobiles / genmob.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/iterate.hpp"
#include "dbat/arrays.hpp"
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/genmob.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/handler.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/class.hpp"
#include "dbat/affect.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static bool free_mobile_strings(struct char_data *mob);
static bool write_mobile_record(GameWorld *g, mob_vnum mvnum, struct char_data *mob, FILE *fd);

/* From db.c */
void init_mobile_skills(void);

static bool update_mobile_strings(struct char_data *t, struct char_data *f);
static void check_mobile_strings(GameWorld *g, struct char_data *mob);
static void check_mobile_string(GameWorld *g, mob_vnum i, char **string, const char *dscr);
static bool write_mobile_espec(GameWorld *g, mob_vnum mvnum, struct char_data *mob, FILE *fd);
static bool copy_mobile_strings(struct char_data *t, struct char_data *f);

#if CONFIG_GENOLC_MOBPROG
bool write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd);
#endif

/* local functions */
static void extract_mobile_all(GameWorld *g, mob_vnum vnum);

mob_rnum add_mobile(GameWorld *g, struct char_data *mob, mob_vnum vnum)
{
  mob_rnum rnum, i, found = false;
  int cmd_no;

  if ((rnum = real_mobile(g, vnum)) != NOBODY) {
    /* Copy over the mobile and free() the old strings. */
    copy_mobile(g, &g->mob_proto[rnum], mob);
    char_set_nr(&g->mob_proto[rnum], rnum); /* medit's copy option can bring another mob's number */

    /* Now re-point all existing mobile strings to here. */
    for (char_data *live_mob : each_char(g))
      if (rnum == char_get_nr(live_mob))
        update_mobile_strings(live_mob, &g->mob_proto[rnum]);

    add_to_save_list(g, find_zone_by_thing(g, vnum)->number, SL_MOB);
    mud_log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    return rnum;
  }

  resize_array(g->mob_proto, g->top_mob + 1, g->top_mob + 2);
  resize_array(g->mob_index, g->top_mob + 1, g->top_mob + 2);
  g->top_mob++;

  for (i = g->top_mob; i > 0; i--) {
    if (vnum > g->mob_index[i - 1].vnum) {
      g->mob_proto[i]    = *mob;
      char_set_nr(&g->mob_proto[i], i);
      copy_mobile_strings(g->mob_proto + i, mob);
      g->mob_index[i].vnum   = vnum;
      g->mob_index[i].number = 0;
      g->mob_index[i].func   = nullptr;
      found               = i;
      break;
    }
    g->mob_index[i] = g->mob_index[i - 1];
    g->mob_proto[i] = g->mob_proto[i - 1];
    char_set_nr(&g->mob_proto[i], char_get_nr(&g->mob_proto[i]) + 1);
  }
  if (!found) {
    g->mob_proto[0]    = *mob;
    char_set_nr(&g->mob_proto[0], 0);
    copy_mobile_strings(&g->mob_proto[0], mob);
    g->mob_index[0].vnum   = vnum;
    g->mob_index[0].number = 0;
    g->mob_index[0].func   = nullptr;
  }

  mud_log("GenOLC: add_mobile: Added mobile %d at index #%d.", vnum, found);

#if CONFIG_GENOLC_MOBPROG
  GET_MPROG(OLC_MOB(d))      = OLC_MPROGL(d);
  GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
  while (OLC_MPROGL(d)) {
    GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
    OLC_MPROGL(d) = OLC_MPROGL(d)->next;
  }
#endif

  /* Update live mobile rnums. */
  for (char_data *live_mob : each_char(g))
    char_set_nr(live_mob, char_get_nr(live_mob) + ((char_get_nr(live_mob) != NOTHING && char_get_nr(live_mob) >= found)));

  /* Update zone table. */
  for (struct zone_data *zone : g->zones.all())
    for (cmd_no = 0; zone_get_cmd(g, zone, cmd_no)->command != 'S'; cmd_no++)
      if (zone_get_cmd(g, zone, cmd_no)->command == 'M')
        zone_get_cmd(g, zone, cmd_no)->arg1 += (zone_get_cmd(g, zone, cmd_no)->arg1 >= found);

  /* Update shop keepers. */
  for (struct shop_data *shop : g->shops.all())
    if (shop_get_keeper(shop) != NOTHING && shop_get_keeper(shop) >= found)
      shop_set_keeper(shop, shop_get_keeper(shop) + 1);

  /* Update guild masters */
  for (struct guild_data *guild : g->guilds.all())
    if (guild_get_gm(guild) != NOTHING && guild_get_gm(guild) >= found)
      guild_set_gm(guild, guild_get_gm(guild) + 1);

  add_to_save_list(g, find_zone_by_thing(g, vnum)->number, SL_MOB);
  return found;
}

bool copy_mobile(GameWorld *g, struct char_data *to, struct char_data *from)
{
  free_mobile_strings(to);
  *to = *from;
  check_mobile_strings(g, from);
  copy_mobile_strings(to, from);
  return true;
}

static void extract_mobile_all(GameWorld *g, mob_vnum vnum)
{

  for (char_data *ch : each_char(g)) {
    if (char_get_mob_vnum(g, ch) == vnum)
      extract_char(g, ch);
  }
}

mob_rnum delete_mobile(GameWorld *g, mob_rnum refpt)
{
  IDXTYPE counter;
  int cmd_no;
  mob_vnum vnum;

  if (refpt < 0 || refpt > g->top_mob) {
    mud_log("SYSERR: GenOLC: delete_mobile: Invalid rnum %d.", refpt);
    return NOBODY;
  }

  vnum = g->mob_index[refpt].vnum;
  extract_mobile_all(g, vnum);

  for (counter = refpt; counter < g->top_mob; counter++) {
    g->mob_index[counter]    = g->mob_index[counter + 1];
    g->mob_proto[counter]    = g->mob_proto[counter + 1];
    char_set_nr(&g->mob_proto[counter], counter);
  }

  g->top_mob--;
  resize_array(g->mob_index, g->top_mob + 2, g->top_mob + 1);
  resize_array(g->mob_proto, g->top_mob + 2, g->top_mob + 1);

  add_to_save_list(g, find_zone_by_thing(g, vnum)->number, SL_MOB);

  /* Update live mobile rnums.  */
  for (char_data *live_mob : each_char(g))
    char_set_nr(live_mob, char_get_nr(live_mob) - ((char_get_nr(live_mob) >= refpt)));

  /* Update zone table.  */
  for (struct zone_data *zone : g->zones.all()) {
    bool changed = false;
    for (cmd_no = 0; zone_get_cmd(g, zone, cmd_no)->command != 'S'; cmd_no++) {
      if (zone_get_cmd(g, zone, cmd_no)->command == 'M' && zone_get_cmd(g, zone, cmd_no)->arg1 == refpt) {
        zone_get_cmd(g, zone, cmd_no)->command = '*';
        zone_get_cmd(g, zone, cmd_no)->arg1    = NOTHING;
        changed                             = true;
      }

      if (zone_get_cmd(g, zone, cmd_no)->command == 'M' && zone_get_cmd(g, zone, cmd_no)->arg1 > refpt) {
        zone_get_cmd(g, zone, cmd_no)->arg1 -= 1;
        changed = true;
      }
    }
    if (changed) {
      add_to_save_list(g, zone->number, SL_ZON);
    }
  }

  zone_vnum last_saved_zone = NOTHING;
  /* Update shop keepers.  */
  for (struct shop_data *shop : g->shops.all()) {
    zone_vnum zvnum = find_zone_by_thing(g, shop_get_vnum(shop))->number;
    /* Find the shop for this keeper and reset it's keeper to
       * -1 to keep the shop so it could be assigned to someone else */
    if (shop_get_keeper(shop) == refpt) {
      shop_set_keeper(shop, NOTHING);
      if (zvnum != last_saved_zone) {
        add_to_save_list(g, zvnum, SL_SHP);
        last_saved_zone = zvnum;
      }
    }
  }

  last_saved_zone = NOTHING;
  /* Update guild masters */
  for (struct guild_data *guild : g->guilds.all()) {
    /* Find the guild for this trainer and reset it's trainer to
       * -1 to keep the guild so it could be assigned to someone else */
    if (guild_get_gm(guild) == refpt) {
      guild_set_gm(guild, NOTHING);
      zone_vnum zvnum = find_zone_by_thing(g, guild_get_vnum(guild))->number;
      if (zvnum != last_saved_zone) {
        add_to_save_list(g, zvnum, SL_GLD);
        last_saved_zone = zvnum;
      }
    }
  }

  return refpt;
}

static bool copy_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (char_get_alias(f))
    char_set_alias(t, strdup(char_get_alias(f)));
  if (char_get_raw_title(f))
    char_set_raw_title(t, strdup(char_get_raw_title(f)));
  if (char_get_sdesc(f))
    char_set_sdesc(t, strdup(char_get_sdesc(f)));
  if (char_get_ldesc(f))
    char_set_ldesc(t, strdup(char_get_ldesc(f)));
  if (char_get_ddesc(f))
    char_set_ddesc(t, strdup(char_get_ddesc(f)));
  return true;
}

static bool update_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (char_get_alias(f))
    char_set_alias(t, char_get_alias(f));
  if (char_get_raw_title(f))
    char_set_raw_title(t, char_get_raw_title(f));
  if (char_get_sdesc(f))
    char_set_sdesc(t, char_get_sdesc(f));
  if (char_get_ldesc(f))
    char_set_ldesc(t, char_get_ldesc(f));
  if (char_get_ddesc(f))
    char_set_ddesc(t, char_get_ddesc(f));
  return true;
}

static bool free_mobile_strings(struct char_data *mob)
{
  if (char_get_alias(mob))
    free(char_get_alias(mob));
  if (char_get_raw_title(mob))
    free(char_get_raw_title(mob));
  if (char_get_sdesc(mob))
    free(char_get_sdesc(mob));
  if (char_get_ldesc(mob))
    free(char_get_ldesc(mob));
  if (char_get_ddesc(mob))
    free(char_get_ddesc(mob));
  return true;
}

/* Free a mobile structure that has been edited. Take care of existing mobiles 
 * and their mob_proto!  */
bool free_mobile(GameWorld *g, struct char_data *mob)
{
  mob_rnum i;

  if (mob == nullptr)
    return false;

  /* Non-prototyped mobile.  Also known as new mobiles.  */
  if ((i = char_get_nr(mob)) == NOBODY) {
    free_mobile_strings(mob);
    /* free script proto list */
    free_proto_script(mob, MOB_TRIGGER);
  } else { /* Prototyped mobile. */
    if (char_get_alias(mob) && char_get_alias(mob) != char_get_alias(&g->mob_proto[i]))
      free(char_get_alias(mob));
    if (char_get_raw_title(mob) && char_get_raw_title(mob) != char_get_raw_title(&g->mob_proto[i]))
      free(char_get_raw_title(mob));
    if (char_get_sdesc(mob) && char_get_sdesc(mob) != char_get_sdesc(&g->mob_proto[i]))
      free(char_get_sdesc(mob));
    if (char_get_ldesc(mob) && char_get_ldesc(mob) != char_get_ldesc(&g->mob_proto[i]))
      free(char_get_ldesc(mob));
    if (char_get_ddesc(mob) && char_get_ddesc(mob) != char_get_ddesc(&g->mob_proto[i]))
      free(char_get_ddesc(mob));
    /* free script proto list if it's not the prototype */
    if (char_get_proto_script(mob) && char_get_proto_script(mob) != char_get_proto_script(&g->mob_proto[i]))
      free_proto_script(mob, MOB_TRIGGER);
  }
  while (char_get_affected(mob))
    affect_remove(g, mob, char_get_affected(mob));

  /* free any assigned scripts */
  if (char_get_script(mob))
    extract_script(g, mob, MOB_TRIGGER);

  delete mob;
  return true;
}

bool save_mobiles(GameWorld *g, struct zone_data *zone_num)
{
  FILE *mobfd;
  room_vnum i;
  mob_rnum rmob;
  char mobfname[64], usedfname[64];
  struct atomic_file af;

  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  snprintf(mobfname, sizeof(mobfname), "%s%d.mob", MOB_PREFIX, zone_num->number);
  if ((mobfd = atomic_open(&af, mobfname, false)) == nullptr) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return false;
  }

  for (i = genolc_zone_bottom(g, zone_num); i <= zone_num->top; i++) {
    if ((rmob = real_mobile(g, i)) == NOBODY)
      continue;
    check_mobile_strings(g, &g->mob_proto[rmob]);
    if (!write_mobile_record(g, i, &g->mob_proto[rmob], mobfd))
      mud_log("SYSERR: GenOLC: Error writing mobile #%d.", i);
  }
  fputs("$\n", mobfd);
  if (!atomic_commit(&af))
    return false;
  snprintf(usedfname, sizeof(usedfname), "%s", mobfname);

  if (in_save_list(g, zone_num->number, SL_MOB)) {
    remove_from_save_list(g, zone_num->number, SL_MOB);
    create_world_index(g, zone_num->number, "mob");
    mud_log("GenOLC: save_mobiles: Saving mobiles '%s'", usedfname);
  }
  return true;
}

#if CONFIG_GENOLC_MOBPROG
bool write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  char wmmarg[MAX_STRING_LENGTH], wmmcom[MAX_STRING_LENGTH];
  MPROG_DATA *mob_prog;

  for (mob_prog = GET_MPROG(mob); mob_prog; mob_prog = mob_prog->next) {
    strlcpy(wmmarg, mob_prog->arglist, sizeof(wmmarg));
    strip_cr(wmmarg);
    strlcpy(wmmcom, mob_prog->comlist, sizeof(wmmcom));
    strip_cr(wmmcom);
    fprintf(fd, "%s %s~\n"
                "%s%c\n",
            medit_get_mprog_type(mob_prog), wmmarg,
            wmmcom, STRING_TERMINATOR);
    if (mob_prog->next == nullptr)
      fputs("|\n", fd);
  }
  return true;
}
#endif

static bool write_mobile_espec(GameWorld *g, mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  struct affected_type *aff;

  if (get_size(mob) != race_get_size(char_get_race(mob)))
    fprintf(fd, "Size: %d\n", get_size(mob));
  if (char_get_mob_specials_attack_type(mob) != 0)
    fprintf(fd, "BareHandAttack: %d\n", char_get_mob_specials_attack_type(mob));
  if (char_get_aff_abils_str(mob) != 0)
    fprintf(fd, "Str: %d\n", char_get_aff_abils_str(mob));
  if (char_get_aff_abils_dex(mob) != 0)
    fprintf(fd, "Dex: %d\n", char_get_aff_abils_dex(mob));
  if (char_get_aff_abils_intel(mob) != 0)
    fprintf(fd, "Int: %d\n", char_get_aff_abils_intel(mob));
  if (char_get_aff_abils_wis(mob) != 0)
    fprintf(fd, "Wis: %d\n", char_get_aff_abils_wis(mob));
  if (char_get_aff_abils_con(mob) != 0)
    fprintf(fd, "Con: %d\n", char_get_aff_abils_con(mob));
  if (char_get_aff_abils_cha(mob) != 0)
    fprintf(fd, "Cha: %d\n", char_get_aff_abils_cha(mob));
  if (g->mob_proto + real_mobile(g, mvnum) != mob) { /* Not saving a prototype */
    fprintf(fd, "Hit: %" PRId64 "\nMaxHit: %" PRId64 "\nMana: %" PRId64 "\nMaxMana: %" PRId64 "\nMoves: %" PRId64 "\nMaxMoves: %" PRId64 "\n",
            getCurPL(g, mob), getEffMaxPL(g, mob), (getCurKI(g, mob)), getMaxKI(g, mob),
            (getCurST(g, mob)), getMaxST(g, mob));
    for (aff = char_get_affected(mob); aff; aff = aff->next)
      if (aff->type)
        fprintf(fd, "Affect: %d %d %d %d %d %d\n", aff->type, aff->duration,
                aff->modifier, aff->location, (int)aff->bitvector, aff->specific);
  }
  fputs("E\n", fd);
  return true;
}

static bool write_mobile_record(GameWorld *g, mob_vnum mvnum, struct char_data *mob, FILE *fd)
{

  char ldesc[MAX_STRING_LENGTH], ddesc[MAX_STRING_LENGTH];
  char fbuf1[MAX_STRING_LENGTH], fbuf2[MAX_STRING_LENGTH];
  char fbuf3[MAX_STRING_LENGTH], fbuf4[MAX_STRING_LENGTH];
  char abuf1[MAX_STRING_LENGTH], abuf2[MAX_STRING_LENGTH];
  char abuf3[MAX_STRING_LENGTH], abuf4[MAX_STRING_LENGTH];

  strlcpy(ldesc, char_get_ldesc(mob), sizeof(ldesc));
  strip_cr(ldesc);
  strlcpy(ddesc, char_get_ddesc(mob), sizeof(ddesc));
  strip_cr(ddesc);

  fprintf(fd, "#%d\n"
              "%s%c\n"
              "%s%c\n"
              "%s%c\n"
              "%s%c\n",
          mvnum,
          char_get_alias(mob), STRING_TERMINATOR,
          char_get_sdesc(mob), STRING_TERMINATOR,
          ldesc, STRING_TERMINATOR,
          ddesc, STRING_TERMINATOR);

  sprintascii(fbuf1, char_get_act(mob)[0]);
  sprintascii(fbuf2, char_get_act(mob)[1]);
  sprintascii(fbuf3, char_get_act(mob)[2]);
  sprintascii(fbuf4, char_get_act(mob)[3]);
  sprintascii(abuf1, char_get_affected_by(mob)[0]);
  sprintascii(abuf2, char_get_affected_by(mob)[1]);
  sprintascii(abuf3, char_get_affected_by(mob)[2]);
  sprintascii(abuf4, char_get_affected_by(mob)[3]);

  fprintf(fd, "%s %s %s %s %s %s %s %s %d E\n"
              "%d %d %d %" PRId64 "d%" PRId64 "+%" PRId64 " %dd%d+%d\n",
          fbuf1, fbuf2, fbuf3, fbuf4,
          abuf1, abuf2, abuf3, abuf4,
          char_get_alignment(mob),
          char_get_race_level(mob), char_get_accuracy_mod(mob), 10 - (char_get_armor(mob) / 10),
          getCurPL(g, mob), (getCurKI(g, mob)), (getCurST(g, mob)), char_get_mob_specials_damnodice(mob), char_get_mob_specials_damsizedice(mob),
          0 /* damage mod: unused, still written to keep the file format */
  );
  fprintf(fd, "%d 0 %d %d\n"
              "%d %d %d\n",
          char_get_gold(mob), char_get_race(mob), char_get_class(mob),
          char_get_position(mob), char_get_mob_specials_default_pos(mob), char_get_sex(mob));

  if (!write_mobile_espec(g, mvnum, mob, fd))
    mud_log("SYSERR: GenOLC: Error writing E-specs for mobile #%d.", mvnum);

  script_save_to_disk(fd, mob, MOB_TRIGGER);

#if CONFIG_GENOLC_MOBPROG
  if (write_mobile_mobprog(mvnum, mob, fd) < 0)
    mud_log("SYSERR: GenOLC: Error writing MobProgs for mobile #%d.", mvnum);
#endif

  return true;
}

static void check_mobile_strings(GameWorld *g, struct char_data *mob)
{
  mob_vnum mvnum = g->mob_index[char_get_nr(mob)].vnum;
  check_mobile_string(g, mvnum, char_long_descr_slot(mob), "long description");
  check_mobile_string(g, mvnum, char_description_slot(mob), "detailed description");
  check_mobile_string(g, mvnum, char_name_slot(mob), "alias list");
  check_mobile_string(g, mvnum, char_short_descr_slot(mob), "short description");
}

static void check_mobile_string(GameWorld *g, mob_vnum i, char **string, const char *dscr)
{
  if (*string == nullptr || **string == '\0') {
    char smbuf[128];
    sprintf(smbuf, "GenOLC: Mob #%d has an invalid %s.", i, dscr);
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "%s", smbuf);
    if (*string)
      free(*string);
    *string = strdup("An undefined string.");
  }
}
