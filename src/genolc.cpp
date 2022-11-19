/************************************************************************
 * Generic OLC Library - General / genolc.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#define __GENOLC_C__

#include "genolc.h"
#include "gengld.h"
#include "cedit.h"
#include "genmob.h"
#include "genobj.h"
#include "genshp.h"
#include "genwld.h"
#include "modify.h"
#include "genzon.h"
#include "oasis.h"
#include "utils.h"
#include "comm.h"

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
int save_all() {
    for (auto &s : save_list) {
        if (s.type < 0 || s.type > SL_MAX) {
            switch (s.type) {
                case SL_ACT:
                    log("Actions not saved - can not autosave. Use 'aedit save'.");
                    break;
                case SL_HLP:
                    log("Help not saved - can not autosave. Use 'hedit save'.");
                    break;
                default:
                    log("SYSERR: GenOLC: Invalid save type %d in save list.\n", s.type);
                    break;
            }
        } else if ((*save_types[s.type].func)(real_zone(s.zone)) < 0) {}
    }
    save_list.clear();
    return true;
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

void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from) {
    struct extra_descr_data *wpos;

    CREATE(*to, struct extra_descr_data, 1);
    wpos = *to;

    for (; from; from = from->next, wpos = wpos->next) {
        wpos->keyword = str_udup(from->keyword);
        wpos->description = str_udup(from->description);
        if (from->next)
            CREATE(wpos->next, struct extra_descr_data, 1);
    }
}

void free_ex_descriptions(struct extra_descr_data *head) {
    struct extra_descr_data *thised, *next_one;

    if (!head) {
        log("free_ex_descriptions: nullptr pointer or nullptr data.");
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

int remove_from_save_list(zone_vnum zone, int type) {
    int counter = 0;
    auto check = [&](save_list_data &d) {if(d.zone == zone && d.type == type) {counter++; return true;}};

    std::remove_if(save_list.begin(), save_list.end(), check);
    return counter;
}

int add_to_save_list(zone_vnum zone, int type) {
    if (type == SL_CFG)
        return false;

    if (!zone_table.count(zone)) {
        if (zone != AEDIT_PERMISSION && zone != HEDIT_PERMISSION) {
            log("SYSERR: add_to_save_list: Invalid zone number passed. (%d)", zone);
            return false;
        }
    }

    auto &l = save_list.emplace_back();
    l.zone = zone;
    l.type = type;
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
        send_to_char(ch, "All world files are up to date.\r\n");
    else {
        send_to_char(ch, "The following files need saving:\r\n");
        for (auto &i : save_list) {
            if (i.type != SL_CFG)
                send_to_char(ch, " - %s data for zone %d.\r\n", save_types[i.type].message, i.zone);
            else
                send_to_char(ch, " - Game configuration data.\r\n");
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

