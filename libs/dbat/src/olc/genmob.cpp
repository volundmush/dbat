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

static bool write_mobile_record(GameWorld *g, mob_vnum mvnum, struct char_data *mob, FILE *fd);

/* From db.c */
void init_mobile_skills(void);

static bool update_mobile_strings(struct char_data *t, struct char_data *f);
static void check_mobile_strings(GameWorld *g, struct char_data *mob);
static void check_mobile_string(GameWorld *g, mob_vnum i, std::string *string, const char *dscr);
static bool write_mobile_espec(GameWorld *g, mob_vnum mvnum, struct char_data *mob, FILE *fd);

#if CONFIG_GENOLC_MOBPROG
bool write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd);
#endif

/* local functions */
static void extract_mobile_all(GameWorld *g, mob_vnum vnum);

mob_vnum add_mobile(GameWorld *g, struct char_data *mob, mob_vnum vnum)
{
  mob_vnum rnum, found = false;

  if (mob_proto_of(g, rnum = vnum)) {
    /* Copy over the mobile. */
    copy_mobile(g, mob_proto_of(g, rnum), mob);
    char_set_nr(mob_proto_of(g, rnum), rnum); /* medit's copy option can bring another mob's number */

    /* Now give all existing mobiles its strings. */
    for (char_data *live_mob : each_char(g))
      if (rnum == char_get_nr(live_mob))
        update_mobile_strings(live_mob, mob_proto_of(g, rnum));

    add_to_save_list(g, find_zone_by_thing(g, vnum)->number, SL_MOB);
    mud_log("GenOLC: add_mobile: Updated existing mobile #{:d}.", vnum);
    return rnum;
  }

  struct char_data *proto  = g->mob_proto.emplace(vnum);
  struct index_data *index = g->mob_index.emplace(vnum);
  *proto                   = *mob;
  char_set_nr(proto, vnum);
  index->vnum   = vnum;
  index->number = 0;
  index->func   = nullptr;
  found         = vnum;

  mud_log("GenOLC: add_mobile: Added mobile {:d} at index #{:d}.", vnum, found);

#if CONFIG_GENOLC_MOBPROG
  GET_MPROG(OLC_MOB(d))      = OLC_MPROGL(d);
  GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
  while (OLC_MPROGL(d)) {
    GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
    OLC_MPROGL(d) = OLC_MPROGL(d)->next;
  }
#endif

  add_to_save_list(g, find_zone_by_thing(g, vnum)->number, SL_MOB);
  return found;
}

bool copy_mobile(GameWorld *g, struct char_data *to, struct char_data *from)
{
  check_mobile_strings(g, from);
  *to = *from;
  return true;
}

static void extract_mobile_all(GameWorld *g, mob_vnum vnum)
{

  for (char_data *ch : each_char(g)) {
    if (char_get_mob_vnum(g, ch) == vnum)
      extract_char(g, ch);
  }
}

