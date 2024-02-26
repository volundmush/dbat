/************************************************************************
 * Generic OLC Library - Mobiles / genmob.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/genmob.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/genolc.h"
#include "dbat/shop.h"
#include "dbat/genzon.h"
#include "dbat/guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/handler.h"
#include "dbat/dg_olc.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/spells.h"
#include "dbat/players.h"
#include "dbat/account.h"

/* From db.c */
void check_mobile_strings(BaseCharacter *mob);

void check_mobile_string(mob_vnum i, char **string, const char *dscr);

/* local functions */


int save_mobiles(zone_rnum zone_num) {
    return true;
}


void check_mobile_string(mob_vnum i, char **string, const char *dscr) {
    if (*string == nullptr || **string == '\0') {
        char smbuf[128];
        sprintf(smbuf, "GenOLC: Mob #%d has an invalid %s.", i, dscr);
        mudlog(BRF, ADMLVL_GOD, true, smbuf);
        if (*string)
            free(*string);
        *string = strdup("An undefined string.");
    }
}

nlohmann::json time_data::serialize() {
    nlohmann::json j;

    if(birth) j["birth"] = birth;
    if(created) j["created"] = created;
    if(maxage) j["maxage"] = maxage;
    if(logon) j["logon"] = logon;
    if(played != 0.0) j["played"] = played;
    if(secondsAged != 0.0) j["secondsAged"] = secondsAged;

    return j;
}

void time_data::deserialize(const nlohmann::json &j) {
    if(j.contains("birth")) birth = j["birth"];
    if(j.contains("created")) created = j["created"];
    if(j.contains("maxage")) maxage = j["maxage"];
    if(j.contains("logon")) logon = j["logon"];
    if(j.contains("played")) played = j["played"];
    if(j.contains("secondsAged")) secondsAged = j["secondsAged"];
}

time_data::time_data(const nlohmann::json &j) : time_data() {
    deserialize(j);
}
