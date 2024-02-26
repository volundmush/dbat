#include "dbat/structs.h"
#include "dbat/account.h"
#include "dbat/utils.h"
#include "dbat/races.h"

// Flags
nlohmann::json Flags::serialize() {
    nlohmann::json j;
    for(auto &[ftype, fset] : flags) {
        j["flags"].push_back({ftype, fset});
    }
    return j;
}

void Flags::deserialize(const nlohmann::json& j) {
    if(j.contains("flags")) {
        auto ftypes = j["flags"];
        for(auto& ftype : ftypes) {
            auto ft = ftype[0].get<FlagType>();
            auto &fl = flags[ft];
            for(auto i : ftype[1]) {
                fl.insert(i.get<int>());
            }
        }
    }
}

Flags::Flags(const nlohmann::json& j) {
    deserialize(j);
}


// Extra Descriptions
extra_descr_data::extra_descr_data(const nlohmann::json& j) {
    deserialize(j);
}

void extra_descr_data::deserialize(const nlohmann::json& j) {
    if(j.contains("keyword")) keyword = j["keyword"];
    if(j.contains("description")) description = j["description"];
}

nlohmann::json extra_descr_data::serialize() {
    nlohmann::json j;
    if(!keyword.empty()) j["keyword"] = keyword;
    if(!description.empty()) j["description"] = description;
    return j;
}

ExtraDescriptions::ExtraDescriptions(const nlohmann::json& j) {
    deserialize(j);
}

void ExtraDescriptions::deserialize(const nlohmann::json& j) {
    if(j.contains("ex_description")) {
        auto extras = j["ex_description"];
        for(auto& extra : extras) {
            ex_description.emplace_back(extra);
        }
    }
}

nlohmann::json ExtraDescriptions::serialize() {
    nlohmann::json j;
    for(auto& extra : ex_description) {
        j["ex_description"].push_back(extra.serialize());
    }
    return j;
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

nlohmann::json Exit::serialize() {
    auto j = GameEntity::serialize();

    if(key > 0) j["key"] = key;

    if(dclock) j["dclock"] = dclock;

    return j;
}

void Exit::deserialize(const nlohmann::json& j) {
    GameEntity::deserialize(j);
    if(j.contains("key")) key = j["key"];
    if(j.contains("dclock")) dclock = j["dclock"];

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

nlohmann::json BaseCharacter::serialize() {
    auto j = GameEntity::serialize();

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

    if(upgrade) j["upgrade"] = upgrade;
    if(voice && strlen(voice)) j["voice"] = voice;

    if(relax_count) j["relax_count"] = relax_count;
    if(ingestLearned) j["ingestLearned"] = ingestLearned;

    if (poofin && strlen(poofin)) j["poofin"] = poofin;
    if (poofout && strlen(poofout)) j["poofout"] = poofout;

    j["transBonus"] = transBonus;
    for(auto &[frm, tra] : transforms) {
        j["transforms"].push_back(std::make_pair(static_cast<int>(frm), tra.serialize()));
    }

    return j;
}


void BaseCharacter::deserialize(const nlohmann::json &j) {
    GameEntity::deserialize(j);

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


BaseCharacter::BaseCharacter(const nlohmann::json &j) : BaseCharacter() {
    deserialize(j);

    SPEAKING(this) = SKILL_LANG_COMMON;
    set_height_and_weight_by_race(this);

    setFlag(FlagType::NPC, MOB_ISNPC);
    clearFlag(FlagType::NPC, MOB_NOTDEADYET);

    clearFlag(FlagType::PC, PLR_NOTDEADYET);

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

nlohmann::json PlayerCharacter::serialize() {
    auto j = nlohmann::json::object();

    if(account) j["account"] = account->vn;

    for(auto &a : aliases) {
        j["aliases"].push_back(a.serialize());
    }

    for(auto &i : senseEntity) {
        j["senseEntity"].push_back(i);
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


void PlayerCharacter::deserialize(const nlohmann::json &j) {

    if(j.contains("account")) {
        auto accID = j["account"].get<vnum>();
        if(auto accFind = accounts.find(accID); accFind != accounts.end()) account = accFind->second;
        else {
            basic_mud_log("Player data found with invalid account ID.");
        }
    }

    if(j.contains("aliases")) {
        for(auto ja : j["aliases"]) {
            aliases.emplace_back(ja);
        }
    }

    if(j.contains("senseEntity")) {
        for(auto &i : j["senseEntity"]) {
            senseEntity.insert(i.get<int64_t>());
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


nlohmann::json Room::serialize() {
    auto j = GameEntity::serialize();

    if(sector_type) j["sector_type"] = sector_type;

    if(timed) j["timed"] = timed;
    if(dmg) j["dmg"] = dmg;
    if(geffect) j["geffect"] = geffect;

    for(auto p : proto_script) {
        if(trig_index.contains(p)) j["proto_script"].push_back(p);
    }

    return j;
}

Room::Room(const nlohmann::json &j) {
    deserialize(j);
}

void Room::deserialize(const nlohmann::json &j) {
    GameEntity::deserialize(j);
    if(j.contains("sector_type")) sector_type = j["sector_type"];
    if(j.contains("timed")) timed = j["timed"];
    if(j.contains("dmg")) dmg = j["dmg"];
    if(j.contains("geffect")) geffect = j["geffect"];
}