mob_vnum delete_mobile(GameWorld *g, mob_vnum refpt)
{
  int cmd_no;
  mob_vnum vnum;

  if (refpt == NOBODY || !mob_index_of(g, refpt)) {
    mud_log("SYSERR: GenOLC: delete_mobile: Invalid rnum {:d}.", refpt);
    return NOBODY;
  }

  vnum = refpt;
  extract_mobile_all(g, vnum);

  add_to_save_list(g, find_zone_by_thing(g, vnum)->number, SL_MOB);

  /* Update zone table.  */
  for (struct zone_data *zone : g->zones.all()) {
    bool changed = false;
    for (cmd_no = 0; zone_get_cmd(g, zone, cmd_no)->command != 'S'; cmd_no++) {
      if (zone_get_cmd(g, zone, cmd_no)->command == 'M' && zone_get_cmd(g, zone, cmd_no)->arg1 == refpt) {
        zone_get_cmd(g, zone, cmd_no)->command = '*';
        zone_get_cmd(g, zone, cmd_no)->arg1    = NOTHING;
        changed                             = true;
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

  /* Copies still being extracted own everything they hold, so the prototype can go now. */
  g->mob_proto.take(refpt);
  g->mob_index.take(refpt);
  return refpt;
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

/* Free a mobile structure that has been edited. Take care of existing mobiles 
 * and their mob_proto!  */
bool free_mobile(GameWorld *g, struct char_data *mob)
{
  mob_vnum i;

  if (mob == nullptr)
    return false;

  /* Non-prototyped mobile.  Also known as new mobiles.  */
  if ((i = char_get_nr(mob)) == NOBODY) {
    /* free script proto list */
    free_proto_script(mob, MOB_TRIGGER);
  } else { /* Prototyped mobile. */
    /* free script proto list if it's not the prototype */
    if (char_get_proto_script(mob) && char_get_proto_script(mob) != char_get_proto_script(mob_proto_of(g, i)))
      free_proto_script(mob, MOB_TRIGGER);
  }
  while (!char_affects(mob).empty())
    affect_remove(g, mob, &char_affects(mob).front());

  /* free any assigned scripts */
  if (char_get_script(mob))
    extract_script(g, mob, MOB_TRIGGER);

  delete mob;
  return true;
}

bool save_mobiles(GameWorld *g, struct zone_data *zone_num)
{
  FILE *mobfd;
  mob_vnum rmob;
  std::string mobfname;
  std::string usedfname;
  struct atomic_file af;

  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  mobfname = str_format("{}{:d}.mob", MOB_PREFIX, zone_num->number);
  if ((mobfd = atomic_open(&af, mobfname.c_str(), false)) == nullptr) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return false;
  }

  for (mob_vnum i : g->mob_proto.vnums_in(genolc_zone_bottom(g, zone_num), zone_num->top)) {
    if (!mob_proto_of(g, rmob = i))
      continue;
    check_mobile_strings(g, mob_proto_of(g, rmob));
    if (!write_mobile_record(g, i, mob_proto_of(g, rmob), mobfd))
      mud_log("SYSERR: GenOLC: Error writing mobile #{:d}.", i);
  }
  fputs("$\n", mobfd);
  if (!atomic_commit(&af))
    return false;
  usedfname = mobfname;

  if (in_save_list(g, zone_num->number, SL_MOB)) {
    remove_from_save_list(g, zone_num->number, SL_MOB);
    create_world_index(g, zone_num->number, "mob");
    mud_log("GenOLC: save_mobiles: Saving mobiles '{}'", usedfname);
  }
  return true;
}

#if CONFIG_GENOLC_MOBPROG
bool write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  std::string wmmarg;
  std::string wmmcom;
  MPROG_DATA *mob_prog;

  for (mob_prog = GET_MPROG(mob); mob_prog; mob_prog = mob_prog->next) {
    wmmarg = mob_prog->arglist;
    strip_cr(wmmarg);
    wmmcom = mob_prog->comlist;
    strip_cr(wmmcom);
    fprintf(fd, "%s %s~\n"
                "%s%c\n",
            medit_get_mprog_type(mob_prog), wmmarg.c_str(),
            wmmcom.c_str(), STRING_TERMINATOR);
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
  if (mob_proto_of(g, mvnum) != mob) { /* Not saving a prototype */
    fprintf(fd, "Hit: %" PRId64 "\nMaxHit: %" PRId64 "\nMana: %" PRId64 "\nMaxMana: %" PRId64 "\nMoves: %" PRId64 "\nMaxMoves: %" PRId64 "\n",
            getCurPL(g, mob), getEffMaxPL(g, mob), (getCurKI(g, mob)), getMaxKI(g, mob),
            (getCurST(g, mob)), getMaxST(g, mob));
    for (auto aff_it = char_affects(mob).begin(); aff_it != char_affects(mob).end() && (aff = &*aff_it); ++aff_it)
      if (aff->type)
        fprintf(fd, "Affect: %d %d %d %d %d %d\n", aff->type, aff->duration,
                aff->modifier, aff->location, (int)aff->bitvector, aff->specific);
  }
  fputs("E\n", fd);
  return true;
}

static bool write_mobile_record(GameWorld *g, mob_vnum mvnum, struct char_data *mob, FILE *fd)
{

  std::string ldesc;
  std::string ddesc;
  char fbuf1[MAX_STRING_LENGTH], fbuf2[MAX_STRING_LENGTH];
  char fbuf3[MAX_STRING_LENGTH], fbuf4[MAX_STRING_LENGTH];
  char abuf1[MAX_STRING_LENGTH], abuf2[MAX_STRING_LENGTH];
  char abuf3[MAX_STRING_LENGTH], abuf4[MAX_STRING_LENGTH];

  ldesc = char_get_ldesc(mob);
  strip_cr(ldesc);
  ddesc = char_get_ddesc(mob);
  strip_cr(ddesc);

  fprintf(fd, "#%d\n"
              "%s%c\n"
              "%s%c\n"
              "%s%c\n"
              "%s%c\n",
          mvnum,
          char_get_alias(mob), STRING_TERMINATOR,
          char_get_sdesc(mob), STRING_TERMINATOR,
          ldesc.c_str(), STRING_TERMINATOR,
          ddesc.c_str(), STRING_TERMINATOR);

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
    mud_log("SYSERR: GenOLC: Error writing E-specs for mobile #{:d}.", mvnum);

  script_save_to_disk(fd, mob, MOB_TRIGGER);

#if CONFIG_GENOLC_MOBPROG
  if (write_mobile_mobprog(mvnum, mob, fd) < 0)
    mud_log("SYSERR: GenOLC: Error writing MobProgs for mobile #{:d}.", mvnum);
#endif

  return true;
}

static void check_mobile_strings(GameWorld *g, struct char_data *mob)
{
  mob_vnum mvnum = mob_index_of(g, char_get_nr(mob))->vnum;
  check_mobile_string(g, mvnum, char_long_descr_slot(mob), "long description");
  check_mobile_string(g, mvnum, char_description_slot(mob), "detailed description");
  check_mobile_string(g, mvnum, char_name_slot(mob), "alias list");
  check_mobile_string(g, mvnum, char_short_descr_slot(mob), "short description");
}

static void check_mobile_string(GameWorld *g, mob_vnum i, std::string *string, const char *dscr)
{
  if (string->empty()) {
    std::string smbuf = str_format("GenOLC: Mob #{:d} has an invalid {}.", i, dscr);
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, smbuf);
    *string = "An undefined string.";
  }
}
