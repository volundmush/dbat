/************************************************************************
 * Generic OLC Library - Mobiles / genmob.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/genmob.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/genolc.h"
#include "dbat/genzon.h"
#include "dbat/dg_scripts.h"
#include "dbat/handler.h"
#include "dbat/dg_olc.h"
#include "dbat/class.h"
#include "dbat/affect.h"
#include "dbat/atomicfile.h"

static bool free_mobile_strings(struct char_data *mob);
static bool write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd);

/* From db.c */
void init_mobile_skills(void);

static bool update_mobile_strings(struct char_data *t, struct char_data *f);
static void check_mobile_strings(struct char_data *mob);
static void check_mobile_string(mob_vnum i, char **string, const char *dscr);
static bool write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd);
static bool copy_mobile_strings(struct char_data *t, struct char_data *f);

#if CONFIG_GENOLC_MOBPROG
bool write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd);
#endif

/* local functions */
static void extract_mobile_all(mob_vnum vnum);

mob_rnum add_mobile(struct char_data *mob, mob_vnum vnum)
{
  mob_rnum rnum, i, found = false;
  shop_rnum shop;
  guild_rnum guild;
  int cmd_no;
  zone_rnum zone;
  struct char_data *live_mob;

  if ((rnum = real_mobile(vnum)) != NOBODY) {
    /* Copy over the mobile and free() the old strings. */
    copy_mobile(&mob_proto[rnum], mob);

    /* Now re-point all existing mobile strings to here. */
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (rnum == live_mob->nr)
        update_mobile_strings(live_mob, &mob_proto[rnum]);

    add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
    mud_log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    return rnum;
  }

  RECREATE(mob_proto, struct char_data, top_of_mobt + 2);
  RECREATE(mob_index, struct index_data, top_of_mobt + 2);
  top_of_mobt++;

  for (i = top_of_mobt; i > 0; i--) {
    if (vnum > mob_index[i - 1].vnum) {
      mob_proto[i]    = *mob;
      mob_proto[i].nr = i;
      copy_mobile_strings(mob_proto + i, mob);
      mob_index[i].vnum   = vnum;
      mob_index[i].number = 0;
      mob_index[i].func   = 0;
      found               = i;
      break;
    }
    mob_index[i] = mob_index[i - 1];
    mob_proto[i] = mob_proto[i - 1];
    mob_proto[i].nr++;
  }
  if (!found) {
    mob_proto[0]    = *mob;
    mob_proto[0].nr = 0;
    copy_mobile_strings(&mob_proto[0], mob);
    mob_index[0].vnum   = vnum;
    mob_index[0].number = 0;
    mob_index[0].func   = 0;
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
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    char_set_nr(live_mob, char_get_nr(live_mob) + ((char_get_nr(live_mob) != NOTHING && char_get_nr(live_mob) >= found)));

  /* Update zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; zone_get_cmd(zone, cmd_no)->command != 'S'; cmd_no++)
      if (zone_get_cmd(zone, cmd_no)->command == 'M')
        zone_get_cmd(zone, cmd_no)->arg1 += (zone_get_cmd(zone, cmd_no)->arg1 >= found);

  /* Update shop keepers. */
  if (shop_index)
    for (shop = 0; shop <= top_shop; shop++)
      if (rshop_get_keeper(shop) != NOTHING && rshop_get_keeper(shop) >= found)
        rshop_set_keeper(shop, rshop_get_keeper(shop) + 1);

  /* Update guild masters */
  if (guild_index)
    for (guild = 0; guild <= top_guild; guild++)
      if (rguild_get_gm(guild) != NOTHING && rguild_get_gm(guild) >= found)
        rguild_set_gm(guild, rguild_get_gm(guild) + 1);

  add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
  return found;
}

bool copy_mobile(struct char_data *to, struct char_data *from)
{
  free_mobile_strings(to);
  *to = *from;
  check_mobile_strings(from);
  copy_mobile_strings(to, from);
  return true;
}

static void extract_mobile_all(mob_vnum vnum)
{
  struct char_data *next, *ch;

  for (ch = character_list; ch; ch = next) {
    next = ch->next;
    if (char_get_mob_vnum(ch) == vnum)
      extract_char(ch);
  }
}

mob_rnum delete_mobile(mob_rnum refpt)
{
  struct char_data *live_mob;
  IDXTYPE counter;
  int cmd_no;
  mob_vnum vnum;
  zone_rnum zone;

  if (refpt < 0 || refpt > top_of_mobt) {
    mud_log("SYSERR: GenOLC: delete_mobile: Invalid rnum %d.", refpt);
    return NOBODY;
  }

  vnum = mob_index[refpt].vnum;
  extract_mobile_all(vnum);

  for (counter = refpt; counter < top_of_mobt; counter++) {
    mob_index[counter]    = mob_index[counter + 1];
    mob_proto[counter]    = mob_proto[counter + 1];
    mob_proto[counter].nr = counter;
  }

  top_of_mobt--;
  RECREATE(mob_index, struct index_data, top_of_mobt + 1);
  RECREATE(mob_proto, struct char_data, top_of_mobt + 1);

  add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);

  /* Update live mobile rnums.  */
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    char_set_nr(live_mob, char_get_nr(live_mob) - ((char_get_nr(live_mob) >= refpt)));

  /* Update zone table.  */
  for (zone = 0; zone <= top_of_zone_table; zone++) {
    bool changed = false;
    for (cmd_no = 0; zone_get_cmd(zone, cmd_no)->command != 'S'; cmd_no++) {
      if (zone_get_cmd(zone, cmd_no)->command == 'M' && zone_get_cmd(zone, cmd_no)->arg1 == refpt) {
        zone_get_cmd(zone, cmd_no)->command = '*';
        zone_get_cmd(zone, cmd_no)->arg1    = NOTHING;
        changed                             = true;
      }

      if (zone_get_cmd(zone, cmd_no)->command == 'M' && zone_get_cmd(zone, cmd_no)->arg1 > refpt) {
        zone_get_cmd(zone, cmd_no)->arg1 -= 1;
        changed = true;
      }
    }
    if (changed) {
      add_to_save_list(zone_table[zone].number, SL_ZON);
    }
  }

  zone_vnum last_saved_zone = NOTHING;
  /* Update shop keepers.  */
  if (shop_index)
    for (counter = 0; counter <= top_shop; counter++) {
      zone_vnum zvnum = zone_table[real_zone_by_thing(counter)].number;
      /* Find the shop for this keeper and reset it's keeper to
       * -1 to keep the shop so it could be assigned to someone else */
      if (rshop_get_keeper(counter) == refpt) {
        rshop_set_keeper(counter, NOTHING);
        if (zvnum != last_saved_zone) {
          add_to_save_list(zvnum, SL_SHP);
          last_saved_zone = zvnum;
        }
      }
    }

  last_saved_zone = NOTHING;
  /* Update guild masters */
  if (guild_index)
    for (counter = 0; counter <= top_guild; counter++) {
      /* Find the guild for this trainer and reset it's trainer to
       * -1 to keep the guild so it could be assigned to someone else */
      if (rguild_get_gm(counter) == refpt) {
        rguild_set_gm(counter, NOTHING);
        zone_vnum zvnum = zone_table[real_zone_by_thing(counter)].number;
        if (zvnum != last_saved_zone) {
          add_to_save_list(zvnum, SL_GLD);
          last_saved_zone = zvnum;
        }
      }
    }

  return refpt;
}

