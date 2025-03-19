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
int update_mobile_strings(struct char_data *t, struct char_data *f);

void check_mobile_strings(struct char_data *mob);

void check_mobile_string(mob_vnum i, char **string, const char *dscr);

int copy_mobile_strings(struct char_data *t, struct char_data *f);

#if CONFIG_GENOLC_MOBPROG
int write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd);
#endif

/* local functions */
void extract_mobile_all(mob_vnum vnum);

int add_mobile(struct char_data *mob, mob_vnum vnum) {
    mob_vnum rnum, found = false;
    struct char_data *live_mob;

    if ((rnum = real_mobile(vnum)) != NOBODY) {
        /* Copy over the mobile and free() the old strings. */
        copy_mobile(&mob_proto[rnum], mob);

        /* Now re-point all existing mobile strings to here. */
        auto mobs = characterSubscriptions.all(fmt::format("vnum_{}", vnum));
        for (auto live_mob : filter_raw(mobs)) {
            update_mobile_strings(live_mob, &mob_proto[rnum]);
        }

        basic_mud_log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
        return rnum;
    }

    auto &m = mob_proto[vnum];
    m = *mob;

    m.vn = 0;
    copy_mobile_strings(&m, mob);
    auto &ix = mob_index[vnum];
    ix.vn = vnum;

    basic_mud_log("GenOLC: add_mobile: Added mobile %d.", vnum);

#if CONFIG_GENOLC_MOBPROG
    GET_MPROG(OLC_MOB(d)) = OLC_MPROGL(d);
    GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
    while (OLC_MPROGL(d)) {
      GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
      OLC_MPROGL(d) = OLC_MPROGL(d)->next;
    }
#endif

    auto zvnum = real_zone_by_thing(vnum);
    auto &z = zone_table[zvnum];
    z.mobiles.insert(vnum);
    return found;
}

