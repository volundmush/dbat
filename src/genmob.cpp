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
void check_mobile_strings(struct char_data *mob);

void check_mobile_string(mob_vnum i, char **string, const char *dscr);

/* local functions */
void extract_mobile_all(mob_vnum vnum);


void extract_mobile_all(mob_vnum vnum) {
    struct char_data *next, *ch;

    for (ch = character_list; ch; ch = next) {
        next = ch->next;
        if (GET_MOB_VNUM(ch) == vnum)
            extract_char(ch);
    }
}


int save_mobiles(zone_rnum zone_num) {
    return true;
}


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
        sprintf(smbuf, "GenOLC: Mob #%d has an invalid %s.", i, dscr);
        mudlog(BRF, ADMLVL_GOD, true, smbuf);
        if (*string)
            free(*string);
        *string = strdup("An undefined string.");
    }
}

nlohmann::json mob_special_data::serialize() {
    nlohmann::json j;
    if(attack_type) j["attack_type"] = attack_type;
    if(default_pos != POS_STANDING) j["default_pos"] = default_pos;
    if(damnodice) j["damnodice"] = damnodice;
    if(damsizedice) j["damsizedice"] = damsizedice;

    return j;
}


void mob_special_data::deserialize(const nlohmann::json &j) {
    if(j.contains("attack_type")) attack_type = j["attack_type"];
    if(j.contains("default_pos")) default_pos = j["default_pos"];
    if(j.contains("damnodice")) damnodice = j["damnodice"];
    if(j.contains("damsizedice")) damsizedice = j["damsizedice"];
}