static bool copy_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (f->name)
    t->name = strdup(f->name);
  if (f->title)
    t->title = strdup(f->title);
  if (f->short_descr)
    t->short_descr = strdup(f->short_descr);
  if (f->long_descr)
    t->long_descr = strdup(f->long_descr);
  if (f->description)
    t->description = strdup(f->description);
  return true;
}

static bool update_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (f->name)
    t->name = f->name;
  if (f->title)
    t->title = f->title;
  if (f->short_descr)
    t->short_descr = f->short_descr;
  if (f->long_descr)
    t->long_descr = f->long_descr;
  if (f->description)
    t->description = f->description;
  return true;
}

static bool free_mobile_strings(struct char_data *mob)
{
  if (mob->name)
    free(mob->name);
  if (mob->title)
    free(mob->title);
  if (mob->short_descr)
    free(mob->short_descr);
  if (mob->long_descr)
    free(mob->long_descr);
  if (mob->description)
    free(mob->description);
  return true;
}

/* Free a mobile structure that has been edited. Take care of existing mobiles 
 * and their mob_proto!  */
bool free_mobile(struct char_data *mob)
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
    if (mob->name && mob->name != mob_proto[i].name)
      free(mob->name);
    if (mob->title && mob->title != mob_proto[i].title)
      free(mob->title);
    if (mob->short_descr && mob->short_descr != mob_proto[i].short_descr)
      free(mob->short_descr);
    if (mob->long_descr && mob->long_descr != mob_proto[i].long_descr)
      free(mob->long_descr);
    if (mob->description && mob->description != mob_proto[i].description)
      free(mob->description);
    /* free script proto list if it's not the prototype */
    if (mob->proto_script && mob->proto_script != mob_proto[i].proto_script)
      free_proto_script(mob, MOB_TRIGGER);
  }
  while (mob->affected)
    affect_remove(mob, mob->affected);

  /* free any assigned scripts */
  if (mob->script)
    extract_script(mob, MOB_TRIGGER);

  free(mob);
  return true;
}

