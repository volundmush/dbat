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
#include "dbat/constants.h"
#include "dbat/filter.h"

/* From db.c */
void check_mobile_strings(struct char_data *mob);

void check_mobile_string(mob_vnum i, char **string, const char *dscr);

/* local functions */
void extract_mobile_all(mob_vnum vnum);

int add_mobile(struct npc_proto_data *mob, mob_vnum vnum) {
    mob_vnum rnum, found = false;
    struct char_data *live_mob;

    bool exists = mob_proto.contains(vnum);

    if (exists) {
        /* Copy over the mobile and free() the old strings. */
        mob_proto.at(rnum) = *mob;
        basic_mud_log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    } else {
        mob_proto[vnum] = *mob;
        auto &ix = mob_index[vnum];
        ix.vn = vnum;
        auto zvnum = real_zone_by_thing(vnum);
        auto& z = zone_table.at(zvnum);
        z.mobiles.insert(vnum);
        basic_mud_log("GenOLC: add_mobile: Added mobile %d.", vnum);
    }

    return vnum;
}


void extract_mobile_all(mob_vnum vnum) {
    auto mobs = characterSubscriptions.all(fmt::format("vnum_{}", vnum));
    for (auto ch : filter_raw(mobs)) {
        extract_char(ch);
    }
}

int delete_mobile(mob_rnum refpt) {
    struct char_data *live_mob;
    int counter, cmd_no;
    mob_vnum vnum;
    zone_rnum zone;

    if (!mob_proto.count(refpt)) {
        basic_mud_log("SYSERR: GenOLC: delete_mobile: Invalid rnum %d.", refpt);
        return NOBODY;
    }

    vnum = refpt;
    extract_mobile_all(vnum);
    auto& z = zone_table.at(real_zone_by_thing(refpt));
    z.mobiles.erase(refpt);

    /* Update zone table.  */
    for (auto &[zone, z] : zone_table) {
        z.cmd.erase(std::remove_if(z.cmd.begin(), z.cmd.end(), [refpt](auto &cmd) { return cmd.command == 'M' && cmd.arg1 == refpt; }));
    }

    /* Update shop keepers.  */
    for (auto &sh : shop_index) {
        /* Find the shop for this keeper and reset it's keeper to
         * -1 to keep the shop so it could be assigned to someone else */
        if (sh.second.keeper == refpt) {
            sh.second.keeper = NOBODY;
        }
    }

    /* Update guild masters */
    for (auto &g : guild_index) {
        /* Find the guild for this trainer and reset it's trainer to
         * -1 to keep the guild so it could be assigned to someone else */
        if (g.second.keeper == refpt) {
            g.second.keeper = NOBODY;
        }
    }

    mob_proto.erase(vnum);
    mob_index.erase(vnum);
    save_mobiles(real_zone_by_thing(vnum));

    return refpt;
}





int save_mobiles(zone_rnum zone_num) {
    return true;
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
    if (mob_prog->next == nullptr)
      fputs("|\n", fd);
  }
  return TRUE;
}
#endif


void check_mobile_strings(struct char_data *mob) {
    mob_vnum mvnum = mob->getVnum();
    check_mobile_string(mvnum, &GET_LDESC(mob), "long description");
    check_mobile_string(mvnum, &GET_DDESC(mob), "detailed description");
    check_mobile_string(mvnum, &GET_ALIAS(mob), "alias list");
    check_mobile_string(mvnum, &GET_SDESC(mob), "short description");
}

void check_mobile_string(mob_vnum i, char **string, const char *dscr) {
    if (*string == nullptr || **string == '\0') {
        char smbuf[128];
        snprintf(smbuf, sizeof(smbuf), "GenOLC: Mob #%d has an invalid %s.", i, dscr);
        mudlog(BRF, ADMLVL_GOD, true, "%s", smbuf);
        if (*string)
            free(*string);
        *string = strdup("An undefined string.");
    }
}


std::shared_ptr<char_data> char_data::shared() {
    return shared_from_this();
}

