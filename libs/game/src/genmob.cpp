/************************************************************************
 * Generic OLC Library - Mobiles / genmob.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/db/shops.h"
#include "dbat/db/guilds.h"
#include "dbat/db/iterate.hpp"
#include "dbat/game/genmob.h"
#include "dbat/game/utils.h"
#include "dbat/game/db.h"
#include "dbat/game/genolc.h"
#include "dbat/game/shop.h"
#include "dbat/game/genzon.h"
#include "dbat/game/guild.h"
#include "dbat/game/dg_scripts.h"
#include "dbat/game/handler.h"
#include "dbat/game/dg_olc.h"
#include "dbat/game/class.h"
#include "dbat/game/races_plus.h"
#include "dbat/game/affect.h"

#include <string.h>

/* From db.c */
void init_mobile_skills(void);


int update_mobile_strings(struct char_data *t, struct char_data *f);
void check_mobile_strings(struct char_data *mob);
void check_mobile_string(mob_vnum i, char **string, const char *dscr);
int write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd);


#if CONFIG_GENOLC_MOBPROG
int write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd);
#endif

/* local functions */
void extract_mobile_all(mob_vnum vnum);

static void free_trig_proto_list(struct trig_proto_list *list)
{
  while (list) {
    auto next = list->next;
    free(list);
    list = next;
  }
}

static struct trig_proto_list *copy_trig_proto_list(const struct trig_proto_list *from)
{
  struct trig_proto_list *head = NULL, *tail = NULL;

  for (; from; from = from->next) {
    struct trig_proto_list *node;
    CREATE(node, struct trig_proto_list, 1);
    node->vnum = from->vnum;
    if (tail)
      tail->next = node;
    else
      head = node;
    tail = node;
  }

  return head;
}

static void mob_proto_free_strings(struct mob_proto_data *mob)
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
  mob->name = NULL;
  mob->title = NULL;
  mob->short_descr = NULL;
  mob->long_descr = NULL;
  mob->description = NULL;
}

void mob_proto_free(struct mob_proto_data *mob)
{
  if (!mob)
    return;
  mob_proto_free_strings(mob);
  free_trig_proto_list(mob->proto_script);
  free(mob);
}

void mob_proto_free_script(struct mob_proto_data *mob)
{
  if (!mob)
    return;
  free_trig_proto_list(mob->proto_script);
  mob->proto_script = NULL;
}

void mob_proto_copy_script_to_mobile(struct mob_proto_data *source, struct char_data *dest)
{
  if (!dest)
    return;
  if (dest->proto_script)
    free_proto_script(dest, MOB_TRIGGER);
  dest->proto_script = source ? copy_trig_proto_list(source->proto_script) : NULL;
}

int copy_mobile_to_proto(struct mob_proto_data *to, struct char_data *from)
{
  char *old_name = to->name;
  char *old_title = to->title;
  char *old_short_descr = to->short_descr;
  char *old_long_descr = to->long_descr;
  char *old_description = to->description;
  struct trig_proto_list *old_proto_script = to->proto_script;

  memset(to, 0, sizeof(*to));

  to->name = old_name;
  to->title = old_title;
  to->short_descr = old_short_descr;
  to->long_descr = old_long_descr;
  to->description = old_description;
  to->proto_script = old_proto_script;

  mob_proto_free_strings(to);
  free_trig_proto_list(to->proto_script);
  to->proto_script = NULL;

  to->vnum = from->vnum;
  to->size = from->size;
  to->sex = from->sex;
  to->race = from->race;
  to->chclass = from->chclass;
  to->alignment = from->alignment;
  to->weight = from->weight;
  to->height = from->height;
  to->level = from->level;
  to->race_level = from->race_level;
  to->level_adj = from->level_adj;
  to->gold = from->gold;
  to->exp = from->exp;
  to->basepl = from->basepl;
  to->baseki = from->baseki;
  to->basest = from->basest;
  to->armor = from->armor;
  to->real_abils = from->real_abils;
  to->mob_specials = from->mob_specials;
  to->position = from->position;
  to->speaking = from->speaking;
  memcpy(to->act, from->act, sizeof(to->act));
  memcpy(to->affected_by, from->affected_by, sizeof(to->affected_by));
  to->name = from->name ? strdup(from->name) : NULL;
  to->title = from->title ? strdup(from->title) : NULL;
  to->short_descr = from->short_descr ? strdup(from->short_descr) : NULL;
  to->long_descr = from->long_descr ? strdup(from->long_descr) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->proto_script = copy_trig_proto_list(from->proto_script);
  return TRUE;
}