bool save_mobiles(zone_rnum zone_num)
{
  FILE *mobfd;
  room_vnum i;
  mob_rnum rmob;
  char mobfname[64], usedfname[64];
  struct atomic_file af;

  if (zone_num < 0 || zone_num > top_of_zone_table) {
    mud_log("SYSERR: GenOLC: save_mobiles: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
    return false;
  }

  snprintf(mobfname, sizeof(mobfname), "%s%d.mob", MOB_PREFIX, zone_table[zone_num].number);
  if ((mobfd = atomic_open(&af, mobfname, false)) == nullptr) {
    mudlog(LOG_BRF, ADMLVL_GOD, true, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return false;
  }

  for (i = genolc_zone_bottom(zone_num); i <= zone_table[zone_num].top; i++) {
    if ((rmob = real_mobile(i)) == NOBODY)
      continue;
    check_mobile_strings(&mob_proto[rmob]);
    if (!write_mobile_record(i, &mob_proto[rmob], mobfd))
      mud_log("SYSERR: GenOLC: Error writing mobile #%d.", i);
  }
  fputs("$\n", mobfd);
  if (!atomic_commit(&af))
    return false;
  snprintf(usedfname, sizeof(usedfname), "%s", mobfname);

  if (in_save_list(zone_table[zone_num].number, SL_MOB)) {
    remove_from_save_list(zone_table[zone_num].number, SL_MOB);
    create_world_index(zone_table[zone_num].number, "mob");
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

static bool write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  struct affected_type *aff;

  if (get_size(mob) != race_get_size(mob->race))
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
  if (mob_proto + real_mobile(mvnum) != mob) { /* Not saving a prototype */
    fprintf(fd, "Hit: %" PRId64 "\nMaxHit: %" PRId64 "\nMana: %" PRId64 "\nMaxMana: %" PRId64 "\nMoves: %" PRId64 "\nMaxMoves: %" PRId64 "\n",
            getCurPL(mob), getEffMaxPL(mob), (getCurKI(mob)), getMaxKI(mob),
            (getCurST(mob)), getMaxST(mob));
    for (aff = mob->affected; aff; aff = aff->next)
      if (aff->type)
        fprintf(fd, "Affect: %d %d %d %d %d %d\n", aff->type, aff->duration,
                aff->modifier, aff->location, (int)aff->bitvector, aff->specific);
  }
  fputs("E\n", fd);
  return true;
}

static bool write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd)
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
          getCurPL(mob), (getCurKI(mob)), (getCurST(mob)), char_get_mob_specials_damnodice(mob), char_get_mob_specials_damsizedice(mob),
          0 /* damage mod: unused, still written to keep the file format */
  );
  fprintf(fd, "%d 0 %d %d\n"
              "%d %d %d\n",
          char_get_gold(mob), char_get_race(mob), char_get_class(mob),
          char_get_position(mob), char_get_mob_specials_default_pos(mob), char_get_sex(mob));

  if (!write_mobile_espec(mvnum, mob, fd))
    mud_log("SYSERR: GenOLC: Error writing E-specs for mobile #%d.", mvnum);

  script_save_to_disk(fd, mob, MOB_TRIGGER);

#if CONFIG_GENOLC_MOBPROG
  if (write_mobile_mobprog(mvnum, mob, fd) < 0)
    mud_log("SYSERR: GenOLC: Error writing MobProgs for mobile #%d.", mvnum);
#endif

  return true;
}

static void check_mobile_strings(struct char_data *mob)
{
  mob_vnum mvnum = mob_index[mob->nr].vnum;
  check_mobile_string(mvnum, &((mob)->long_descr), "long description");
  check_mobile_string(mvnum, &((mob)->description), "detailed description");
  check_mobile_string(mvnum, &((mob)->name), "alias list");
  check_mobile_string(mvnum, &((mob)->short_descr), "short description");
}

static void check_mobile_string(mob_vnum i, char **string, const char *dscr)
{
  if (*string == nullptr || **string == '\0') {
    char smbuf[128];
    sprintf(smbuf, "GenOLC: Mob #%d has an invalid %s.", i, dscr);
    mudlog(LOG_BRF, ADMLVL_GOD, true, "%s", smbuf);
    if (*string)
      free(*string);
    *string = strdup("An undefined string.");
  }
}
