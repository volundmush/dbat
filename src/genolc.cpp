/************************************************************************
 * Generic OLC Library - General / genolc.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#define __GENOLC_C__

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

/* List of zones to be saved.  */
std::list<struct save_list_data> save_list;

/* Structure defining all known save types.  */
struct {
    int save_type;

    int (*func)(IDXTYPE rnum);

    const char *message;
} save_types[] = {
        {SL_MOB, save_mobiles, "mobile"},
        {SL_OBJ, save_objects, "object"},
        {SL_SHP, save_shops,   "shop"},
        {SL_WLD, save_rooms,   "room"},
        {SL_ZON, save_zone,    "zone"},
        {SL_CFG, save_config,  "config"},
        {SL_GLD, save_guilds,  "guild"},
        {SL_ACT, nullptr,      "social"},
        {SL_HLP, nullptr,      "help"},
        {-1,     nullptr,      nullptr},
};

int genolc_checkstring(struct descriptor_data *d, char *arg) {
    smash_tilde(arg);
    return true;
}

char *str_udup(const char *txt) {
    return strdup((txt && *txt) ? txt : "undefined");
}

/* Original use: to be called at shutdown time.  */
void save_all() {
    saveAll = true;
}

/* NOTE: This changes the buffer passed in.  */
void strip_cr(char *buffer) {
    int rpos, wpos;

    if (buffer == nullptr)
        return;

    for (rpos = 0, wpos = 0; buffer[rpos]; rpos++) {
        buffer[wpos] = buffer[rpos];
        wpos += (buffer[rpos] != '\r');
    }
    buffer[wpos] = '\0';
}


int remove_from_save_list(zone_vnum zone, int type) {
    int counter = 0;
    auto check = [&](save_list_data &d) {if(d.zone == zone && d.type == type) {counter++; return true;}};

    save_list.erase(std::remove_if(save_list.begin(), save_list.end(), check), save_list.end());
    return counter;
}

int add_to_save_list(zone_vnum zone, int type) {
    if(zone == HEDIT_PERMISSION || zone == NOWHERE || zone == AEDIT_PERMISSION) {
        return true;
    }
    return true;
}

int in_save_list(zone_vnum zone, int type) {

    for (auto &i : save_list)
        if (i.zone == zone && i.type == type)
            return true;
    return false;
}

/* Used from do_show(), ideally.  */
ACMD(do_show_save_list) {
    if (save_list.empty())
        ch->sendf("All world files are up to date.\r\n");
    else {
        ch->sendf("The following files need saving:\r\n");
        for (auto &i : save_list) {
            if (i.type != SL_CFG)
                ch->sendf(" - %s data for zone %d.\r\n", save_types[i.type].message, i.zone);
            else
                ch->sendf(" - Game configuration data.\r\n");
        }
    }
}

room_vnum genolc_zonep_bottom(struct zone_data *zone) {
    return zone->bot;
}

zone_vnum genolc_zone_bottom(zone_rnum rznum) {
    return zone_table[rznum].bot;
}

int sprintascii(char *out, bitvector_t bits) {
    int i, j = 0;
    /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
    char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

    for (i = 0; flags[i] != '\0'; i++)
        if (bits & (1 << i))
            out[j++] = flags[i];

    if (j == 0) /* Didn't write anything. */
        out[j++] = '0';

    /* NUL terminate the output string. */
    out[j++] = '\0';
    return j;
}