int copy_mobile_from_proto(struct char_data *to, struct mob_proto_data *from)
{
  int32_t id = to->id;
  struct descriptor_data *desc = to->desc;
  struct char_data *next = to->next;
  struct char_data *next_affect = to->next_affect;

  free_mobile_strings(to);
  if (to->proto_script)
    free_proto_script(to, MOB_TRIGGER);

  memset(to, 0, sizeof(*to));

  to->id = id;
  to->desc = desc;
  to->next = next;
  to->next_affect = next_affect;

  to->vnum = from->vnum;
  to->size = from->size;
  to->sex = from->sex;
  to->race = from->race;
  to->chclass = from->chclass;
  to->alignment = from->alignment;
  to->weight = from->weight;
  to->height = from->height;
  to->level = from->level;
  to->race_level = from->race_level;
  to->level_adj = from->level_adj;
  to->gold = from->gold;
  to->exp = from->exp;
  to->basepl = from->basepl;
  to->baseki = from->baseki;
  to->basest = from->basest;
  to->armor = from->armor;
  to->real_abils = from->real_abils;
  to->mob_specials = from->mob_specials;
  to->position = from->position;
  to->speaking = from->speaking;
  memcpy(to->act, from->act, sizeof(to->act));
  memcpy(to->affected_by, from->affected_by, sizeof(to->affected_by));
  to->name = from->name ? strdup(from->name) : NULL;
  to->title = from->title ? strdup(from->title) : NULL;
  to->short_descr = from->short_descr ? strdup(from->short_descr) : NULL;
  to->long_descr = from->long_descr ? strdup(from->long_descr) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->proto_script = copy_trig_proto_list(from->proto_script);
  return TRUE;
}

int add_mobile(struct char_data *mob, mob_vnum vnum)
{
  int i, shop, guild, cmd_no;
  zone_rnum zone;
  struct char_data *live_mob;

  auto proto = mob_proto_by_id(vnum);

  if (proto) {
    /* Copy over the mobile and free() the old strings. */
    copy_mobile_to_proto(proto, mob);

    /* Now re-point all existing mobile strings to here. */
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (vnum == live_mob->vnum) {
        struct char_data temp = {};
        copy_mobile_from_proto(&temp, proto);
        update_mobile_strings(live_mob, &temp);
        free_mobile_strings(&temp);
        if (temp.proto_script)
          free_proto_script(&temp, MOB_TRIGGER);
      }

    add_to_save_list(virtual_zone_by_thing(vnum), SL_MOB);
    log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    return vnum;
  }

  struct mob_proto_data *new_proto = NULL;
  CREATE(new_proto, struct mob_proto_data, 1);
  copy_mobile_to_proto(new_proto, mob);

  mob_proto_put(vnum, new_proto);

  log("GenOLC: add_mobile: Added mobile %d.", vnum);

  add_to_save_list(virtual_zone_by_thing(vnum), SL_MOB);
  return vnum;
}

int copy_mobile(struct char_data *to, struct char_data *from)
{
  struct mob_proto_data tmp = {};

  copy_mobile_to_proto(&tmp, from);
  copy_mobile_from_proto(to, &tmp);
  mob_proto_free_strings(&tmp);
  free_trig_proto_list(tmp.proto_script);
  return TRUE;
}