void char_data::activate() {
    if(active) {
        basic_mud_log("Attempted to activate an already active character.");
        return;
    }
    active = true;
    std::unordered_set<std::string> services;
    auto sh = shared_from_this();

    if(auto vn = getVnum(); mob_proto.contains(vn)) {
        services.insert(fmt::format("vnum_{}", vn));
    }

    assign_triggers(this, MOB_TRIGGER);

    if(!scripts.empty()) {
        activateScripts();

        if(SCRIPT_TYPES(this) & MTRIG_RANDOM)
            services.insert("randomTriggers");
        if(SCRIPT_TYPES(this) & MTRIG_TIME)
            services.insert("timeTriggers");
    }

    if(!IS_NPC(this)) services.insert("players");

    if(PLR_FLAGGED(this, PLR_GOOP))
        services.insert("goopTimeService");
    if(AFF_FLAGGED(this, AFF_POISON)) 
        services.insert("poisoned");
    if(PLR_FLAGGED(this, PLR_AURALIGHT))
        services.insert("auralight");
    if(ABSORBING(this))
        services.insert("androidAbsorbSystem");
    if(character_flags.get(CharacterFlag::powering_up))
        services.insert("powerupService");
    if(!isFullVitals())
        services.insert("characterVitalsRecovery");
    if(!IS_ANDROID(this) && GET_LIFEPERC(this) > 0 && getCurVitalMeterPercent(CharVital::health) < GET_LIFEPERC(this))
        services.insert("lifeforceSystem");
    if(GET_CHARGE(this) || PLR_FLAGGED(this, PLR_CHARGE))
        services.insert("kiChargeSystem");
    if(PLR_FLAGGED(this, PLR_FISHING))
        services.insert("goneFishing");
    if(form != Form::base || technique != Form::base)
        services.insert("transforms");
    services.insert("active");

    if(affected) {
        services.insert("affected");
    }

    if(affectedv) {
        services.insert("affectedv");
    }

    for(const auto& s : services) characterSubscriptions.subscribe(s, sh);

    activateContents();
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(GET_EQ(this, i)) {
            auto obj = GET_EQ(this, i);
            obj->activate();
        }
    }
}


void char_data::deactivate() {
    if(!active) return;
    active = false;
    char_data *temp = nullptr;

    for(auto &[vn, sc] : scripts) {
        sc->deactivate();
    }

    auto sh = shared_from_this();
    characterSubscriptions.unsubscribeFromAll(sh);
    deactivateContents();
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(GET_EQ(this, i)) {
            auto obj = GET_EQ(this, i);
            obj->deactivate();
        }
    }
}

bool char_data::isActive() {
    return active;
}


bool char_data::isProvidingLight() {
    if(!IS_NPC(this) && PLR_FLAGGED(this, PLR_AURALIGHT)) return true;
    for(auto i = 0; i < NUM_WEARS; i++) if(auto e = GET_EQ(this, i); e) if(e->isProvidingLight()) return true;
    return false;
}

double char_data::currentGravity() {
    if(auto room = getRoom(); room) {
        return room->getEnvironment(ENV_GRAVITY);
    }
    return 1.0;
}

struct obj_data* char_data::findObject(const std::function<bool(struct obj_data*)> &func, bool working) {
    auto o = unit_data::findObject(func, working);
    if(o) return o;

    for(auto i = 0; i < NUM_WEARS; i++) {
        auto obj = equipment[i];
        if(!obj) continue;
        if(working && !obj->isWorking()) continue;
        if(func(obj)) return obj;
        auto p = obj->findObject(func, working);
        if(p) return p;
    }

    return nullptr;
}

std::unordered_set<struct obj_data*> char_data::gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working) {
    auto out = unit_data::gatherObjects(func, working);

    for(auto i = 0; i < NUM_WEARS; i++) {
        auto obj = equipment[i];
        if(!obj) continue;
        if(working && !obj->isWorking()) continue;
        if(func(obj)) out.insert(obj);
        auto contents = obj->gatherObjects(func, working);
        out.insert(contents.begin(), contents.end());
    }
    return out;
}

void char_data::ageBy(double addedTime) {
    this->time.seconds_aged += addedTime;
}

void char_data::setAge(double newAge) {
    this->time.seconds_aged = newAge * SECS_PER_GAME_YEAR;
}

vnum char_data::getVnum() const {
    return proto ? proto->vn : NOTHING;
}

char* char_data::getName() {
    if(name) return name;
    if(proto && proto->name) return proto->name;
    return nullptr;
}

char* char_data::getRoomDescription() {
    if(room_description) return room_description;
    if(proto && proto->room_description) return proto->room_description;
    return nullptr;
}

char* char_data::getLookDescription() {
    if(look_description) return look_description;
    if(proto && proto->look_description) return proto->look_description;
    return nullptr;
}

char* char_data::getShortDescription() {
    if(short_description) return short_description;
    if(proto && proto->short_description) return proto->short_description;
    return nullptr;
}

extra_descr_data* char_data::getExtraDescription() {
    if(proto && proto->ex_description) return proto->ex_description;
    return nullptr;
}

char_data::~char_data() {
    if(title) free(title);
    struct affected_type *cmtemp;

    while (affected)
        REMOVE_FROM_LIST(affected, this->affected, next, cmtemp);

    while (affectedv)
        REMOVE_FROM_LIST(affectedv, this->affectedv, next, cmtemp);
    
    free_followers(followers);

    if (desc)
        desc->character = nullptr;

}

std::vector<trig_vnum> char_data::getProtoScript() const {
    return proto ? proto->proto_script : std::vector<trig_vnum>{};
}

std::string char_data::scriptString() const {
    std::vector<std::string> vnums;
    auto proto_script = getProtoScript();
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}