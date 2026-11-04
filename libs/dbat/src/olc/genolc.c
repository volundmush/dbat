/************************************************************************
 * Generic OLC Library - General / genolc.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/genolc.h"
#include "dbat/gengld.h"
#include "dbat/cedit.h"
#include "dbat/genmob.h"
#include "dbat/genobj.h"
#include "dbat/genshp.h"
#include "dbat/genwld.h"
#include "dbat/modify.h"
#include "dbat/genzon.h"
#include "dbat/oasis.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/game_world.h"

/* List of zones to be saved.  */

/* Structure defining all known save types.  */
static struct {
  int save_type;
  bool (*func)(GameWorld *g, IDXTYPE rnum);
  const char *message;
} save_types[] = {
    {SL_MOB, save_mobiles, "mobile"},
    {SL_OBJ, save_objects, "object"},
    {SL_SHP, save_shops,   "shop"  },
    {SL_WLD, save_rooms,   "room"  },
    {SL_ZON, save_zone,    "zone"  },
    {SL_CFG, save_config,  "config"},
    {SL_GLD, save_guilds,  "guild" },
    {SL_ACT, nullptr,      "social"},
    {SL_HLP, nullptr,      "help"  },
    {-1,     nullptr,      nullptr },
};

bool genolc_checkstring(struct descriptor_data *d, char *arg)
{
  smash_tilde(arg);
  return true;
}

const char *npc_msg_format_problem(const char *msg, bool allow_int)
{
  static char why[64];
  int ss = 0, ds = 0;

  for (const char *p = msg; *p; p++) {
    if (*p != '%')
      continue;

    p++;
    if (*p == 's')
      ss++;
    else if (*p == 'd' && allow_int) {
      if (ss == 0)
        return "%d before %s";
      ds++;
    } else if (*p == '\0')
      return "a '%' at the very end";
    else if (*p != '%') {
      snprintf(why, sizeof(why), "invalid format '%%%c'", *p);
      return why;
    }
  }

  if (ss > 1)
    return "more than one %s";
  if (ds > 1)
    return "more than one %d";
  return nullptr;
}

/* A numbered menu of names, `columns` to a line: "@g%2d@n) %-20.20s" then gap after each name.
   Numbering starts at first_number. */
void olc_list_columns(GameWorld *g, struct descriptor_data *d, const char *const *names, int count, int first_number, const char *gap, int columns)
{
  for (int i = 0; i < count; i++)
    write_to_output(g, d, "@g%2d@n) %-20.20s%s%s", i + first_number, names[i], gap, !((i + 1) % columns) ? "\r\n" : "");
}

char *str_udup(const char *txt)
{
  return strdup((txt && *txt) ? txt : "undefined");
}

/* Original use: to be called at shutdown time.  */
bool save_all(GameWorld *g)
{
  while (g->save_list) {
    if (g->save_list->type < 0 || g->save_list->type > SL_MAX) {
      switch (g->save_list->type) {
        case SL_ACT:
          mud_log("Actions not saved - can not autosave. Use 'aedit save'.");
          g->save_list = g->save_list->next; /* Fatal error, skip this one. */
          break;
        case SL_HLP:
          mud_log("Help not saved - can not autosave. Use 'hedit save'.");
          g->save_list = g->save_list->next; /* Fatal error, skip this one. */
          break;
        default:
          mud_log("SYSERR: GenOLC: Invalid save type %d in save list.\n", g->save_list->type);
          break;
      }
    } else if (!save_types[g->save_list->type].func(g, real_zone(g, g->save_list->zone)))
      g->save_list = g->save_list->next; /* Fatal error, skip this one. */
  }

  return true;
}

/* NOTE: This changes the buffer passed in.  */
void strip_cr(char *buffer)
{
  int rpos, wpos;

  if (buffer == nullptr)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++) {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\r');
  }
  buffer[wpos] = '\0';
}

void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{
  struct extra_descr_data *wpos;

  CREATE(*to, struct extra_descr_data, 1);
  wpos = *to;

  for (; from; from = from->next, wpos = wpos->next) {
    wpos->keyword     = str_udup(from->keyword);
    wpos->description = str_udup(from->description);
    if (from->next)
      CREATE(wpos->next, struct extra_descr_data, 1);
  }
}

void free_ex_descriptions(struct extra_descr_data *head)
{
  struct extra_descr_data *thised, *next_one;

  if (!head) {
    mud_log("free_ex_descriptions: NULL pointer or NULL data.");
    return;
  }

  for (thised = head; thised; thised = next_one) {
    next_one = thised->next;
    if (thised->keyword)
      free(thised->keyword);
    if (thised->description)
      free(thised->description);
    free(thised);
  }
}

bool remove_from_save_list(GameWorld *g, zone_vnum zone, int type)
{
  struct save_list_data *ritem, *temp;

  for (ritem = g->save_list; ritem; ritem = ritem->next)
    if (ritem->zone == zone && ritem->type == type)
      break;

  if (ritem == nullptr) {
    mud_log("SYSERR: remove_from_save_list: Saved item not found. (%d/%d)", zone, type);
    return false;
  }
  REMOVE_FROM_LIST(ritem, g->save_list, next, temp);
  free(ritem);
  return true;
}

bool add_to_save_list(GameWorld *g, zone_vnum zone, int type)
{
  struct save_list_data *nitem;
  zone_rnum rznum;

  if (type == SL_CFG)
    return false;

  rznum = real_zone(g, zone);
  if (rznum == NOWHERE || rznum > g->top_zone) {
    if (zone != AEDIT_PERMISSION && zone != HEDIT_PERMISSION) {
      mud_log("SYSERR: add_to_save_list: Invalid zone number passed. (%d => %d, 0-%d)", zone, rznum, g->top_zone);
      return false;
    }
  }

  for (nitem = g->save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return false;

  CREATE(nitem, struct save_list_data, 1);
  nitem->zone = zone;
  nitem->type = type;
  nitem->next = g->save_list;
  g->save_list   = nitem;
  return true;
}

bool in_save_list(GameWorld *g, zone_vnum zone, int type)
{
  struct save_list_data *nitem;

  for (nitem = g->save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return true;

  return false;
}

void free_save_list(GameWorld *g)
{
  struct save_list_data *sld, *next_sld;

  for (sld = g->save_list; sld; sld = next_sld) {
    next_sld = sld->next;
    free(sld);
  }
}

/* Used from do_show(), ideally.  */
ACMD(do_show_save_list)
{
  if (g->save_list == nullptr)
    send_to_char(g, ch, "All world files are up to date.\r\n");
  else {
    struct save_list_data *item;

    send_to_char(g, ch, "The following files need saving:\r\n");
    for (item = g->save_list; item; item = item->next) {
      if (item->type != SL_CFG)
        send_to_char(g, ch, " - %s data for zone %d.\r\n", save_types[item->type].message, item->zone);
      else
        send_to_char(g, ch, " - Game configuration data.\r\n");
    }
  }
}

room_vnum genolc_zonep_bottom(struct zone_data *zone)
{
  return zone->bot;
}

zone_vnum genolc_zone_bottom(GameWorld *g, zone_rnum rznum)
{
  return g->zones[rznum].bot;
}

int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  const char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1U << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}