void extract_mobile_all(mob_vnum vnum)
{
  struct char_data *next, *ch;

  for (ch = character_list; ch; ch = next) {
    next = ch->next;
    if (GET_MOB_VNUM(ch) == vnum)
      extract_char(ch);
  }
}

int delete_mobile(mob_vnum refpt)
{
  struct char_data *live_mob;
  int counter, cmd_no;
  mob_vnum vnum;
  zone_rnum zone;
  
  if(!mob_proto_by_id(refpt)) {
    log("GenOLC: delete_mobile: Attempted to delete non-existant mobile #%d.", refpt);
    return FALSE;
  }

  vnum = refpt;
  extract_mobile_all(vnum);

  auto vz = virtual_zone_by_thing(vnum);

  add_to_save_list(vz, SL_MOB);

  /* Update zone table.  */
  zone_iterate ([&](auto zone) {
    bool changed = FALSE;
    for (cmd_no = 0; zone->cmd[cmd_no].command != 'S'; cmd_no++) {
      if (zone->cmd[cmd_no].command == 'M' && zone->cmd[cmd_no].arg1 == vnum) {
        zone->cmd[cmd_no].command = '*';
        zone->cmd[cmd_no].arg1 = NOTHING;
        changed = true;
      }
    }
    if(changed) {
      add_to_save_list(zone->number, SL_ZON);
    }
  return true;
});

  zone_vnum last_saved_zone = NOTHING;
  /* Update shop keepers.  */
    shop_iterate ([&](auto shop) {
      zone_vnum zone = virtual_zone_by_thing(SHOP_NUM(shop));
      /* Find the shop for this keeper and reset it's keeper to
       * -1 to keep the shop so it could be assigned to someone else */
      if (SHOP_KEEPER(shop) == vnum) {
        SHOP_KEEPER(shop) = NOTHING;
        if(zone != last_saved_zone) {
          add_to_save_list(zone, SL_SHP);
          last_saved_zone = zone;
        }
      }
      return true;
    });
  
    last_saved_zone = NOTHING;
  /* Update guild masters */
    guild_iterate ([&](auto guild) {
      zone_vnum zone = virtual_zone_by_thing(guild->vnum);
      /* Find the guild for this trainer and reset it's trainer to
       * -1 to keep the guild so it could be assigned to someone else */
      if (GM_TRAINER(guild) == vnum) {
        GM_TRAINER(guild) = NOTHING;
        zone_vnum zone = virtual_zone_by_thing(guild->vnum);
        if(zone != last_saved_zone) {
          add_to_save_list(zone, SL_GLD);
          last_saved_zone = zone;
        }
      }
      return true;
    });

  return refpt;
}

int copy_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (f->name)
    t->name = strdup(f->name);
  if (f->voice)
    t->voice = strdup(f->voice);
  if (f->clan)
    t->clan = strdup(f->clan);
  if (f->title)
    t->title = strdup(f->title);
  if (f->short_descr)
    t->short_descr = strdup(f->short_descr);
  if (f->long_descr)
    t->long_descr = strdup(f->long_descr);
  if (f->description)
    t->description = strdup(f->description);
  return TRUE;
}

int update_mobile_strings(struct char_data *t, struct char_data *f)
{
  free_mobile_strings(t);
  copy_mobile_strings(t, f);
  return TRUE;
}

int free_mobile_strings(struct char_data *mob)
{
  if (mob->name)
    free(mob->name);
  if (mob->voice)
    free(mob->voice);
  if (mob->clan)
    free(mob->clan);
  if (mob->title)
    free(mob->title);
  if (mob->short_descr)
    free(mob->short_descr);
  if (mob->long_descr)
    free(mob->long_descr);
  if (mob->description)
    free(mob->description);
  return TRUE;
}


/* Free a mobile structure that has been edited. Take care of existing mobiles 
 * and their mob_proto!  */