mob_special_data::mob_special_data(const nlohmann::json &j) : mob_special_data() {
    deserialize(j);
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


nlohmann::json char_data::serialize() {
    auto j = unit_data::serialize();

    for(auto &[id, train] : trains) {
        if(train) j["trains"].push_back(std::make_pair(id, train));
    }

    for(auto &[id, attr] : attributes) {
        if(attr) j["attributes"].push_back(std::make_pair(id, attr));
    }

    for(auto &[id, mon] : moneys) {
        if(mon) j["moneys"].push_back(std::make_pair(id, mon));
    }

    for(auto &[id, align] : aligns) {
        if(align) j["aligns"].push_back(std::make_pair(id, align));
    }

    for(auto &[id, app] : appearances) {
        if(app) j["appearances"].push_back(std::make_pair(id, app));
    }

    for(auto &[id, app] : stats) {
        if(app) j["stats"].push_back(std::make_pair(id, app));
    }

    for(auto &[id, app] : nums) {
        if(app) j["nums"].push_back(std::make_pair(id, app));
    }

    for(auto i = 0; i < bodyparts.size(); i++)
        if(bodyparts.test(i)) j["bodyparts"].push_back(i);

    if(title && strlen(title)) j["title"] = title;
    j["race"] = race;

    j["chclass"] = chclass;
    if(weight != 0.0) j["weight"] = weight;


    if(armor) j["armor"] = armor;
    if(damage_mod) j["damage_mod"] = damage_mod;

        if(health < 1.0) j["health"] = health;
    if(energy < 1.0) j["energy"] = energy;
    if(stamina < 1.0) j["stamina"] = stamina;
    if(life < 1.0) j["life"] = life;

    if(exp) j["exp"] = exp;

    if(was_in_room != NOWHERE) j["was_in_room"] = was_in_room;
    auto td = time.serialize();
    if(!td.empty()) j["time"] = td;

    for(auto i = 0; i < 4; i++) {
        if(limb_condition[i]) j["limb_condition"].push_back(std::make_pair(i, limb_condition[i]));
    }

    for(auto i = 0; i < NUM_CONDITIONS; i++) {
        if(conditions[i]) j["conditions"].push_back(std::make_pair(i, conditions[i]));
    }

    if(freeze_level) j["freeze_level"] = freeze_level;
    if(invis_level) j["invis_level"] = invis_level;
    if(wimp_level) j["wimp_level"] = wimp_level;
    if(world.contains(load_room)) j["load_room"] = load_room;
    if(world.contains(hometown)) j["hometown"] = hometown;

    for(auto &[skill_id, s] : skill) {
        auto sk = s.serialize();
        if(!sk.empty()) j["skill"].push_back(std::make_pair(skill_id, sk));
    }

    if(speaking) j["speaking"] = speaking;
    if(preference) j["preference"] = preference;

    if(practice_points) j["practice_points"] = practice_points;

    for(auto a = affected; a; a = a->next) {
        if(a->type) j["affected"].push_back(a->serialize());
    }

    for(auto a = affectedv; a; a = a->next) {
        if(a->type) j["affectedv"].push_back(a->serialize());
    }

    if(absorbs) j["absorbs"] = absorbs;
    if(blesslvl) j["blesslvl"] = blesslvl;
    for(auto i = 0; i < 5; i++) {
        if(lboard[i]) j["lboard"].push_back(std::make_pair(i, lboard[i]));
    }

    for(auto i = 0; i < MAX_BONUSES; i++) {
        if(bonuses[i]) j["bonuses"].push_back(i);
    }

    if(boosts) j["boosts"] = boosts;

    if(clan && strlen(clan)) j["clan"] = clan;
    if(crank) j["crank"] = crank;
    if(con_cooldown) j["con_cooldown"] = con_cooldown;
    if(deathtime) j["deathtime"] = deathtime;
    if(dcount) j["dcount"] = dcount;
    if(death_type) j["death_type"] = death_type;
    if(damage_mod) j["damage_mod"] = damage_mod;
    if(droom) j["droom"] = droom;
    if(accuracy_mod) j["accuracy_mod"] = accuracy_mod;
    for(auto i = 0; i < 2; i++) {
        if(genome[i]) j["genome"].push_back(std::make_pair(i, genome[i]));
    }
    if(gauntlet) j["gauntlet"] = gauntlet;
    if(ingestLearned) j["ingestLearned"] = ingestLearned;
    if(kaioken) j["kaioken"] = kaioken;
    if(lifeperc) j["lifeperc"] = lifeperc;
    if(lastint) j["lastint"] = lastint;
    if(lastpl) j["lastpl"] = lastpl;
    if(moltexp) j["moltexp"] = moltexp;
    if(moltlevel) j["moltlevel"] = moltlevel;
    if(majinize) j["majinize"] = majinize;
    if(majinizer) j["majinizer"] = majinizer;
    if(mimic) j["mimic"] = mimic.value();
    if(form != FormID::Base) j["form"] = form;
    if(olc_zone) j["olc_zone"] = olc_zone;
    if(starphase) j["starphase"] = starphase;
    if(accuracy) j["accuracy"] = accuracy;
    if(position) j["position"] = position;

    if(rdisplay) j["rdisplay"] = rdisplay;
    if(relax_count) j["relax_count"] = relax_count;
    if(radar1) j["radar1"] = radar1;
    if(radar2) j["radar2"] = radar2;
    if(radar3) j["radar3"] = radar3;
    if(feature) j["feature"] = feature;
    if(ship) j["ship"] = ship;
    if(con_sdcooldown) j["con_sdcooldown"] = con_sdcooldown;
    if(shipr) j["shipr"] = shipr;
    if(skill_slots) j["skill_slots"] = skill_slots;
    if(stupidkiss) j["stupidkiss"] = stupidkiss;
    if(suppression) j["suppression"] = suppression;
    if(tail_growth) j["tail_growth"] = tail_growth;

    for(auto i = 0; i < 3; i++) {
        if(saving_throw[i]) j["saving_throw"].push_back(std::make_pair(i, saving_throw[i]));
    }
    for(auto i = 0; i < 3; i++) {
        if(apply_saving_throw[i]) j["apply_saving_throw"].push_back(std::make_pair(i, apply_saving_throw[i]));
    }
    if(upgrade) j["upgrade"] = upgrade;
    if(voice && strlen(voice)) j["voice"] = voice;

    if(relax_count) j["relax_count"] = relax_count;
    if(ingestLearned) j["ingestLearned"] = ingestLearned;

    if (poofin && strlen(poofin)) j["poofin"] = poofin;
    if (poofout && strlen(poofout)) j["poofout"] = poofout;
    if(players.contains(last_tell)) j["last_tell"] = last_tell;

    j["transBonus"] = transBonus;
    for(auto &[frm, tra] : transforms) {
        j["transforms"].push_back(std::make_pair(static_cast<int>(frm), tra.serialize()));
    }

    return j;
}


void char_data::deserialize(const nlohmann::json &j) {
    unit_data::deserialize(j);

    if(j.contains("trains")) {
        for(auto j2 : j["trains"]) {
            auto id = j2[0].get<CharTrain>();
            trains[id] = j2[1].get<attribute_train_t>();
        }
    }

    if(j.contains("attributes")) {
        for(auto j2 : j["attributes"]) {
            auto id = j2[0].get<CharAttribute>();
            attributes[id] = j2[1].get<attribute_t>();
        }
    }

    if(j.contains("moneys")) {
        for(auto j2 : j["moneys"]) {
            auto id = j2[0].get<CharMoney>();
            moneys[id] = j2[1].get<money_t>();
        }
    }

    if(j.contains("aligns")) {
        for(auto j2 : j["aligns"]) {
            auto id = j2[0].get<CharAlign>();
            aligns[id] = j2[1].get<align_t>();
        }
    }

    if(j.contains("appearances")) {
        for(auto j2 : j["appearances"]) {
            auto id = j2[0].get<CharAppearance>();
            appearances[id] = j2[1].get<appearance_t>();
        }
    }

    if(j.contains("stats")) {
        for(auto j2 : j["stats"]) {
            auto id = j2[0].get<CharStat>();
            stats[id] = j2[1].get<stat_t>();
        }
    }

    if(j.contains("nums")) {
        for(auto j2 : j["nums"]) {
            auto id = j2[0].get<CharNum>();
            nums[id] = j2[1].get<num_t>();
        }
    }

    if(j.contains("title")) title = strdup(j["title"].get<std::string>().c_str());
    if(j.contains("race")) race = j["race"].get<RaceID>();

    if(j.contains("chclass")) chclass = static_cast<SenseiID>(std::min(14, j["chclass"].get<int>()));

    if(j.contains("weight")) weight = j["weight"];

    if(j.contains("affected_by"))
        for(auto &i : j["affected_by"])
            setFlag(FlagType::Affect, i.get<int>());

    if(j.contains("armor")) armor = j["armor"];
    if(j.contains("damage_mod")) damage_mod = j["damage_mod"];
    if(j.contains("mob_specials")) mob_specials.deserialize(j["mob_specials"]);
    if(j.contains("mobFlags")) for(auto &i : j["mobFlags"]) setFlag(FlagType::NPC, i.get<int>());
    if(j.contains("playerFlags")) for(auto &i : j["playerFlags"]) playerFlags.set(i.get<int>());
    if(j.contains("pref")) for(auto &i : j["pref"]) setFlag(FlagType::Pref, i.get<int>());
    if(j.contains("bodyparts")) for(auto &i : j["bodyparts"]) bodyparts.set(i.get<int>());

    if(j.contains("hometown")) hometown = j["hometown"];

    if(j.contains("time")) {
        time.deserialize(j["time"]);
    }

    if(j.contains("health")) health = j["health"];
    if(j.contains("energy")) energy = j["energy"];
    if(j.contains("stamina")) stamina = j["stamina"];
    if(j.contains("life")) life = j["life"];

    if(j.contains("limb_condition")) {
        for(auto &i : j["limb_condition"]) {
            limb_condition[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("exp")) exp = j["exp"];

    if(j.contains("was_in_room")) was_in_room = j["was_in_room"];

    if(j.contains("skill")) {
        for(auto &i : j["skill"]) {
            auto id = i[0].get<uint16_t>();
            skill.emplace(id, i[1]);
        }
    }

    if(j.contains("affected")) {
        auto ja = j["affected"];
        // reverse iterate using .rbegin() and .rend() while filling out
        // the linked list.
        for(auto it = ja.rbegin(); it != ja.rend(); ++it) {
            auto a = new affected_type(*it);
            a->next = affected;
            affected = a;
        }
    }

    if(j.contains("affectedv")) {
        auto ja = j["affectedv"];
        // reverse iterate using .rbegin() and .rend() while filling out
        // the linked list.
        for(auto it = ja.rbegin(); it != ja.rend(); ++it) {
            auto a = new affected_type(*it);
            a->next = affectedv;
            affectedv = a;
        }
    }

    if(j.contains("absorbs")) absorbs = j["absorbs"];
    if(j.contains("blesslvl")) blesslvl = j["blesslvl"];
    if(j.contains("lboard")) {
        for(auto &i : j["lboard"]) {
            lboard[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("bonuses")) {
        for(auto &i : j["bonuses"]) {
            bonuses[i.get<int>()] = true;
        }
    }

    if(j.contains("boosts")) boosts = j["boosts"];

    if(j.contains("clan")) clan = strdup(j["clan"].get<std::string>().c_str());
    if(j.contains("crank")) crank = j["crank"];
    if(j.contains("con_cooldown")) con_cooldown = j["con_cooldown"];
    if(j.contains("deathtime")) deathtime = j["deathtime"];
    if(j.contains("dcount")) dcount = j["dcount"];
    if(j.contains("death_type")) death_type = j["death_type"];

    if(j.contains("conditions")) {
        for(auto &i : j["conditions"]) {
            conditions[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("damage_mod")) damage_mod = j["damage_mod"];
    if(j.contains("droom")) droom = j["droom"];
    if(j.contains("accuracy_mod")) accuracy_mod = j["accuracy_mod"];
    if(j.contains("genome")) {
        for(auto &i : j["genome"]) {
            genome[i[0].get<int>()] = i[1];
        }
    }
    if(j.contains("gauntlet")) gauntlet = j["gauntlet"];
    if(j.contains("ingestLearned")) ingestLearned = j["ingestLearned"];
    if(j.contains("kaioken")) kaioken = j["kaioken"];
    if(j.contains("lifeperc")) lifeperc = j["lifeperc"];
    if(j.contains("lastint")) lastint = j["lastint"];
    if(j.contains("lastpl")) lastpl = j["lastpl"];
    if(j.contains("moltexp")) moltexp = j["moltexp"];
    if(j.contains("moltlevel")) moltlevel = j["moltlevel"];
    if(j.contains("majinize")) majinize = j["majinize"];
    if(j.contains("majinizer")) majinizer = j["majinizer"];
    if(j.contains("mimic")) mimic = j["mimic"].get<RaceID>();
    if(j.contains("olc_zone")) olc_zone = j["olc_zone"];
    if(j.contains("starphase")) starphase = j["starphase"];
    if(j.contains("accuracy")) accuracy = j["accuracy"];
    if(j.contains("position")) position = j["position"];

    if(j.contains("rdisplay")) rdisplay = strdup(j["rdisplay"].get<std::string>().c_str());
    if(j.contains("relax_count")) relax_count = j["relax_count"];
    if(j.contains("radar1")) radar1 = j["radar1"];
    if(j.contains("radar2")) radar2 = j["radar2"];
    if(j.contains("radar3")) radar3 = j["radar3"];
    if(j.contains("feature")) feature = strdup(j["feature"].get<std::string>().c_str());
    if(j.contains("ship")) ship = j["ship"];
    if(j.contains("con_sdcooldown")) con_sdcooldown = j["con_sdcooldown"];
    if(j.contains("shipr")) shipr = j["shipr"];
    if(j.contains("skill_slots")) skill_slots = j["skill_slots"];
    if(j.contains("stupidkiss")) stupidkiss = j["stupidkiss"];
    if(j.contains("suppression")) suppression = j["suppression"];
    if(j.contains("tail_growth")) tail_growth = j["tail_growth"];

    if(j.contains("saving_throw")) {
        for(auto t : j["saving_throw"]) {
            saving_throw[t[0].get<int>()] = t[1];
        }
    }
    if(j.contains("apply_saving_throw")) {
        for(auto t : j["apply_saving_throw"]) {
            apply_saving_throw[t[0].get<int>()] = t[1];
        }
    }
    if(j.contains("upgrade")) upgrade = j["upgrade"];
    if(j.contains("voice")) voice = strdup(j["voice"].get<std::string>().c_str());
    if(j.contains("wimp_level")) wimp_level = j["wimp_level"];
    
    if(j.contains("dgvariables")) {
        // dgvariables is a json object of string keys and string values which must fill up script->vars the likewise map.
        for(auto &i : j["dgvariables"].items()) {
            script->vars[i.key()] = i.value();
        }
        
    }

    if(j.contains("load_room")) load_room = j["load_room"];

    if(j.contains("transBonus")) transBonus = j["transBonus"];
    if(j.contains("form")) form = j["form"].get<FormID>();
    if(j.contains("transforms")) {
        // it is a list of pairs that fills up the transforms map.
        for(const auto &j2 : j["transforms"]) {
            transforms.emplace(j2[0].get<FormID>(), j2[1]);
        }
    }

    if(j.contains("preference")) preference = j["preference"];
    if(j.contains("freeze_level")) freeze_level = j["freeze_level"];
    if(j.contains("practice_points")) practice_points = j["practice_points"];
    if(j.contains("speaking")) speaking = j["speaking"];

}


char_data::char_data(const nlohmann::json &j) : char_data() {
    deserialize(j);

    if (!IS_HUMAN(this))
        setFlag(FlagType::Affect, AFF_INFRAVISION);

    SPEAKING(this) = SKILL_LANG_COMMON;
    set_height_and_weight_by_race(this);

    setFlag(FlagType::NPC, MOB_ISNPC);
    clearFlag(FlagType::NPC, MOB_NOTDEADYET);

    playerFlags.reset(PLR_NOTDEADYET);

}

nlohmann::json skill_data::serialize() {
    nlohmann::json j = nlohmann::json::object();

    if(level) j["level"] = level;
    if(perfs) j["perfs"] = perfs;

    return j;
}


void skill_data::deserialize(const nlohmann::json &j) {
    if(j.contains("level")) level = j["level"];
    if(j.contains("perfs")) perfs = j["perfs"];
}

skill_data::skill_data(const nlohmann::json &j) : skill_data() {
    deserialize(j);
}


nlohmann::json alias_data::serialize() {
    auto j = nlohmann::json::object();

    if(!name.empty()) j["name"] = name;
    if(!replacement.empty()) j["replacement"] = replacement;
    if(type) j["type"] = type;

    return j;
}


alias_data::alias_data(const nlohmann::json &j) : alias_data() {
    if(j.contains("name")) name = j["name"].get<std::string>();
    if(j.contains("replacement")) replacement = j["replacement"].get<std::string>();
    if(j.contains("type")) type = j["type"];
}

nlohmann::json player_data::serialize() {
    auto j = nlohmann::json::object();
    j["id"] = id;
    j["name"] = name;
    if(account) j["account"] = account->vn;

    for(auto &a : aliases) {
        j["aliases"].push_back(a.serialize());
    }

    for(auto &i : sensePlayer) {
        j["sensePlayer"].push_back(i);
    }

    for(auto &i : senseMemory) {
        j["senseMemory"].push_back(i);
    }

    for(auto &i : dubNames) {
        j["dubNames"].push_back(i);
    }

    for(auto i = 0; i < NUM_COLOR; i++) {
        if(color_choices[i] && strlen(color_choices[i])) j["color_choices"].push_back(std::make_pair(i, color_choices[i]));
    }

    return j;
}


player_data::player_data(const nlohmann::json &j) {
    id = j["id"];
    name = j["name"].get<std::string>();
    if(j.contains("account")) {
        auto accID = j["account"].get<vnum>();
        if(auto accFind = accounts.find(accID); accFind != accounts.end()) account = accFind->second;
    }

    if(j.contains("aliases")) {
        for(auto ja : j["aliases"]) {
            aliases.emplace_back(ja);
        }
    }

    if(j.contains("sensePlayer")) {
        for(auto &i : j["sensePlayer"]) {
            sensePlayer.insert(i.get<int64_t>());
        }
    }

    if(j.contains("senseMemory")) {
        for(auto &i : j["senseMemory"]) {
            senseMemory.insert(i.get<vnum>());
        }
    }

    if(j.contains("dubNames")) {
        for(auto &i : j["dubNames"]) {
            dubNames.emplace(i[0].get<int64_t>(), i[1].get<std::string>());
        }
    }

    if(j.contains("color_choices")) {
        for(auto &i : j["color_choices"]) {
            color_choices[i[0].get<int>()] = strdup(i[1].get<std::string>().c_str());
        }
    }

}

void char_data::activate() {
    if(active) {
        basic_mud_log("Attempted to activate an already active character.");
        return;
    }
    active = true;
    next = character_list;
    character_list = this;

    if(script) script->activate();

    if(mob_proto.contains(vn)) {
        insert_vnum(characterVnumIndex, this);
    }
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(GET_EQ(this, i)) {
            auto obj = GET_EQ(this, i);
            obj->activate();
        }
    }
    if(affected) {
        next_affect = affect_list;
        affect_list = this;
    }
    if(affectedv) {
        next_affectv = affectv_list;
        affectv_list = this;
    }
}


void char_data::deactivate() {
    if(!active) return;
    active = false;
    struct char_data *temp;
    REMOVE_FROM_LIST(this, character_list, next, temp);

    if(vn != NOTHING) {
        erase_vnum(characterVnumIndex, this);
    }

    if(affected) {
        REMOVE_FROM_LIST(this, affect_list, next_affect, temp);
    }
    if(affectedv) {
        REMOVE_FROM_LIST(this, affectv_list, next_affectv, temp);
    }

    for(auto i = 0; i < NUM_WEARS; i++) {
        if(GET_EQ(this, i)) {
            auto obj = GET_EQ(this, i);
            obj->deactivate();
        }
    }
}

nlohmann::json affected_type::serialize() {
    auto j = nlohmann::json::object();

    if(type) j["type"] = type;
    if(duration) j["duration"] = duration;
    if(modifier) j["modifier"] = modifier;
    if(location) j["location"] = location;
    if(specific) j["specific"] = specific;
    if(bitvector) j["bitvector"] = bitvector;

    return j;
}

affected_type::affected_type(const nlohmann::json &j) {
    if(j.contains("type")) type = j["type"];
    if(j.contains("duration")) duration = j["duration"];
    if(j.contains("modifier")) modifier = j["modifier"];
    if(j.contains("location")) location = j["location"];
    if(j.contains("specific")) specific = j["specific"];
    if(j.contains("bitvector")) bitvector = j["bitvector"];
}

weight_t char_data::getWeight(bool base) {
    auto total = weight;

    if(!base) {
        total += getAffectModifier(APPLY_CHAR_WEIGHT);
        total *= (1.0 + getAffectModifier(APPLY_WEIGHT_MULT));
    }

    return total;
}

int char_data::getHeight(bool base) {
    int total = get(CharNum::Height);

    if(!base) {
        total += getAffectModifier(APPLY_CHAR_HEIGHT);
        total *= (1.0 + getAffectModifier(APPLY_HEIGHT_MULT));
    }

    return total;
}

int char_data::setHeight(int val) {
    return set(CharNum::Height, std::max(0, val));
}

int char_data::modHeight(int val) {
    return setHeight(getHeight(true) + val);
}

double char_data::getTotalWeight() {
    return getWeight() + getCarriedWeight();
}

bool char_data::isActive() {
    return active;
}


nlohmann::json char_data::serializeRelations() {
    auto j = nlohmann::json::object();

    return j;
}


void char_data::save() {

}

bool char_data::isProvidingLight() {
    if(!IS_NPC(this) && PLR_FLAGGED(this, PLR_AURALIGHT)) return true;
    for(auto i = 0; i < NUM_WEARS; i++) if(auto e = GET_EQ(this, i); e) if(e->isProvidingLight()) return true;
    return false;
}


double char_data::currentGravity() {
    return myEnvVar(EnvVar::Gravity);
}

struct obj_data* char_data::findObject(const std::function<bool(struct obj_data*)> &func, bool working) {
    auto o = unit_data::findObject(func, working);
    if(o) return o;

    for(auto [pos, obj] : getEquipment()) {
        if(working && !obj->isWorking()) continue;
        if(func(obj)) return obj;
        auto p = obj->findObject(func, working);
        if(p) return p;
    }

    return nullptr;
}

std::set<struct obj_data*> char_data::gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working) {
    auto out = unit_data::gatherObjects(func, working);

    for(auto [pos, obj] : getEquipment()) {
        if(working && !obj->isWorking()) continue;
        if(func(obj)) out.insert(obj);
        auto contents = obj->gatherObjects(func, working);
        out.insert(contents.begin(), contents.end());
    }
    return out;
}

void char_data::ageBy(double addedTime) {
    this->time.secondsAged += addedTime;
}

void char_data::setAge(double newAge) {
    this->time.secondsAged = newAge * SECS_PER_GAME_YEAR;
}

void npc_data::assignTriggers() {
    // Nothing to do without a prototype...

    // remove all duplicates from i->proto_script but do not change its order otherwise.
    std::set<trig_vnum> existVnums;
    std::set<trig_vnum> valid;
    for(auto t : proto_script) valid.insert(t);
    
    for(auto t : script->dgScripts) existVnums.insert(t->parent->vn);
    bool added = false;
    bool removed = false;

    // remove any dgScript instances in i->script->dgScripts that aren't in i->proto_script
    std::list<std::shared_ptr<trig_data>> validScripts;
    for(auto t : script->dgScripts) {
        if(valid.contains(t->parent->vn)) {
            validScripts.push_back(t);
        }
        else {
            removed = true;
        }
    }
    if(removed) script->dgScripts = validScripts;

    for(auto p : proto_script) {
        // only add if they don't already have one...
        if(!existVnums.contains(p)) {
            script->addTrigger(read_trigger(p), -1);
            added = true;
            existVnums.insert(p);
        }
    }

    if(added || removed) {
        // we need to sort i->script->dgScripts by the order of i->proto_script
        std::list<std::shared_ptr<trig_data>> sorted;
        for(auto p : proto_script) {
            for(auto t : script->dgScripts) {
                if(t->parent->vn == p) {
                    sorted.push_back(t);
                    break;
                }
            }
        }
        script->dgScripts = sorted;
    }
}