int copy_mobile(struct char_data *to, struct char_data *from) {
    free_mobile_strings(to);
    *to = *from;
    check_mobile_strings(from);
    copy_mobile_strings(to, from);
    return true;
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

    vnum = mob_index[refpt].vn;
    extract_mobile_all(vnum);
    auto &z = zone_table[real_zone_by_thing(refpt)];
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

int copy_mobile_strings(struct char_data *t, struct char_data *f) {
    if (f->name)
        t->name = strdup(f->name);
    if (f->title)
        t->title = strdup(f->title);
    if (f->short_description)
        t->short_description = strdup(f->short_description);
    if (f->room_description)
        t->room_description = strdup(f->room_description);
    if (f->look_description)
        t->look_description = strdup(f->look_description);
    return true;
}

int update_mobile_strings(struct char_data *t, struct char_data *f) {
    if (f->name)
        t->name = f->name;
    if (f->title)
        t->title = f->title;
    if (f->short_description)
        t->short_description = f->short_description;
    if (f->room_description)
        t->room_description = f->room_description;
    if (f->look_description)
        t->look_description = f->look_description;
    return true;
}

int free_mobile_strings(struct char_data *mob) {
    if (mob->name)
        free(mob->name);
    if (mob->title)
        free(mob->title);
    if (mob->short_description)
        free(mob->short_description);
    if (mob->room_description)
        free(mob->room_description);
    if (mob->look_description)
        free(mob->look_description);
    return true;
}


/* Free a mobile structure that has been edited. Take care of existing mobiles 
 * and their mob_proto!  */
int free_mobile(struct char_data *mob) {
    mob_rnum i;

    if (mob == nullptr)
        return false;

    /* Non-prototyped mobile.  Also known as new mobiles.  */
    if ((i = GET_MOB_RNUM(mob)) == NOBODY) {
        free_mobile_strings(mob);
        /* free script proto list */
        free_proto_script(mob, MOB_TRIGGER);
    } else {    /* Prototyped mobile. */
        if (mob->name && mob->name != mob_proto[i].name)
            free(mob->name);
        if (mob->title && mob->title != mob_proto[i].title)
            free(mob->title);
        if (mob->short_description && mob->short_description != mob_proto[i].short_description)
            free(mob->short_description);
        if (mob->room_description && mob->room_description != mob_proto[i].room_description)
            free(mob->room_description);
        if (mob->look_description && mob->look_description != mob_proto[i].look_description)
            free(mob->look_description);
    }
    while (mob->affected)
        affect_remove(mob, mob->affected);

    /* free any assigned scripts */
    extract_script(mob, MOB_TRIGGER);

    free(mob);
    return true;
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
    mob_vnum mvnum = mob_index[mob->vn].vn;
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

    if(mob_proto.contains(vn)) {
        services.insert(fmt::format("vnum_{}", vn));
    }

    if(trig_list) {
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
    if(PLR_FLAGGED(this, PLR_POWERUP))
        services.insert("powerupService");
    if(!damages.empty())
        services.insert("characterVitalsRecovery");
    if(!IS_ANDROID(this) && GET_LIFEPERC(this) > 0 && getCurHealthPercent() < GET_LIFEPERC(this))
        services.insert("lifeforceSystem");
    if(GET_CHARGE(this) || PLR_FLAGGED(this, PLR_CHARGE))
        services.insert("kiChargeSystem");
    if(PLR_FLAGGED(this, PLR_FISHING))
        services.insert("goneFishing");
    if(form != FormID::base || technique != FormID::base)
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


weight_t char_data::getWeight(bool base) {
    return get(CharDim::weight, base);
}

dim_t char_data::getHeight(bool base) {
    return get(CharDim::height, base);
}

dim_t char_data::setHeight(dim_t val) {
    return set(CharDim::height, std::max(0.0, val));
}

dim_t char_data::modHeight(dim_t val) {
    return setHeight(getHeight(true) + val);
}

double char_data::getTotalWeight() {
    return getWeight() + getCarriedWeight();
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

bool char_data::toggleMobFlag(int flag) {
    if(getMobFlag(flag)) {
        setMobFlag(flag, false);
        return false;
    } else {
        setMobFlag(flag, true);
        return true;
    }
}

bool char_data::getMobFlag(int flag) {
    return mob_flags.contains(static_cast<MobFlag>(flag));
}

void char_data::setMobFlag(int flag, bool value) {
    if(value) {
        mob_flags.insert(static_cast<MobFlag>(flag));
    } else {
        mob_flags.erase(static_cast<MobFlag>(flag));
    }
}

bool char_data::togglePlayerFlag(int flag) {
    if(getPlayerFlag(flag)) {
        setPlayerFlag(flag, false);
        return false;
    } else {
        setPlayerFlag(flag, true);
        return true;
    }
}

bool char_data::getPlayerFlag(int flag) {
    return player_flags.contains(static_cast<PlayerFlag>(flag));
}

void char_data::setPlayerFlag(int flag, bool value) {
    if(value) {
        player_flags.insert(static_cast<PlayerFlag>(flag));
    } else {
        player_flags.erase(static_cast<PlayerFlag>(flag));
    }
}

bool char_data::toggleAdminFlag(int flag) {
    if(getAdminFlag(flag)) {
        setAdminFlag(flag, false);
        return false;
    } else {
        setAdminFlag(flag, true);
        return true;
    }
}

bool char_data::getAdminFlag(int flag) {
    return admin_flags.contains(static_cast<AdminFlag>(flag));
}

void char_data::setAdminFlag(int flag, bool value) {
    if(value) {
        admin_flags.insert(static_cast<AdminFlag>(flag));
    } else {
        admin_flags.erase(static_cast<AdminFlag>(flag));
    }
}

bool char_data::togglePrefFlag(int flag) {
    if(getPrefFlag(flag)) {
        setPrefFlag(flag, false);
        return false;
    } else {
        setPrefFlag(flag, true);
        return true;
    }
}

bool char_data::getPrefFlag(int flag) {
    return pref_flags.contains(static_cast<PrefFlag>(flag));
}

void char_data::setPrefFlag(int flag, bool value) {
    if(value) {
        pref_flags.insert(static_cast<PrefFlag>(flag));
    } else {
        pref_flags.erase(static_cast<PrefFlag>(flag));
    }
}