int mobile_free_editor(struct char_data *mob)
{
  mob_rnum i;

  if (mob == NULL)
    return FALSE;
  
  free_mobile_strings(mob);
  if (mob->proto_script)
    free_proto_script(mob, MOB_TRIGGER);
  while (mob->affected)
    affect_remove(mob, mob->affected);

  /* free any assigned scripts */
  if (SCRIPT(mob))
    extract_script(mob, MOB_TRIGGER);

  free(mob);
  return TRUE;
}

int free_mobile(struct char_data *mob)
{
  return mobile_free_editor(mob);
}

int save_mobiles(struct zone_data *zone)
{
  FILE *mobfd;
  room_vnum i;
  mob_rnum rmob;
  int written;
  char mobfname[64], usedfname[64];

if(!zone) {
    log("SYSERR: GenOLC: save_mobiles: Invalid zone!");
    return FALSE;
  }

  snprintf(mobfname, sizeof(mobfname), "%s%d.new", MOB_PREFIX, zone->number);
  if ((mobfd = fopen(mobfname, "w")) == NULL) {
    mudlog(BRF, ADMLVL_GOD, TRUE, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return FALSE;
  }

  for (i = zone->bot; i <= zone->top; i++) {
    auto proto = mob_proto_by_id(i);
    if (!proto) continue;
    if (write_mobile_record(i, proto, mobfd) < 0)
      log("SYSERR: GenOLC: Error writing mobile #%d.", i);
  }
  fputs("$\n", mobfd);
  written = ftell(mobfd);
  fclose(mobfd);
  snprintf(usedfname, sizeof(usedfname), "%s%d.mob", MOB_PREFIX, zone->number);
  remove(usedfname);
  rename(mobfname, usedfname);
  
  if (in_save_list(zone->number, SL_MOB)) {
    remove_from_save_list(zone->number, SL_MOB);
    create_world_index(zone->number, "mob");
    log("GenOLC: save_mobiles: Saving mobiles '%s'", usedfname);
  }
  return written;
}

#if CONFIG_GENOLC_MOBPROG
int write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  char wmmarg[MAX_STRING_LENGTH], wmmcom[MAX_STRING_LENGTH];
  MPROG_DATA *mob_prog;

  for (mob_prog = GET_MPROG(mob); mob_prog; mob_prog = mob_prog->next) {
    wmmarg[MAX_STRING_LENGTH - 1] = '\0';
    wmmcom[MAX_STRING_LENGTH - 1] = '\0';
    strip_cr(strncpy(wmmarg, mob_prog->arglist, MAX_STRING_LENGTH - 1));
    strip_cr(strncpy(wmmcom, mob_prog->comlist, MAX_STRING_LENGTH - 1));
    fprintf(fd,	"%s %s~\n"
		"%s%c\n",
	medit_get_mprog_type(mob_prog), wmmarg,
	wmmcom, STRING_TERMINATOR
    );
    if (mob_prog->next == NULL)
      fputs("|\n", fd);
  }
  return TRUE;
}
#endif

int write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  struct affected_type *aff;
  int i;

  if (get_size(mob) != race_get_size(mob->race))
    fprintf(fd, "Size: %d\n", get_size(mob));
  if (GET_STR(mob) != 0)
    fprintf(fd, "Str: %d\n", GET_STR(mob));
  if (GET_DEX(mob) != 0)
    fprintf(fd, "Dex: %d\n", GET_DEX(mob));
  if (GET_INT(mob) != 0)
    fprintf(fd, "Int: %d\n", GET_INT(mob));
  if (GET_WIS(mob) != 0)
    fprintf(fd, "Wis: %d\n", GET_WIS(mob));
  if (GET_CON(mob) != 0)
    fprintf(fd, "Con: %d\n", GET_CON(mob));
  if (GET_CHA(mob) != 0)
    fprintf(fd, "Cha: %d\n", GET_CHA(mob));
  fputs("E\n", fd);
  return TRUE;
}


