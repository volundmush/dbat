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


/* From db.c */
void init_mobile_skills(void);

static bool update_mobile_strings(struct char_data *t, struct char_data *f);


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
  if (mob == nullptr)
    return false;

  while (!char_affects(mob).empty())
    affect_remove(g, mob, &char_affects(mob).front());

  /* free any assigned scripts */
  if (char_get_script(mob))
    extract_script(g, mob);

  delete mob;
  return true;
}