int write_mobile_record(mob_vnum mvnum, struct mob_proto_data *proto, FILE *fd)
{
  struct char_data temp = {};
  struct char_data *mob = &temp;
  char ldesc[MAX_STRING_LENGTH], ddesc[MAX_STRING_LENGTH];
  char fbuf1[MAX_STRING_LENGTH], fbuf2[MAX_STRING_LENGTH];
  char fbuf3[MAX_STRING_LENGTH], fbuf4[MAX_STRING_LENGTH];
  char abuf1[MAX_STRING_LENGTH], abuf2[MAX_STRING_LENGTH];
  char abuf3[MAX_STRING_LENGTH], abuf4[MAX_STRING_LENGTH];

  copy_mobile_from_proto(mob, proto);

  ldesc[MAX_STRING_LENGTH - 1] = '\0';
  ddesc[MAX_STRING_LENGTH - 1] = '\0';
  strip_cr(strncpy(ldesc, GET_LDESC(mob), MAX_STRING_LENGTH - 1));
  strip_cr(strncpy(ddesc, GET_DDESC(mob), MAX_STRING_LENGTH - 1));

  fprintf(fd,	"#%d\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n",
	mvnum,
	GET_ALIAS(mob), STRING_TERMINATOR,
	GET_SDESC(mob), STRING_TERMINATOR,
	ldesc, STRING_TERMINATOR,
	ddesc, STRING_TERMINATOR
  );

  sprintascii(fbuf1, MOB_FLAGS(mob)[0]);
  sprintascii(fbuf2, MOB_FLAGS(mob)[1]);
  sprintascii(fbuf3, MOB_FLAGS(mob)[2]);
  sprintascii(fbuf4, MOB_FLAGS(mob)[3]);
  sprintascii(abuf1, AFF_FLAGS(mob)[0]);
  sprintascii(abuf2, AFF_FLAGS(mob)[1]);
  sprintascii(abuf3, AFF_FLAGS(mob)[2]);
  sprintascii(abuf4, AFF_FLAGS(mob)[3]);

  fprintf(fd, "%s %s %s %s %s %s %s %s %d E\n"
              "%d %d %d %" I64T "d%" I64T "+%" I64T " %dd%d+%d\n",
                fbuf1, fbuf2, fbuf3, fbuf4,
                abuf1, abuf2, abuf3, abuf4,
		GET_ALIGNMENT(mob),
		GET_HITDICE(mob), 0, 10 - (GET_ARMOR(mob) / 10),
                0, (getCurKI(mob)), (getCurST(mob)), 0, 0,
		0
  );
  fprintf(fd, 	"%d 0 %d %d\n"
		"%d %d %d\n",
		GET_GOLD(mob), GET_RACE(mob), GET_CLASS(mob),
		GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob)
  );

  if (write_mobile_espec(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing E-specs for mobile #%d.", mvnum);

  mob_proto_script_save_to_disk(fd, proto);


#if CONFIG_GENOLC_MOBPROG
  if (write_mobile_mobprog(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing MobProgs for mobile #%d.", mvnum);
#endif

  free_mobile_strings(mob);
  if (mob->proto_script)
    free_proto_script(mob, MOB_TRIGGER);
  return TRUE;
}

void check_mobile_strings(struct char_data *mob)
{
  mob_vnum mvnum = mob->vnum;
  check_mobile_string(mvnum, &GET_LDESC(mob), "long description");
  check_mobile_string(mvnum, &GET_DDESC(mob), "detailed description");
  check_mobile_string(mvnum, &GET_ALIAS(mob), "alias list");
  check_mobile_string(mvnum, &GET_SDESC(mob), "short description");
}

void check_mobile_string(mob_vnum i, char **string, const char *dscr)
{
  if (*string == NULL || **string == '\0') {
    char smbuf[128];
    sprintf(smbuf, "GenOLC: Mob #%d has an invalid %s.", i, dscr);
    mudlog(BRF, ADMLVL_GOD, TRUE, smbuf);
    if (*string)
      free(*string);
    *string = strdup("An undefined string.");
  }
}
