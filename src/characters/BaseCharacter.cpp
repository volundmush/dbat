#include "dbat/structs.h"
#include "dbat/races.h"
#include "dbat/utils.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"
#include "dbat/config.h"
#include "dbat/mail.h"
#include "dbat/dg_comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/interpreter.h"
#include "dbat/players.h"
#include "dbat/transformation.h"
#include "dbat/weather.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/constants.h"


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
    if(players.contains(last_tell)) j["last_tell"] = last_tell;

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

void BaseCharacter::activate() {
    if(active) {
        basic_mud_log("Attempted to activate an already active character.");
        return;
    }
    active = true;
    next = character_list;
    character_list = this;

    if(script) script->activate();

    if(mob_proto.contains(vn)) {
        insert_vnum(characterVnumIndex, dynamic_cast<NonPlayerCharacter*>(this));
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


void BaseCharacter::deactivate() {
    if(!active) return;
    active = false;
    BaseCharacter *temp;
    REMOVE_FROM_LIST(this, character_list, next, temp);

    if(vn != NOTHING) {
        erase_vnum(characterVnumIndex, dynamic_cast<NonPlayerCharacter*>(this));
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

static std::string robot = "Robotic-Humanoid", robot_lower = "robotic-humanoid", unknown = "UNKNOWN";

void BaseCharacter::lookAtLocation() {
    if (GET_POS(this) < POS_SLEEPING)
    {
        sendf("You can't see anything but stars!\r\n");
        return;
    }

    if (AFF_FLAGGED(this, AFF_BLIND))
    {
        sendf("You can't see a damned thing, you're blind!\r\n");
        return;
    }

    if (PLR_FLAGGED(this, PLR_EYEC)) {
        sendf("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }
    
    if(auto loc = getLocation(); loc) {
        sendLine(loc->renderLocationFor(this));
    } else {
        sendf("You are in a void.\r\n");
    }
}

std::vector<std::string> BaseCharacter::baseKeywordsFor(GameEntity* looker) {
    std::vector<std::string> out;

    out.emplace_back(juggleRaceName(false));
    // todo: account for mimicking a race with an unavailable sex?
    if(auto sex = get(CharAppearance::Sex); sex > 0) {
        out.emplace_back(genders[sex]);
    }

    return out;
}

bool BaseCharacter::isInvisible() {
    return AFF_FLAGGED(this, AFF_INVISIBLE);
}

bool BaseCharacter::canSeeInvisible() {
    return AFF_FLAGGED(this, AFF_DETECT_INVIS);
}

bool BaseCharacter::isHidden() {
    return AFF_FLAGGED(this, AFF_HIDE);
}

bool BaseCharacter::isAdminInvisible() {
    return false;
}

bool BaseCharacter::canSeeAdminInvisible() {
    return checkFlag(FlagType::Pref, PRF_HOLYLIGHT);
}

bool BaseCharacter::canSeeInDark() {
    if(checkFlag(FlagType::Pref, PRF_HOLYLIGHT)) return true;
    if(isProvidingLight()) return true;
    if(AFF_FLAGGED(this, AFF_INFRAVISION)) return true;
    if(race == RaceID::Mutant && (genome[0] == 4 || genome[1] == 4)) return true;
    if(race == RaceID::Animal) return true;
    return false;
}

bool BaseCharacter::isProvidingLight() {
    if(checkFlag(FlagType::PC, PLR_AURALIGHT)) return true;
    for(auto &[pos, e] : getEquipment()) {
        if(e->isProvidingLight()) return true;
    }
    return false;
}

UnitFamily BaseCharacter::getFamily() {
    return UnitFamily::Character;
}

void BaseCharacter::deserializeRelations(const nlohmann::json& j) {
    GameEntity::deserializeRelations(j);
}

nlohmann::json BaseCharacter::serializeRelations() {
    return GameEntity::serializeRelations();
}

BaseCharacter::~BaseCharacter() {
    if (title)
        free(title);
    if(clan)
        free(clan);
    if(voice)
        free(voice);
}

room_vnum BaseCharacter::normalizeLoadRoom(room_vnum in) {
    // If they were in the void, then we need to use their last good room.
    room_vnum room = NOWHERE;
    room_vnum lroom = NOWHERE;
    // Handle the void issue...
    if (in == 0 || in == 1) {
        room = GET_WAS_IN(this);
    } else {
        room = in;
    }

    // Personal Pocket Dimensions
    //if (room >= 19800 && room <= 19899) {
        //lroom = room;
    //}
        // those stuck in the pendulum room past get returned to Kami's Lookout.
    if (ROOM_FLAGGED(room, ROOM_PAST)) {
        lroom = 1561;
    }
        // the WMAT arena also is not a good place to log off.
    else if (room >= 2002 && room <= 2011) {
        lroom = 1960;
    }
        // The two Minecarts are possible trap zones.
    else if (room == 2069) {
        lroom = 2017;
    }
    else if (room == 2070) {
        lroom = 2046;
    }
        // The higher plane is a problem...
    else if(room == 6030) {
        // Stick them on the side of King Yemma's, in case they're broke/weak.
        lroom = 6029;
    }
        // This is the MUD School. If they're not done then put them
        // back at the start. Otherwise, send them to their Sensei.
    else if (room >= 101 && room <= 139) {
        if (GET_LEVEL(this) == 1) {
            lroom = 100;
            setExperience(0);
        } else {
            lroom = sensei::getStartRoom(chclass);
        }
    }
    else {
        // looks like room might be okay.
        lroom = room;
    }

    // if lroom is valid, save it... else... emergency fallback to mud school.
    if(world.contains(lroom)) return lroom;
    return CONFIG_MORTAL_START;

}


int BaseCharacter::getArmor() {
    int out = get(CharNum::ArmorWishes) * 5000;
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto obj = GET_EQ(this, i); obj)
            out += obj->getAffectModifier(APPLY_AC, -1);
    }
    return out;
}

int64_t BaseCharacter::getExperience() {
    return exp;
}

int64_t BaseCharacter::setExperience(int64_t value) {
    exp = value;
    if(exp < 0) exp = 0;
    return exp;
}

// This returns the exact amount that was modified by.
int64_t BaseCharacter::modExperience(int64_t value, bool applyBonuses) {

    if(value < 0) {
        // removing experience. We can do this easily.
        auto cur = getExperience();
        auto new_value = setExperience(cur + value);
        // return the actual amount substracted as a negative.
        return cur - new_value;
    }

    // Adding experience may involve bonuses.
    auto gain = value;
    auto cur = getExperience();

    if(!applyBonuses) {
        gain *= (1.0 + getAffectModifier(APPLY_EXP_GAIN_MULT));

        if (AFF_FLAGGED(this, AFF_WUNJO)) {
            gain *= 1.15;
        }
        if (PLR_FLAGGED(this, PLR_IMMORTAL)) {
            gain *= 0.95;
        }

        int64_t diff = gain * 0.15;

        if (gain > 0) {

            // TODO: Modify the spar booster with APPLY_EXP_GAIN_MULT 0.25
            if (auto obj = GET_EQ(this, WEAR_SH); obj && obj->getVN() == 1127) {
                int64_t spar = gain;
                gain += gain * 0.25;
                spar = gain - spar;
                this->sendf("@D[@BBooster EXP@W: @G+%s@D]\r\n", add_commas(spar).c_str());
            }

            // Post-100 gains.
            if (GET_LEVEL(this) == 100 && GET_ADMLEVEL(this) < 1) {
                if (IS_KANASSAN(this) || IS_DEMON(this)) {
                    diff = diff * 1.3;
                }
                if (IS_ANDROID(this)) {
                    diff = diff * 1.2;
                }

                if (rand_number(1, 5) >= 2) {
                    if (IS_HUMAN(this)) {
                        this->gainBasePL(diff * 0.8);
                    } else {
                        this->gainBasePL(diff);
                    }
                    this->sendf("@D[@G+@Y%s @RPL@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2) {
                    if (IS_HALFBREED(this)) {
                        this->gainBaseST(diff * 0.85);
                    } else {
                        this->gainBaseST(diff);
                    }
                    this->sendf("@D[@G+@Y%s @gSTA@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2) {
                    this->gainBaseKI(diff);
                    this->sendf("@D[@G+@Y%s @CKi@D]@n", add_commas(diff).c_str());
                }
            }
        }
    }

    // Amount gained cannot be negative.
    gain = std::max<int64_t>(gain, 0);

    if (MINDLINK(this) && gain > 0 && LINKER(this) == 0) {
        if (GET_LEVEL(this) + 20 < GET_LEVEL(MINDLINK(this)) || GET_LEVEL(this) - 20 > GET_LEVEL(MINDLINK(this))) {
            MINDLINK(this)->sendf("The level difference between the two of you is too great to gain from mind read.\r\n");
        } else {
            act("@GYou've absorbed some new experiences from @W$n@G!@n", false, this, nullptr, MINDLINK(this),
                TO_VICT);
            int64_t read = gain * 0.12;
            gain -= read;
            if (read == 0)
                read = 1;
            MINDLINK(this)->modExperience(read, false);
            act("@RYou sense that @W$N@R has stolen some of your experiences with $S mind!@n", false, this,
                nullptr, MINDLINK(this), TO_CHAR);
        }
    }

    if(GET_LEVEL(this) < 100) {
        int64_t tnl = level_exp(this, GET_LEVEL(this) + 1);

        if(cur < tnl && (cur + gain) >= tnl) {
            this->sendf("@rYou have earned enough experience to gain a @ylevel@r.@n\r\n");
        }

        int64_t max_over_tnl = tnl * 5;
        if((cur + gain) >= max_over_tnl) {
            gain = max_over_tnl - getExperience();
            this->sendf("@WYou -@RNEED@W- to @ylevel@W. You can't hold any more experience!@n\r\n");
        }

    }

    if(gain) setExperience(cur + gain);
    return gain;

}

void BaseCharacter::gazeAtMoon() {
    if(OOZARU_RACE(this) && checkFlag(FlagType::PC, PLR_TAIL)) {
        if(form == FormID::Oozaru || form == FormID::GoldenOozaru) return;
        FormID toForm = FormID::Oozaru;
        if(transforms.contains(FormID::SuperSaiyan)
        || transforms.contains(FormID::SuperSaiyan2)
        || transforms.contains(FormID::SuperSaiyan3)
        || transforms.contains(FormID::SuperSaiyan4))
            toForm = FormID::GoldenOozaru;

        trans::handleEchoTransform(this, toForm);
        form = toForm;
    }
}

void BaseCharacter::sendEvent(const Event &ev) {
    if(desc) desc->sendEvent(ev);
}

void BaseCharacter::sendText(const std::string &text) {
    if(desc) desc->sendText(text);
}

static const std::map<std::string, CharAttribute> _attr_names = {
    {"str", CharAttribute::Strength},
    {"wis", CharAttribute::Wisdom},
    {"con", CharAttribute::Constitution},
    {"cha", CharAttribute::Speed},
    {"spd", CharAttribute::Speed},
    {"dex", CharAttribute::Agility},
    {"agi", CharAttribute::Agility},
    {"int", CharAttribute::Intelligence}
};

static const std::map<std::string, CharMoney> _money_names = {
    {"bank", CharMoney::Bank},
    {"gold", CharMoney::Carried},
    {"zenni", CharMoney::Carried}
};

static const std::map<std::string, int> _cond_names = {
    {"hunger", HUNGER},
    {"thirst", THIRST},
    {"drunk", DRUNK}
};

static const std::map<std::string, int> _save_names = {
    {"saving_fortitude", SAVING_FORTITUDE},
    {"saving_reflex", SAVING_REFLEX},
    {"saving_will", SAVING_WILL}
};

static const std::map<std::string, int> _pflags = {
    {"is_killer", PLR_KILLER},
    {"is_thief", PLR_THIEF}
};

static const std::map<std::string, int> _aflags = {
    {"dead", AFF_SPIRIT},
    {"flying", AFF_FLYING}
};

static const std::set<std::string> _senseiCheck = {"sensei", "class"};


DgResults BaseCharacter::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    std::string lmember = member;
    to_lower(lmember);
    trim(lmember);

    if(auto attr = _attr_names.find(lmember); attr != _attr_names.end()) {
        if (!arg.empty()) {
            attribute_t addition = atof(arg.c_str());
            mod(attr->second, addition);
        }
        return fmt::format("{}", get(attr->second));
    }

    if(auto mon = _money_names.find(lmember); mon != _money_names.end()) {
        if (!arg.empty()) {
            money_t addition = atoll(arg.c_str());
            mod(mon->second, addition);
        }
        return fmt::format("{}", get(mon->second));
    }

    if(auto con = _cond_names.find(lmember); con != _cond_names.end()) {
        if (!arg.empty()) {
            int addition = atof(arg.c_str());
            GET_COND(this, con->second) = std::clamp<int>(addition, -1, 24);
        }
        return fmt::format("{}", GET_COND(this, con->second));
    }

    if(auto save = _save_names.find(lmember); save != _save_names.end()) {
        if (!arg.empty()) {
            int addition = atof(arg.c_str());
        }
        return "";
    }

    if(auto pf = _pflags.find(lmember); pf != _pflags.end()) {
        if (!arg.empty()) {
            if (!strcasecmp("on", arg.c_str()))
                setFlag(FlagType::PC, pf->second);
            else if (!strcasecmp("off", arg.c_str()))
                clearFlag(FlagType::PC, pf->second);
        }
        return checkFlag(FlagType::PC, pf->second) ? "1" : "0";
    }

    if(auto af = _aflags.find(lmember); af != _aflags.end()) {
        return AFF_FLAGGED(this, af->second) ? "1" : "0";
    }

    if(lmember == "aaaaa") {
        // Is this even used?
        return "0";
    }

    if(lmember == "affect") {
        if(arg.empty()) return "0";
        int affect = get_flag_by_name(affected_bits, (char*)arg.c_str());
        return (affect != NOFLAG && AFF_FLAGGED(this, affect)) ? "1" : "0";
    }

    if(lmember == "alias") {
        return GET_PC_NAME(this);
    }

    if(lmember == "align") {
        if (!arg.empty()) {
            int addition = atof(arg.c_str());
            set(CharAlign::GoodEvil, std::clamp<int>(addition, -1000, 1000));
        }
        return fmt::format("{}", GET_ALIGNMENT(this));
    }

    if(lmember == "canbeseen") {
        if(trig->parent->attach_type != 2) return "0";
        auto owner = (BaseCharacter*)trig->sc->owner;
        return CAN_SEE(owner, this) ? "1" : "0";
    }

    if(lmember == "carry") {
        return CARRYING(this) ? "1" : "0";
    }

    if(lmember == "clan") {
        return clan && strstr(clan, arg.c_str()) ? "1": "0";
    }

    if(_senseiCheck.contains(lmember)) {
        return sensei::getName(chclass);
    }

    if(lmember == "death") {
        return fmt::format("{}", GET_DTIME(this));
    }

    if(lmember == "drag") {
        return DRAGGING(this) ? "1" : "0";
    }

    if(lmember == "eq") {
        if(arg.empty()) return "";
        else if(arg == "*") {
            if(auto eq = getEquipment(); !eq.empty()) return "1";
            return "0";
        }
        else {
            auto pos = find_eq_pos_script((char*)arg.c_str());
            if(pos == -1) return "";
            auto eq = getEquipment();
            if(eq[pos]) return eq[pos];
            return "";
        }
    }

    if(lmember == "exp") {
        if (!arg.empty()) {
            int64_t addition = std::max<int64_t>(0, atof(arg.c_str()));
            modExperience(addition);
        }
        return fmt::format("{}", GET_EXP(this));
    }

    if(lmember == "fighting") {
        if(fighting) return fighting;
        return "";
    }

    if(lmember == "followers") {
        if(followers && followers->follower) return followers->follower;
        return "";
    }

    if(lmember == "has_item") {
        if(arg.empty()) return "";
        return fmt::format("{}", char_has_item((char*)arg.c_str(), this));
    }

    if(lmember == "hisher") return HSHR(this);
    if(lmember == "heshe") return HSSH(this);
    if(lmember == "himher") return HMHR(this);

    if(lmember == "hitp") {
        if (!arg.empty()) {
            int64_t addition = atof(arg.c_str());
            if (addition > 0) {
                incCurHealth(addition);
            } else {
                decCurHealth(addition);
            }
            update_pos(this);
        }
        return fmt::format("{}", GET_HIT(this));
    }

    if(lmember == "id") return this;

    if(lmember == "is_pc") return IS_NPC(this) ? "1":"0";

    if(lmember == "inventory") {
        if(arg.empty()) {
            if(auto con = getInventory(); !con.empty()) return con.front();
            return "";
        }
        obj_vnum v = atoll(arg.c_str());
        if(auto found = findObjectVnum(v); found) return found;
        return "";
    }

    if(lmember == "level") return fmt::format("{}", GET_LEVEL(this));

    if(lmember == "maxhitp") {
        if(!arg.empty()) {
            int64_t addition = atof(arg.c_str());
        }
        return fmt::format("{}", GET_MAX_HIT(this));
    }

    if(lmember == "mana") {
        if (!arg.empty()) {
            int64_t addition = atof(arg.c_str());
            if (addition > 0) {
                incCurKI(addition);
            } else {
                decCurKI(addition);
            }
        }
        return fmt::format("{}", getCurKI());
    }

    if(lmember == "maxmana") {
        if(!arg.empty()) {
            int64_t addition = atof(arg.c_str());
        }
        return fmt::format("{}", GET_MAX_MANA(this));
    }

    if(lmember == "move") {
        if (!arg.empty()) {
            int64_t addition = atof(arg.c_str());
            if (addition > 0) {
                incCurST(addition);
            } else {
                decCurST(addition);
            }
        }
        return fmt::format("{}", getCurST());
    }

    if(lmember == "maxmove") {
        if(!arg.empty()) {
            int64_t addition = atof(arg.c_str());
        }
        return fmt::format("{}", GET_MAX_MOVE(this));
    }

    if(lmember == "master") {
        if(master) return master;
        return "";
    }

    if(lmember == "name") return GET_NAME(this);

    if(lmember == "next_in_room") {
        auto loc = getRoom();
        if(!loc) return "";
        auto people = loc->getPeople();
        auto found = std::find(people.begin(), people.end(), this);
        if(found != people.end() && ++found != people.end()) return *found;
        return "";
    }

    if(lmember == "pos") {
        if(!arg.empty()) {
            // This is stupid. It's the dumbest way of doing it. But I guess it works.
            for (auto i = POS_SLEEPING; i <= POS_STANDING; i++) {
                /* allows : Sleeping, Resting, Sitting, Fighting, Standing */
                if (iequals(arg, position_types[i])) {
                    position = i;
                    break;
                }
            }
        }
        return position_types[position];
    }

    if(lmember == "prac") {
        if(!arg.empty()) {
            int addition = atof(arg.c_str());
            modPractices(addition);
        }
        return fmt::format("{}", GET_PRACTICES(this));
    }

    if(lmember == "plr") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(player_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::PC, flag) ? "1" : "0";
    }

    if(lmember == "pref") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(preference_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::Pref, flag) ? "1" : "0";
    }

    if(lmember == "room") {
        if(auto r = getRoom(); r) return r;
        return "";
    }

    if(lmember == "race") return race::getName(race);

    if(lmember == "rpp") {
        if(!arg.empty()) {
            int addition = atof(arg.c_str());
            modRPP(addition);
        }
        return fmt::format("{}", getRPP());
    }

    if(lmember == "sex") return genders[(int)GET_SEX(this)];

    if(lmember == "size") {
        if(!arg.empty()) {
            auto ns = search_block((char*)arg.c_str(), size_names, false);
            if(ns > -1) setSize(ns);
        }
        return size_names[getSize()];
    }

    if(lmember == "skill") return skill_percent(this, (char*)arg.c_str());

    if(lmember == "skillset") {
        if(!arg.empty()) {
            char skillname[MAX_INPUT_LENGTH], *amount;
            amount = one_word((char*)arg.c_str(), skillname);
            skip_spaces(&amount);
            if (amount && *amount && is_number(amount)) {
                int skillnum = find_skill_num(skillname, SKTYPE_SKILL);
                if (skillnum > 0) {
                    int new_value = std::clamp<double>(atof(amount), 0, 100);
                    SET_SKILL(this, skillnum, new_value);
                }
            }
        }
        return "";
    }

    if(lmember == "tnl") return fmt::format("{}", level_exp(this, GET_LEVEL(this)+1));

    if(lmember == "vnum") {
        if(!arg.empty()) {
            auto v = atoll(arg.c_str());
            return vn == v ? "1":"0";
        }
        return fmt::format("{}", vn);
    }

    if(lmember == "varexists") return script->hasVar(arg) ? "1" : "0";

    // nothing left to do but try global variables...
    if(script->hasVar(lmember)) {
        return script->getVar(lmember);
    } else {
        script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                               GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), lmember.c_str());
    }

    return "";
}

int64_t BaseCharacter::getCurHealth() {
    return getCurPL();
}

int64_t BaseCharacter::getMaxHealth() {
    return getMaxPL();
}

double BaseCharacter::getCurHealthPercent() {
    return getCurPLPercent();
}

int64_t BaseCharacter::getPercentOfCurHealth(double amt) {
    return getPercentOfCurPL(amt);
}

int64_t BaseCharacter::getPercentOfMaxHealth(double amt) {
    return getPercentOfMaxPL(amt);
}

bool BaseCharacter::isFullHealth() {
    return isFullPL();
}

int64_t BaseCharacter::setCurHealth(int64_t amt) {
    return 0;
}

int64_t BaseCharacter::setCurHealthPercent(double amt) {
    return 0;
}

int64_t BaseCharacter::incCurHealth(int64_t amt, bool limit_max) {
    if (limit_max)
        health = std::min(1.0, health + (double) std::abs(amt) / (double) getEffMaxPL());
    else
        health += (double) std::abs(amt) / (double) getEffMaxPL();
    return getCurHealth();
};

int64_t BaseCharacter::decCurHealth(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getEffMaxPL();
    if (suppression > 0)
        health = std::max(fl, health - (double) std::abs(amt) / ((double) getEffMaxPL() * ((double) suppression / 100.0)));
    else
        health = std::max(fl, health - (double) std::abs(amt) / (double) getEffMaxPL());
    return getCurHealth();
}

int64_t BaseCharacter::incCurHealthPercent(double amt, bool limit_max) {
    if (limit_max)
        health = std::min(1.0, health + std::abs(amt));
    else
        health += std::abs(amt);
    return getCurHealth();
}

int64_t BaseCharacter::decCurHealthPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getEffMaxPL();
    health = std::max(fl, health - std::abs(amt));
    return getCurHealth();
}

void BaseCharacter::restoreHealth(bool announce) {
    if (!isFullHealth()) health = 1;
}

int64_t BaseCharacter::getMaxPLTrans() {
    auto total = getEffBasePL();

    total += (getAffectModifier(APPLY_ALL_VITALS) + getAffectModifier(APPLY_HIT));
    total *= (1.0 + getAffectModifier(APPLY_VITALS_MULT) + getAffectModifier(APPLY_PL_MULT));
    return total;
}

int64_t BaseCharacter::getMaxPL() {
    auto total = getMaxPLTrans();
    if (GET_KAIOKEN(this) > 0) {
        total += (total / 10) * GET_KAIOKEN(this);
    }
    if (AFF_FLAGGED(this, AFF_METAMORPH)) {
        total += (total * .6);
    }
    return total;
}

int64_t BaseCharacter::getCurPL() {
    if (!IS_NPC(this) && suppression > 0) {
        return getEffMaxPL() * std::min(health, health * ((double) suppression / 100));
    } else {
        return getEffMaxPL() * health;
    }
}

int64_t BaseCharacter::getUnsuppressedPL() {
        return getEffMaxPL() * health;
}

int64_t BaseCharacter::getEffBasePL() {
    if (original) return original->getEffBasePL();

    if (!clones.empty()) {
        return getBasePL() / (clones.size() + 1);
    } else {
        return getBasePL();
    }
}

int64_t BaseCharacter::getBasePL() {
    return get(CharStat::PowerLevel);
}

double BaseCharacter::getCurPLPercent() {
    return (double) getCurPL() / (double) getMaxPL();
}

int64_t BaseCharacter::getPercentOfCurPL(double amt) {
    return getCurPL() * std::abs(amt);
}

int64_t BaseCharacter::getPercentOfMaxPL(double amt) {
    return getMaxPL() * std::abs(amt);
}

bool BaseCharacter::isFullPL() {
    return health >= 1.0;
}

int64_t BaseCharacter::getCurKI() {
    return getMaxKI() * energy;
}

int64_t BaseCharacter::getMaxKI() {
    auto total = getEffBaseKI();
    total += (getAffectModifier(APPLY_ALL_VITALS) + getAffectModifier(APPLY_MANA));
    total *= (1.0 + getAffectModifier(APPLY_VITALS_MULT) + getAffectModifier(APPLY_KI_MULT));
    return total;
}

int64_t BaseCharacter::getEffBaseKI() {
    if (original) return original->getEffBaseKI();
    if (!clones.empty()) {
        return getBaseKI() / (clones.size() + 1);
    } else {
        return getBaseKI();
    }
}

int64_t BaseCharacter::getBaseKI() {
    return get(CharStat::Ki);
}

double BaseCharacter::getCurKIPercent() {
    return (double) getCurKI() / (double) getMaxKI();
}

int64_t BaseCharacter::getPercentOfCurKI(double amt) {
    return getCurKI() * std::abs(amt);
}

int64_t BaseCharacter::getPercentOfMaxKI(double amt) {
    return getMaxKI() * std::abs(amt);
}

bool BaseCharacter::isFullKI() {
    return energy >= 1.0;
}

int64_t BaseCharacter::setCurKI(int64_t amt) {
    return 0;
}

int64_t BaseCharacter::setCurKIPercent(double amt) {
    return 0;
}

int64_t BaseCharacter::incCurKI(int64_t amt, bool limit_max) {
    if (limit_max)
        energy = std::min(1.0, energy + (double) std::abs(amt) / (double) getMaxKI());
    else
        energy += (double) std::abs(amt) / (double) getMaxKI();
    return getCurKI();
};

int64_t BaseCharacter::decCurKI(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    energy = std::max(fl, energy - (double) std::abs(amt) / (double) getMaxKI());
    return getCurKI();
}

int64_t BaseCharacter::incCurKIPercent(double amt, bool limit_max) {
    if (limit_max)
        energy = std::min(1.0, energy + std::abs(amt));
    else
        energy += std::abs(amt);
    return getCurKI();
}

int64_t BaseCharacter::decCurKIPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxKI();
    energy = std::max(fl, energy - std::abs(amt));
    return getCurKI();
}


void BaseCharacter::restoreKI(bool announce) {
    if (!isFullKI()) energy = 1;
}

int64_t BaseCharacter::getCurST() {
    return getMaxST() * stamina;
}

int64_t BaseCharacter::getMaxST() {
    auto total = getEffBaseST();
    total += (getAffectModifier(APPLY_ALL_VITALS) + getAffectModifier(APPLY_MOVE));
    total *= (1.0 + getAffectModifier(APPLY_VITALS_MULT) + getAffectModifier(APPLY_ST_MULT));
    return total;
}

int64_t BaseCharacter::getEffBaseST() {
    if (original) return original->getEffBaseST();
    if (!clones.empty()) {
        return getBaseST() / (clones.size() + 1);
    } else {
        return getBaseST();
    }
}

int64_t BaseCharacter::getBaseST() {
    return get(CharStat::Stamina);
}

double BaseCharacter::getCurSTPercent() {
    return (double) getCurST() / (double) getMaxST();
}

int64_t BaseCharacter::getPercentOfCurST(double amt) {
    return getCurST() * std::abs(amt);
}

int64_t BaseCharacter::getPercentOfMaxST(double amt) {
    return getMaxST() * std::abs(amt);
}

bool BaseCharacter::isFullST() {
    return stamina >= 1;
}

int64_t BaseCharacter::setCurST(int64_t amt) {
    return 0;
}

int64_t BaseCharacter::setCurSTPercent(double amt) {
    return 0;
}

int64_t BaseCharacter::incCurST(int64_t amt, bool limit_max) {
    if (limit_max)
        stamina = std::min(1.0, stamina + (double) std::abs(amt) / (double) getMaxST());
    else
        stamina += (double) std::abs(amt) / (double) getMaxST();
    return getCurST();
};

int64_t BaseCharacter::decCurST(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    stamina = std::max(fl, stamina - (double) std::abs(amt) / (double) getMaxST());
    return getCurST();
}

int64_t BaseCharacter::incCurSTPercent(double amt, bool limit_max) {
    if (limit_max)
        stamina = std::min(1.0, stamina + std::abs(amt));
    else
        stamina += std::abs(amt);
    return getMaxST();
}

int64_t BaseCharacter::decCurSTPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxST();
    stamina = std::max(fl, stamina - std::abs(amt));
    return getCurST();
}


void BaseCharacter::restoreST(bool announce) {
    if (!isFullST()) stamina = 1;
}


int64_t BaseCharacter::getCurLF() {
    return getMaxLF() * life;
}

int64_t BaseCharacter::getMaxLF() {
    auto lb = GET_LIFEBONUSES(this);

    return (IS_DEMON(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.75) + lb
                           : (IS_KONATSU(this) ? (((GET_MAX_MANA(this) * 0.5) + (GET_MAX_MOVE(this) * 0.5)) * 0.85) +
                    lb : (GET_MAX_MANA(this) * 0.5) +
                                                                         (GET_MAX_MOVE(this) * 0.5) +
                    lb));
}

double BaseCharacter::getCurLFPercent() {
    return life;
}

int64_t BaseCharacter::getPercentOfCurLF(double amt) {
    return getCurLF() * std::abs(amt);
}

int64_t BaseCharacter::getPercentOfMaxLF(double amt) {
    return getMaxLF() * std::abs(amt);
}

bool BaseCharacter::isFullLF() {
    return life >= 1.0;
}

int64_t BaseCharacter::setCurLF(int64_t amt) {
    life = std::max<int64_t>(0L, std::abs(amt));
    return getCurLF();
}

int64_t BaseCharacter::setCurLFPercent(double amt) {
    life = std::max<int64_t>(0L, (int64_t) (getMaxLF() * std::abs(amt)));
    return getCurLF();
}

int64_t BaseCharacter::incCurLF(int64_t amt, bool limit_max) {
    if (limit_max)
        life = std::min(1.0, stamina + (double) std::abs(amt) / (double) getMaxLF());
    else
        life += (double) std::abs(amt) / (double) getMaxLF();
    return getCurLF();
};

int64_t BaseCharacter::decCurLF(int64_t amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    life = std::max(fl, life - (double) std::abs(amt) / (double) getMaxLF());
    return getCurLF();
}

int64_t BaseCharacter::incCurLFPercent(double amt, bool limit_max) {
    if (limit_max)
        life = std::min(1.0, life + std::abs(amt));
    else
        life += std::abs(amt);
    return getCurLF();
}

int64_t BaseCharacter::decCurLFPercent(double amt, int64_t floor) {
    auto fl = 0.0;
    if (floor > 0)
        fl = (double) floor / (double) getMaxLF();
    life = std::max(fl, life - std::abs(amt));
    return getCurLF();
}


void BaseCharacter::restoreLF(bool announce) {
    if (!isFullLF()) life = 1;
}


bool BaseCharacter::isFullVitals() {
    return isFullHealth() && isFullKI() && isFullST();
}

void BaseCharacter::restoreVitals(bool announce) {
    restoreHealth(announce);
    restoreKI(announce);
    restoreST(announce);
}

void BaseCharacter::restoreStatus(bool announce) {
    cureStatusKnockedOut(announce);
    cureStatusBurn(announce);
    cureStatusPoison(announce);
}

void BaseCharacter::setStatusKnockedOut() {
    setFlag(FlagType::Affect, AFF_KNOCKED);
    clearFlag(FlagType::Affect,AFF_FLYING);
    altitude = 0;
    GET_POS(this) = POS_SLEEPING;
}

void BaseCharacter::cureStatusKnockedOut(bool announce) {
    if (AFF_FLAGGED(this, AFF_KNOCKED)) {
        if (announce) {
            ::act("@W$n@W is no longer senseless, and wakes up.@n", false, this, nullptr, nullptr, TO_ROOM);
            this->sendf("You are no longer knocked out, and wake up!@n\r\n");
        }

        if (CARRIED_BY(this)) {
            if (GET_ALIGNMENT(CARRIED_BY(this)) > 50) {
                carry_drop(CARRIED_BY(this), 0);
            } else {
                carry_drop(CARRIED_BY(this), 1);
            }
        }

        clearFlag(FlagType::Affect,AFF_KNOCKED);
        GET_POS(this) = POS_SITTING;
    }
}

void BaseCharacter::cureStatusBurn(bool announce) {
    if (AFF_FLAGGED(this, AFF_BURNED)) {
        if (announce) {
            this->sendf("Your burns are healed now.\r\n");
            ::act("$n@w's burns are now healed.@n", true, this, nullptr, nullptr, TO_ROOM);
        }
        clearFlag(FlagType::Affect,AFF_BURNED);
    }
}

void BaseCharacter::cureStatusPoison(bool announce) {
    ::act("@C$n@W suddenly looks a lot better!@b", false, this, nullptr, nullptr, TO_NOTVICT);
    affect_from_char(this, SPELL_POISON);
}

static std::map<int, std::string> limb_names = {
        {0, "right arm"},
        {1, "left arm"},
        {2, "right leg"},
        {3, "left leg"}
};

void BaseCharacter::restoreLimbs(bool announce) {
    // restore head...
    GET_LIMBCOND(this, 0) = 100;

    // limbs...
    for (const auto &l: limb_names) {
        if (announce) {
            if (GET_LIMBCOND(this, l.first) <= 0)
                this->sendf("Your %s grows back!\r\n", l.second.c_str());
            else if (GET_LIMBCOND(this, l.first) < 50)
                this->sendf("Your %s is no longer broken!\r\n", l.second.c_str());
        }
        GET_LIMBCOND(this, l.first) = 100;
    }

    // and lastly, tail.
    this->gainTail(announce);
}


void BaseCharacter::gainTail(bool announce) {
    if (!race::hasTail(race)) return;
    if(checkFlag(FlagType::PC, PLR_TAIL)) return;
    setFlag(FlagType::PC, PLR_TAIL);
    if(announce) {
        this->sendf("@wYour tail grows back.@n\r\n");
        act("$n@w's tail grows back.@n", true, this, nullptr, nullptr, TO_ROOM);
    }
}

void BaseCharacter::loseTail() {
    if (!checkFlag(FlagType::PC, PLR_TAIL)) return;
    clearFlag(FlagType::PC, PLR_TAIL);
    remove_limb(this, 6);
    GET_TGROWTH(this) = 0;
    oozaru_revert(this);
}

bool BaseCharacter::hasTail() {
    return checkFlag(FlagType::PC, PLR_TAIL);
}

void BaseCharacter::addTransform(FormID form) {
    transforms.insert({form, trans_data()});
}

void BaseCharacter::hideTransform(FormID form, bool hide) {
    auto foundForm = transforms.find(form);
    foundForm->second.visible = !hide;
}

bool BaseCharacter::removeTransform(FormID form) {
    if (transforms.contains(form))
    {
        transforms.erase(form);
        return true;
    }
    //Return if failure
    return false;
}

int64_t BaseCharacter::gainBasePL(int64_t amt, bool trans_mult) {
    return mod(CharStat::PowerLevel, amt);
}

int64_t BaseCharacter::gainBaseST(int64_t amt, bool trans_mult) {
    return mod(CharStat::Stamina, amt);
}

int64_t BaseCharacter::gainBaseKI(int64_t amt, bool trans_mult) {
    return mod(CharStat::Ki, amt);
}

void BaseCharacter::gainBaseAll(int64_t amt, bool trans_mult) {
    gainBasePL(amt, trans_mult);
    gainBaseKI(amt, trans_mult);
    gainBaseST(amt, trans_mult);
}

int64_t BaseCharacter::loseBasePL(int64_t amt, bool trans_mult) {
    return mod(CharStat::PowerLevel, -amt);
}

int64_t BaseCharacter::loseBaseST(int64_t amt, bool trans_mult) {
    return mod(CharStat::Stamina, -amt);
}

int64_t BaseCharacter::loseBaseKI(int64_t amt, bool trans_mult) {
    return mod(CharStat::Ki, -amt);
}

void BaseCharacter::loseBaseAll(int64_t amt, bool trans_mult) {
    loseBasePL(amt, trans_mult);
    loseBaseKI(amt, trans_mult);
    loseBaseST(amt, trans_mult);
}

int64_t BaseCharacter::gainBasePLPercent(double amt, bool trans_mult) {
    return gainBasePL(get(CharStat::PowerLevel) * amt, trans_mult);
}

int64_t BaseCharacter::gainBaseKIPercent(double amt, bool trans_mult) {
    return gainBaseKI(get(CharStat::Ki) * amt, trans_mult);
}

int64_t BaseCharacter::gainBaseSTPercent(double amt, bool trans_mult) {
    return gainBaseST(get(CharStat::Stamina) * amt, trans_mult);
}

int64_t BaseCharacter::loseBasePLPercent(double amt, bool trans_mult) {
    return loseBasePL(get(CharStat::PowerLevel) * amt, trans_mult);
}

int64_t BaseCharacter::loseBaseKIPercent(double amt, bool trans_mult) {
    return loseBaseKI(get(CharStat::Ki) * amt, trans_mult);
}

int64_t BaseCharacter::loseBaseSTPercent(double amt, bool trans_mult) {
    return loseBaseST(get(CharStat::Stamina) * amt, trans_mult);
}

void BaseCharacter::gainBaseAllPercent(double amt, bool trans_mult) {
    gainBasePLPercent(amt, trans_mult);
    gainBaseKIPercent(amt, trans_mult);
    gainBaseSTPercent(amt, trans_mult);
}

void BaseCharacter::loseBaseAllPercent(double amt, bool trans_mult) {
    loseBasePLPercent(amt, trans_mult);
    loseBaseKIPercent(amt, trans_mult);
    loseBaseSTPercent(amt, trans_mult);
}


double BaseCharacter::getMaxCarryWeight() {
    return (getWeight() + 100.0) + (getMaxPL() / 200.0) + (GET_STR(this) * 50) + (IS_BARDOCK(this) ? 10000.0 : 0.0);
}

double BaseCharacter::getEquippedWeight() {
    double total_weight = 0;

    for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(this, i)) {
            total_weight += GET_OBJ_WEIGHT(GET_EQ(this, i));
        }
    }
    return total_weight;
}

double BaseCharacter::getCarriedWeight() {
    return getEquippedWeight() + getInventoryWeight() + (carrying ? carrying->getTotalWeight() : 0);
}

double BaseCharacter::getAvailableCarryWeight() {
    return getMaxCarryWeight() - getCarriedWeight();
}

double BaseCharacter::speednar() {
    auto ratio = (double) getCarriedWeight() / (double) getMaxCarryWeight();
    if (ratio >= .05)
        return std::max(0.01, std::min(1.0, 1.0 - ratio));
    return 1.0;
}

int64_t BaseCharacter::getEffMaxPL() {
    if (IS_NPC(this)) {
        return getMaxPL();
    }
    return getMaxPL() * speednar();
}

bool BaseCharacter::isWeightedPL() {
    return getMaxPL() > getEffMaxPL();
}

void BaseCharacter::apply_kaioken(int times, bool announce) {
    GET_KAIOKEN(this) = times;
    clearFlag(FlagType::PC, PLR_POWERUP);

    if (announce) {
        this->sendf("@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", times);
        ::act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, this, nullptr,
              nullptr, TO_ROOM);
    }

}

void BaseCharacter::remove_kaioken(int8_t announce) {
    auto kaio = GET_KAIOKEN(this);
    if (!kaio) {
        return;
    }
    GET_KAIOKEN(this) = 0;

    switch (announce) {
        case 1:
            this->sendf("You drop out of kaioken.\r\n");
            ::act("$n@w drops out of kaioken.@n", true, this, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            this->sendf("You lose focus and your kaioken disappears.\r\n");
            ::act("$n loses focus and $s kaioken aura disappears.", true, this, nullptr, nullptr, TO_ROOM);
    }
}


int BaseCharacter::getRPP() {
    if(IS_NPC(this)) {
        return 0;
    }

    auto p = players[uid];

    return p->account->rpp;

}

void account_data::modRPP(int amt) {
    rpp += amt;
    if(rpp < 0) {
        rpp = 0;
    }
}

void BaseCharacter::modRPP(int amt) {
    if(IS_NPC(this)) {
        return;
    }

    auto p = players[uid];

    p->account->modRPP(amt);
}

int BaseCharacter::getPractices() {
    return practice_points;
}

void BaseCharacter::modPractices(int amt) {
    practice_points += amt;
    if(practice_points < 0) {
        practice_points = 0;
    }
}


void BaseCharacter::login() {
    enter_player_game(desc);
    this->sendf("%s", CONFIG_WELC_MESSG);
    ::act("$n has entered the game.", true, this, nullptr, nullptr, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(this)), true, "%s has entered the game.", GET_NAME(this));
    /*~~~ For PCOUNT and HIGHPCOUNT ~~~*/
    auto count = 0;
    auto oldcount = HIGHPCOUNT;
    struct descriptor_data *k;

    for (k = descriptor_list; k; k = k->next) {
        if (!IS_NPC(k->character) && GET_LEVEL(k->character) > 3) {
            count += 1;
        }

        if (count > PCOUNT) {
            PCOUNT = count;
        }

        if (PCOUNT >= HIGHPCOUNT) {
            oldcount = HIGHPCOUNT;
            HIGHPCOUNT = PCOUNT;
            PCOUNTDATE = ::time(nullptr);
        }

    }

    time.logon = ::time(nullptr);
    greet_mtrigger(this, -1);
    greet_memory_mtrigger(this);

    STATE(desc) = CON_PLAYING;
    if (PCOUNT < HIGHPCOUNT && PCOUNT >= HIGHPCOUNT - 4) {
        payout(0);
    }
    if (PCOUNT == HIGHPCOUNT) {
        payout(1);
    }
    if (PCOUNT > oldcount) {
        payout(2);
    }

    /*~~~ End PCOUNT and HIGHPCOUNT ~~~*/
    if (GET_LEVEL(this) == 0) {
        do_start(this);
        this->sendf("%s", CONFIG_START_MESSG);
    }
    if (GET_ROOM_VNUM(IN_ROOM(this)) <= 1 && GET_LOADROOM(this) != NOWHERE) {
        removeFromLocation();
        addToLocation(getWorld(GET_LOADROOM(this)));
    } else if (GET_ROOM_VNUM(IN_ROOM(this)) <= 1) {
        removeFromLocation();
        addToLocation(getWorld(300));
    } else {
        lookAtLocation();
    }
    if (has_mail(GET_IDNUM(this)))
        this->sendf("\r\nYou have mail waiting.\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWIMM > GET_BOARD(this, 1))
        this->sendf(
                     "\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWCOD > GET_BOARD(this, 2))
        this->sendf(
                     "\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWBUI > GET_BOARD(this, 4))
        this->sendf(
                     "\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWDUO > GET_BOARD(this, 3))
        this->sendf(
                     "\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
    if (BOARDNEWMORT > GET_BOARD(this, 0))
        this->sendf("\r\n@GThere is a new bulletin board post.@n\r\n");
    if (NEWSUPDATE > GET_LPLAY(this))
        this->sendf(
                     "\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n",
                     LASTNEWS);

    if (LASTINTEREST != 0 && LASTINTEREST > GET_LINTEREST(this)) {
        int diff = (LASTINTEREST - GET_LINTEREST(this));
        int mult = 0;
        while (diff > 0) {
            if ((diff - 86400) < 0 && mult == 0) {
                mult = 1;
            } else if ((diff - 86400) >= 0) {
                diff -= 86400;
                mult++;
            } else {
                diff = 0;
            }
        }
        if (mult > 3) {
            mult = 3;
        }
        GET_LINTEREST(this) = LASTINTEREST;
        if (GET_BANK_GOLD(this) > 0) {
            int inc = ((GET_BANK_GOLD(this) / 100) * 2);
            if (inc >= 7500) {
                inc = 7500;
            }
            inc *= mult;
            set(CharMoney::Bank, inc);
            this->sendf("Interest happened while you were away, %d times.\r\n"
                                       "@cBank Interest@D: @Y%s@n\r\n", mult, add_commas(inc).c_str());
        }
    }

    if (!IS_ANDROID(this)) {
        char buf3[MAX_INPUT_LENGTH];
        send_to_sense(0, "You sense someone appear suddenly", this);
        sprintf(buf3,
                "@D[@GBlip@D]@Y %s\r\n@RSomeone has suddenly entered your scouter detection range!@n.",
                add_commas(GET_HIT(this)).c_str());
        send_to_scouter(buf3, this, 0, 0);
    }

    desc->has_prompt = 0;
    /* We've updated to 3.1 - some bits might be set wrongly: */
    clearFlag(FlagType::Pref, PRF_BUILDWALK);
    if (!GET_EQ(this, WEAR_WIELD1) && PLR_FLAGGED(this, PLR_THANDW)) {
        clearFlag(FlagType::PC, PLR_THANDW);
    }

}

double BaseCharacter::getAffectModifier(int location, int specific) {
    double total = 0;
    for(auto a = affected; a; a = a->next) {
        if(location != a->location) continue;
        if(specific != -1 && specific != a->specific) continue;
        total += a->modifier;
    }
    for(auto i = 0; i < NUM_WEARS; i++) {
        if(auto obj = GET_EQ(this, i); obj)
        total += obj->getAffectModifier(location, specific);
    }

    total += race::getModifier(this, location, specific);
    total += trans::getModifier(this, location, specific);

    return total;
}

align_t BaseCharacter::get(CharAlign type) {
    if(auto find = aligns.find(type); find != aligns.end()) {
        return find->second;
    }
    return 0;
}

align_t BaseCharacter::set(CharAlign type, align_t val) {
    return aligns[type] = std::clamp<align_t>(val, -1000, 1000);
}

align_t BaseCharacter::mod(CharAlign type, align_t val) {
    return set(type, get(type) + val);
}

appearance_t BaseCharacter::get(CharAppearance type) {
    if(auto find = appearances.find(type); find != appearances.end()) {
        return find->second;
    }
    return 0;
}

appearance_t BaseCharacter::set(CharAppearance type, appearance_t val) {
    return appearances[type] = std::clamp<appearance_t>(val, 0, 100);
}

appearance_t BaseCharacter::mod(CharAppearance type, appearance_t val) {
    return set(type, get(type) + val);
}

int BaseCharacter::setSize(int val) {
    this->size = val;
    return this->size;
}

int BaseCharacter::getSize() {
    return size != SIZE_UNDEFINED ? size : race::getSize(race);
}


money_t BaseCharacter::get(CharMoney mon) {
    if(auto find = moneys.find(mon); find != moneys.end()) {
        return find->second;
    }
    return 0;
}

money_t BaseCharacter::set(CharMoney mon, money_t val) {
    return moneys[mon] = std::min<money_t>(val, 999999999999);
}

money_t BaseCharacter::mod(CharMoney mon, money_t val) {
    return set(mon, get(mon) + val);
}


attribute_t BaseCharacter::get(CharAttribute attr, bool base) {
    attribute_t val = 0;
    if(auto stat = attributes.find(attr); stat != attributes.end()) {
        val = stat->second;
    }
    if(!base) {
        val += getAffectModifier((int)attr+1) + getAffectModifier(APPLY_ALL_ATTRS);
        return std::clamp<attribute_t>(val, 5, 100);
    }
    return val;

}

attribute_t BaseCharacter::set(CharAttribute attr, attribute_t val) {
    return attributes[attr] = std::clamp<attribute_t>(val, 0, 80);
}

attribute_t BaseCharacter::mod(CharAttribute attr, attribute_t val) {
    return set(attr, get(attr) + val);
}

attribute_train_t BaseCharacter::get(CharTrain attr) {
    if(auto t = trains.find(attr); t != trains.end()) {
        return t->second;
    }
    return 0;
}

attribute_train_t BaseCharacter::set(CharTrain attr, attribute_train_t val) {
    return trains[attr] = std::max<attribute_train_t>(0, val);
}

attribute_train_t BaseCharacter::mod(CharTrain attr, attribute_train_t val) {
    return set(attr, get(attr) + val);
}


num_t BaseCharacter::get(CharNum stat) {
    if(auto st = nums.find(stat); st != nums.end()) {
        return st->second;
    }
    return 0;
}

num_t BaseCharacter::set(CharNum stat, num_t val) {
    return nums[stat] = val;
}

num_t BaseCharacter::mod(CharNum stat, num_t val) {
    return set(stat, get(stat) + val);
}

stat_t BaseCharacter::set(CharStat type, stat_t val) {
    return stats[type] = std::max<stat_t>(0, val);
}

stat_t BaseCharacter::mod(CharStat type, stat_t val) {
    return set(type, get(type) + val);
}

stat_t BaseCharacter::get(CharStat type, bool base) {
    if(auto st = stats.find(type); st != stats.end()) {
        return st->second;
    }
    return 0;
}


bool BaseCharacter::canCarryWeight(weight_t val) {
    double gravity = myEnvVar(EnvVar::Gravity);
    return getAvailableCarryWeight() >= (val * gravity);
}

bool BaseCharacter::canCarryWeight(Object *obj) {
    return canCarryWeight(obj->getTotalWeight());
}

bool BaseCharacter::canCarryWeight(BaseCharacter *obj) {
    return canCarryWeight(obj->getTotalWeight());
}

weight_t BaseCharacter::getCurrentBurden() {
    auto total = getTotalWeight();
    auto gravity = myEnvVar(EnvVar::Gravity);
    return total * gravity;
}

double BaseCharacter::getBurdenRatio() {
    auto total = getCurrentBurden();
    auto max = getMaxCarryWeight();
    if(max == 0) return 0;
    return total / max;
}

std::string BaseCharacter::juggleRaceName(bool capitalized) {

    auto apparent = race;

    switch (apparent) {
        case RaceID::Hoshijin:
            if (mimic) apparent = *mimic;
            break;
        case RaceID::Halfbreed:
            switch (RACIAL_PREF(this)) {
                case 1:
                    apparent = RaceID::Human;
                    break;
                case 2:
                    apparent = RaceID::Saiyan;
                    break;
            }
            break;
        case RaceID::Android:
            switch (RACIAL_PREF(this)) {
                case 1:
                    apparent = RaceID::Android;
                    break;
                case 2:
                    apparent = RaceID::Human;
                    break;
                case 3:
                    if (capitalized) {
                        return robot;
                    } else {
                        return robot_lower;
                    }
            }
            break;
        case RaceID::Saiyan:
            if (PLR_FLAGGED(this, PLR_TAILHIDE)) {
                apparent = RaceID::Human;
            }
            break;
    }

    if (capitalized) {
        return race::getName(apparent);
    } else {
        auto out = race::getName(apparent);
        to_lower(out);
        return out;
    }
}

void BaseCharacter::restore_by(BaseCharacter *ch) {
    this->restore(true);

    ::act("You have been fully healed by $N!", false, this, nullptr, ch, TO_CHAR | TO_SLEEP);
}

void BaseCharacter::restore(bool announce) {
    restoreVitals(announce);
    restoreLimbs(announce);
    restoreStatus(announce);
    restoreLF(announce);
}

void BaseCharacter::resurrect(ResurrectionMode mode) {
    // First, fully heal the character.
    restore(true);
    for(auto f : {AFF_ETHEREAL, AFF_SPIRIT}) clearFlag(FlagType::Affect,f);
    clearFlag(FlagType::PC, PLR_PDEATH);
    // Send them to their starting room and have them 'look'.
    removeFromLocation();
    if (GET_DROOM(this) != NOWHERE && GET_DROOM(this) != 0 && GET_DROOM(this) != 1) {
        addToLocation(getWorld(GET_DROOM(this)));
    } else {
        addToLocation(getWorld(sensei::getStartRoom(chclass)));
    }
    lookAtLocation();

    // If Costless, there's not going to be any penalties.
    int dur = 100;
    switch (mode) {
        case Costless:
            return;
        case Basic:
            if (dcount >= 8 && dcount < 10) {
                dur = 90;
            } else if (dcount >= 5 && dcount < 8) {
                dur = 75;
            } else if (dcount >= 3 && dcount < 5) {
                dur = 60;
            } else if (dcount >= 1 && dcount < 3) {
                dur = 40;
            }
            break;
        case RPP:
            dur = 100;
            break;
    }

    // Also no penalties if the character isn't at least level 10.
    if (GET_LEVEL(this) > 9) {
        int losschance = axion_dice(0);
        this->sendf(
                     "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength,itution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        std::map<CharAttribute, int> statReductions = {
            {CharAttribute::Strength, -8},
            {CharAttribute::Constitution, -8},
            {CharAttribute::Intelligence, -8},
            {CharAttribute::Wisdom, -8},
            {CharAttribute::Speed, -8},
            {CharAttribute::Agility, -8}
        };

        for (auto& [attr, reduction] : statReductions) {
            int baseStat = this->get(attr, true);
            int effectiveReduction = std::max(reduction, 15 - baseStat);
            statReductions[attr] = effectiveReduction;
        }

        assign_affect(this, AFF_WEAKENED_STATE, SKILL_WARP, dur,
                      statReductions[CharAttribute::Strength],
                      statReductions[CharAttribute::Constitution],
                      statReductions[CharAttribute::Intelligence],
                      statReductions[CharAttribute::Agility],
                      statReductions[CharAttribute::Wisdom],
                      statReductions[CharAttribute::Speed]);

        if (losschance >= 100) {
            int psloss = rand_number(100, 300);
            modPractices(-psloss);
            this->sendf("@R...and a loss of @r%d@R PS!@n", psloss);
        }
    }
    GET_DTIME(this) = 0;
    ::act("$n's body forms in a pool of @Bblue light@n.", true, this, nullptr, nullptr, TO_ROOM);
}

void BaseCharacter::ghostify() {
    restore(true);
    for(auto f : {AFF_SPIRIT, AFF_ETHEREAL, AFF_KNOCKED, AFF_SLEEP, AFF_PARALYZE}) clearFlag(FlagType::Affect,f);

    // upon death, ghost-bodies gain new natural limbs... unless they're a
    // cyborg and want to keep their implants.
    if (!PRF_FLAGGED(this, PRF_LKEEP)) {
        for(auto f : {PLR_CLLEG, PLR_CRLEG, PLR_CLARM, PLR_CRARM}) clearFlag(FlagType::PC, f);
    }

}

void BaseCharacter::teleport_to(IDXTYPE rnum) {
    removeFromLocation();
    auto r = getWorld<Room>(rnum);
    addToLocation(r);
    lookAtLocation();
    update_pos(this);
}

bool BaseCharacter::in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum) {
    return GET_ROOM_VNUM(IN_ROOM(this)) >= low_rnum && GET_ROOM_VNUM(IN_ROOM(this)) <= high_rnum;
}

bool BaseCharacter::in_past() {
    return ROOM_FLAGGED(IN_ROOM(this), ROOM_PAST);
}

bool BaseCharacter::is_newbie() {
    return GET_LEVEL(this) < 9;
}

bool BaseCharacter::in_northran() {
    return in_room_range(17900, 17999);
}

static std::map<int, uint16_t> grav_threshold = {
        {10,    5000},
        {20,    20000},
        {30,    50000},
        {40,    100000},
        {50,    200000},
        {100,   400000},
        {200,   1000000},
        {300,   5000000},
        {400,   8000000},
        {500,   15000000},
        {1000,  25000000},
        {5000,  100000000},
        {10000, 200000000}
};

int64_t BaseCharacter::calc_soft_cap() {
    auto level = get(CharNum::Level);
    if(level >= 100) return 5e9;
    return race::getSoftCap(race, level);
}

bool BaseCharacter::is_soft_cap(int64_t type) {
    return is_soft_cap(type, 1.0);
}

bool BaseCharacter::is_soft_cap(int64_t type, long double mult) {
    if (IS_NPC(this))
        return true;

    // Level 100 characters are never softcapped.
    if (get(CharNum::Level) >= 100) {
        return false;
    }
    auto cur_cap = calc_soft_cap() * mult;

    int64_t against = 0;

    switch (type) {
        case 0:
            against = (getBasePL());
            break;
        case 1:
            against = (getBaseKI());
            break;
        case 2:
            against = (getBaseST());
            break;
    }

    return against >= cur_cap;
}

int BaseCharacter::wearing_android_canister() {
    if (!IS_ANDROID(this))
        return 0;
    auto obj = GET_EQ(this, WEAR_BACKPACK);
    if (!obj)
        return 0;
    switch (GET_OBJ_VNUM(obj)) {
        case 1806:
            return 1;
        case 1807:
            return 2;
        default:
            return 0;
    }
}

int64_t BaseCharacter::calcGravCost(int64_t num) {
    double gravity = myEnvVar(EnvVar::Gravity);
    int64_t cost = (gravity * gravity);

    if (!num) {
        if (cost) {
            this->sendf("You sweat bullets struggling against a mighty burden.\r\n");
        }
        if ((this->getCurST()) > cost) {
            this->decCurST(cost);
            return 1;
        } else {
            this->decCurST(cost);
            return 0;
        }
    } else {
        return (this->getCurST()) > (cost + num);
    }
}

weight_t BaseCharacter::getWeight(bool base) {
    auto total = weight;

    if(!base) {
        total += getAffectModifier(APPLY_CHAR_WEIGHT);
        total *= (1.0 + getAffectModifier(APPLY_WEIGHT_MULT));
    }

    return total;
}

int BaseCharacter::getHeight(bool base) {
    int total = get(CharNum::Height);

    if(!base) {
        total += getAffectModifier(APPLY_CHAR_HEIGHT);
        total *= (1.0 + getAffectModifier(APPLY_HEIGHT_MULT));
    }

    return total;
}

int BaseCharacter::setHeight(int val) {
    return set(CharNum::Height, std::max(0, val));
}

int BaseCharacter::modHeight(int val) {
    return setHeight(getHeight(true) + val);
}

double BaseCharacter::getTotalWeight() {
    return getWeight() + getCarriedWeight();
}

bool BaseCharacter::isActive() {
    return active;
}

double BaseCharacter::currentGravity() {
    return myEnvVar(EnvVar::Gravity);
}

Object* BaseCharacter::findObject(const std::function<bool(Object*)> &func, bool working) {
    auto o = GameEntity::findObject(func, working);
    if(o) return o;

    for(auto [pos, obj] : getEquipment()) {
        if(working && !obj->isWorking()) continue;
        if(func(obj)) return obj;
        auto p = obj->findObject(func, working);
        if(p) return p;
    }

    return nullptr;
}

std::set<Object*> BaseCharacter::gatherObjects(const std::function<bool(Object*)> &func, bool working) {
    auto out = GameEntity::gatherObjects(func, working);

    for(auto [pos, obj] : getEquipment()) {
        if(working && !obj->isWorking()) continue;
        if(func(obj)) out.insert(obj);
        auto contents = obj->gatherObjects(func, working);
        out.insert(contents.begin(), contents.end());
    }
    return out;
}

void BaseCharacter::ageBy(double addedTime) {
    this->time.secondsAged += addedTime;
}

void BaseCharacter::setAge(double newAge) {
    this->time.secondsAged = newAge * SECS_PER_GAME_YEAR;
}

std::string BaseCharacter::renderDiagnostics(GameEntity* viewer) {
    static struct {
        int percent;
        const char *text;
    } diagnosis[] = {
            {100, "@wis in @Gexcellent@w condition."},
            {90,  "@whas a few @Rscratches@w."},
            {80,  "@whas some small @Rwounds@w and @Rbruises@w."},
            {70,  "@whas quite a few @Rwounds@w."},
            {60,  "@whas some big @rnasty wounds@w and @Rscratches@w."},
            {50,  "@wlooks pretty @rhurt@w."},
            {40,  "@wis mainly @rinjured@w."},
            {30,  "@wis a @rmess@w of @rinjuries@w."},
            {20,  "@wis @rstruggling@w to @msurvive@w."},
            {10,  "@wis in @mawful condition@w."},
            {0,   "@Ris barely alive.@w"},
            {-1,  "@ris nearly dead.@w"},
    };
    int percent, ar_index;

    int64_t hit = GET_HIT(this), max = (getEffMaxPL());

    int64_t total = max;

    if (hit == total) {
        percent = 100;
    } else if (hit < total && hit >= (total * .9)) {
        percent = 90;
    } else if (hit < total && hit >= (total * .8)) {
        percent = 80;
    } else if (hit < total && hit >= (total * .7)) {
        percent = 70;
    } else if (hit < total && hit >= (total * .6)) {
        percent = 60;
    } else if (hit < total && hit >= (total * .5)) {
        percent = 50;
    } else if (hit < total && hit >= (total * .4)) {
        percent = 40;
    } else if (hit < total && hit >= (total * .3)) {
        percent = 30;
    } else if (hit < total && hit >= (total * .2)) {
        percent = 20;
    } else if (hit < total && hit >= (total * .1)) {
        percent = 10;
    } else if (hit < total * .1) {
        percent = 0;
    } else {
        percent = -1;        /* How could MAX_HIT be < 1?? */
    }

    for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
        if (percent >= diagnosis[ar_index].percent)
            break;

    return fmt::sprintf("%s\r\n", diagnosis[ar_index].text);
}



std::string BaseCharacter::renderAppearance(GameEntity* viewer) {
    std::string result;

    // getLookDesc() may be anything, for PCs. for NPCs, it's a fixed thing.
    if (auto ld = getLookDesc(); !ld.empty()) {
        result += fmt::sprintf("%s\r\n", ld);
    }

    if(IS_HUMANOID(this)) {
        if (GET_LIMBCOND(this, 0) >= 50 && !PLR_FLAGGED(this, PLR_CRARM)) {
            result += fmt::sprintf("            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(this, 0),
                        "%", "%");
        } else if (GET_LIMBCOND(this, 0) > 0 && !PLR_FLAGGED(this, PLR_CRARM)) {
            result += fmt::sprintf("            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                            GET_LIMBCOND(this, 0), "%", "%");
        } else if (GET_LIMBCOND(this, 0) > 0 && PLR_FLAGGED(this, PLR_CRARM)) {
            result += fmt::sprintf("            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                            GET_LIMBCOND(this, 0), "%", "%");
        } else if (GET_LIMBCOND(this, 0) <= 0) {
            result += fmt::sprintf("            @D[@cRight Arm   @D: @rMissing.            @D]@n\r\n");
        }
        if (GET_LIMBCOND(this, 1) >= 50 && !PLR_FLAGGED(this, PLR_CLARM)) {
            result += fmt::sprintf("            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(this, 1),
                            "%", "%");
        } else if (GET_LIMBCOND(this, 1) > 0 && !PLR_FLAGGED(this, PLR_CLARM)) {
            result += fmt::sprintf("            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                            GET_LIMBCOND(this, 1), "%", "%");
        } else if (GET_LIMBCOND(this, 1) > 0 && PLR_FLAGGED(this, PLR_CLARM)) {
            result += fmt::sprintf("            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                            GET_LIMBCOND(this, 1), "%", "%");
        } else if (GET_LIMBCOND(this, 1) <= 0) {
            result += fmt::sprintf("            @D[@cLeft Arm    @D: @rMissing.            @D]@n\r\n");
        }
        if (GET_LIMBCOND(this, 2) >= 50 && !PLR_FLAGGED(this, PLR_CLARM)) {
            result += fmt::sprintf("            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(this, 2),
                            "%", "%");
        } else if (GET_LIMBCOND(this, 2) > 0 && !PLR_FLAGGED(this, PLR_CRLEG)) {
            result += fmt::sprintf("            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                            GET_LIMBCOND(this, 2), "%", "%");
        } else if (GET_LIMBCOND(this, 2) > 0 && PLR_FLAGGED(this, PLR_CRLEG)) {
            result += fmt::sprintf("            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                            GET_LIMBCOND(this, 2), "%", "%");
        } else if (GET_LIMBCOND(this, 2) <= 0) {
            result += fmt::sprintf("            @D[@cRight Leg   @D: @rMissing.            @D]@n\r\n");
        }
        if (GET_LIMBCOND(this, 3) >= 50 && !PLR_FLAGGED(this, PLR_CLLEG)) {
            result += fmt::sprintf("            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(this, 3),
                            "%", "%");
        } else if (GET_LIMBCOND(this, 3) > 0 && !PLR_FLAGGED(this, PLR_CLLEG)) {
            result += fmt::sprintf("            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                            GET_LIMBCOND(this, 3), "%", "%");
        } else if (GET_LIMBCOND(this, 3) > 0 && PLR_FLAGGED(this, PLR_CLLEG)) {
            result += fmt::sprintf("            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                            GET_LIMBCOND(this, 3), "%", "%");
        } else if (GET_LIMBCOND(this, 3) <= 0) {
            result += fmt::sprintf("            @D[@cLeft Leg    @D: @rMissing.             @D]@n\r\n");
        }
        if (PLR_FLAGGED(this, PLR_HEAD)) {
            result += fmt::sprintf("            @D[@cHead        @D: @GHas.                 @D]@n\r\n");
        }
        if (!PLR_FLAGGED(this, PLR_HEAD)) {
            result += fmt::sprintf("            @D[@cHead        @D: @rMissing.             @D]@n\r\n");
        }
        if (race::hasTail(race) && !PLR_FLAGGED(this, PLR_TAILHIDE)) {
            if(PLR_FLAGGED(this, PLR_TAIL))
                result += fmt::sprintf("            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
            else
                result += fmt::sprintf("            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
        }
        result += fmt::sprintf("\r\n");
    }

    result += fmt::sprintf("\r\n         @D----------------------------------------@n\r\n");
    result += trans_check(this, viewer);
    result += fmt::sprintf("         @D----------------------------------------@n\r\n");
    result += fmt::sprintf("\r\n");

    auto heshe = HSSH(this);
    auto raceName = juggleRaceName(false);

    result += fmt::format("{} appears to be a {} {}, ", heshe, AN(raceName.c_str()), raceName);

    auto w = getWeight();
    int h = getHeight();
    auto wString = fmt::format("{}kg", w);
    result += fmt::sprintf("is %s sized, about %dcm tall,\r\nabout %s heavy,", size_names[get_size(this)],
                    h, wString.c_str());
    
    result += renderDiagnostics(viewer);

    if(!viewer->checkFlag(FlagType::Pref, PRF_NOEQSEE))
        if(auto eq = renderEquipment(viewer); !eq.empty()) {
            result += fmt::sprintf("\r\n");    /* act() does capitalization. */
            result += fmt::sprintf("They are using:\r\n");
            result += eq;
        }

    return result;
}

std::string BaseCharacter::renderStatusLines(GameEntity* viewer) {
    std::string result;

    auto icur = GET_HIT(this);
    auto imax = getEffMaxPL();

    std::vector<std::string> messages;

    // display current health approximation.
    auto heshe = HSSH(this);
    auto himher = HMHR(this);
    auto hisher = HSHR(this);

    if (icur >= (imax) * .9 && icur != (imax))
        messages.emplace_back(fmt::format("@R...Some slight wounds on {} body.@w", hisher));
    else if (icur >= (imax) * .8 && icur < (imax) * .9)
        messages.emplace_back(fmt::format("@R...A few wounds on {} body.@w", hisher));
    else if (icur >= (imax) * .7 && icur < (imax) * .8)
        messages.emplace_back(fmt::format("@R...Many wounds on {} body.@w", hisher));
    else if (icur >= (imax) * .6 && icur < (imax) * .7)
        messages.emplace_back(fmt::format("@R...Quite a few wounds on {} body.@w", hisher));
    else if (icur >= (imax) * .5 && icur < (imax) * .6)
        messages.emplace_back(fmt::format("@R...Horrible wounds on {} body.@w", hisher));
    else if (icur >= (imax) * .4 && icur < (imax) * .5)
        messages.emplace_back(fmt::format("@R...Blood is seeping from the wounds on {} body.@w", hisher));
    else if (icur >= (imax) * .3 && icur < (imax) * .4)
        messages.emplace_back(fmt::format("@R...{} body is in terrible shape.@w", hisher));
    else if (icur >= (imax) * .2 && icur < (imax) * .3)
        messages.emplace_back(fmt::format("@R...{} absolutely covered in wounds.@w", heshe));
    else if (icur >= (imax) * .1 && icur < (imax) * .2)
        messages.emplace_back(fmt::format("@R...Is on {} last leg.@w", hisher));
    else if (icur < (imax) * .1)
        messages.emplace_back("@R...Should be DEAD soon.@w");

    

    // current actions.
    if (GET_EAVESDROP(this) > 0) {
        messages.emplace_back(fmt::format("@w...{} is spying on everything to the @c{}@w.", heshe, dirs[GET_EAVESDIR(this)]));
    }
    // Maybe this should go to the main line and not a status modifier?
    if (PLR_FLAGGED(this, PLR_FISHING)) {
        messages.emplace_back(fmt::format("@w...{} is @Cfishing@w.@n", heshe));
    }
    if (PLR_FLAGGED(this, PLR_AURALIGHT)) {
        // TODO: implement aura-based-on-form and modifiers.
        messages.emplace_back(fmt::format("...is surrounded by a bright {} aura.", aura_types[GET_AURA(this)]));
    }

    auto is_oozaru = (form == FormID::Oozaru || form == FormID::GoldenOozaru);

    if(checkFlag(FlagType::Affect, AFF_FLYING)) {
        if (GET_ALT(this) == 1)
            messages.emplace_back(fmt::format("...{} is in the air!", heshe));
        if (GET_ALT(this) == 2)
            messages.emplace_back(fmt::format("...{} is high in the air!", heshe));
    }

    if(checkFlag(FlagType::Affect, AFF_SANCTUARY)) {
        if (GET_SKILL(this, SKILL_AQUA_BARRIER))
            messages.emplace_back(fmt::format("...{} has a @Gbarrier@w of @cwater@w and @Cki@w around {} body!", heshe, hisher));
        else
            messages.emplace_back(fmt::format("...{} has a barrier around {} body!", heshe, hisher));
    }

    if (checkFlag(FlagType::Affect, AFF_FIRESHIELD))
        messages.emplace_back(fmt::format("...{} has @rf@Rl@Ya@rm@Re@Ys@w around {} body!", heshe, hisher));
    if (PLR_FLAGGED(this, PLR_SPIRAL))
        messages.emplace_back(fmt::format("...{} is spinning in a vortex!", heshe));
    if (GET_CHARGE(this))
        messages.emplace_back(fmt::format("...{} has a bright {} aura around {} body!", heshe, aura_types[GET_AURA(this)], hisher));
    if (checkFlag(FlagType::Affect, AFF_METAMORPH))
        messages.emplace_back(fmt::format("@w...{} has a dark, @rred@w aura and menacing presence.", heshe));
    if (checkFlag(FlagType::Affect, AFF_HAYASA))
        messages.emplace_back(fmt::format("@w...{} has a soft @cblue@w glow around {} body!", heshe, hisher));
    if (checkFlag(FlagType::Affect, AFF_BLIND))
        messages.emplace_back(fmt::format("...{} is groping around blindly!", heshe));
    if (checkFlag(FlagType::Affect, AFF_KYODAIKA))
        messages.emplace_back(fmt::format("@w...{} has expanded {} body size@w!", heshe, hisher));

    // is this even USED?
    if (affected_by_spell(this, SPELL_FAERIE_FIRE))
        messages.emplace_back(fmt::format("@m...{} @mis outlined with purple fire!", heshe));

    if (checkFlag(FlagType::Affect, AFF_HEALGLOW))
        messages.emplace_back(fmt::format("@w...{} has a serene @Cblue@Y glow@w around {} body.", heshe, hisher));
    if (checkFlag(FlagType::Affect, AFF_EARMOR))
        messages.emplace_back(fmt::format("@w...{} has ghostly @Ggreen@w ethereal armor around {} body.", heshe, hisher));

    if (GET_KAIOKEN(this) > 0)
        messages.emplace_back(fmt::format("@w...@r{} has a red aura around {} body!", heshe, hisher));
    if (!isNPC() && PLR_FLAGGED(this, PLR_SPIRAL))
        messages.emplace_back(fmt::format("@w...{} is spinning in a vortex!", heshe));
    if (IS_TRANSFORMED(this) && !IS_ANDROID(this) && !IS_SAIYAN(this) && !IS_HALFBREED(this))
        messages.emplace_back(fmt::format("@w...{} has energy crackling around {} body!", heshe, hisher));
    if (GET_CHARGE(this) && !IS_SAIYAN(this) && !IS_HALFBREED(this)) {
        messages.emplace_back(fmt::format("@w...{} has a bright {} aura around {} body!", heshe, aura_types[GET_AURA(this)], hisher));
    }
    if (!is_oozaru && GET_CHARGE(this) && IS_TRANSFORMED(this) && (IS_SAIYAN(this) || IS_HALFBREED(this)))
        messages.emplace_back(fmt::format("@w...{} has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around {} body!", heshe, hisher));
    if (!is_oozaru && GET_CHARGE(this) && !IS_TRANSFORMED(this) && (IS_SAIYAN(this) || IS_HALFBREED(this))) {
        messages.emplace_back(fmt::format("@w...{} has a @Ybright@w {} aura around {} body!", heshe, aura_types[GET_AURA(this)], hisher));
    }
    if (form != FormID::Oozaru && !GET_CHARGE(this) && IS_TRANSFORMED(this) && (IS_SAIYAN(this) || IS_HALFBREED(this)))
        messages.emplace_back(fmt::format("@w...{} has energy crackling around {} body!", heshe, hisher));
    if (form == FormID::Oozaru && GET_CHARGE(this) && (IS_SAIYAN(this) || IS_HALFBREED(this)))
        messages.emplace_back(fmt::format("@w...{} is in the form of a @rgreat ape@w!", heshe));
    
    if (form == FormID::Oozaru && !GET_CHARGE(this) && (IS_SAIYAN(this) || IS_HALFBREED(this)))
        messages.emplace_back(fmt::format("@w...{} has energy crackling around {} @rgreat ape@w body!", heshe, hisher));
    if (GET_FEATURE(this)) {
        messages.emplace_back(fmt::format("@C{}@n", GET_FEATURE(this)));
    }

    return join(messages, "@w\r\n");
}

static const char *positions[] = {
    " is dead",
    " is mortally wounded",
    " is lying here, incapacitated",
    " is lying here, stunned",
    " is sleeping here",
    " is resting here",
    " is sitting here",
    "!FIGHTING!",
    " is standing here"
};

std::string BaseCharacter::renderRoomListingHelper(GameEntity* viewer) {
    std::string result;

    auto shd = renderRoomListName(viewer);

    // should an exclamation be used at the end of the line instead of a period?
    bool exclamation = false;

    std::vector<std::string> commaSeparated;
    auto spar = is_sparring(this);
    if (checkFlag(FlagType::Affect, AFF_INVISIBLE)) {
        commaSeparated.emplace_back("is invisible");
    }
    if(checkFlag(FlagType::Affect, AFF_ETHEREAL)) {
        commaSeparated.emplace_back("has a halo");
    }
    if(checkFlag(FlagType::Affect, AFF_HIDE)) {
        commaSeparated.emplace_back("is hiding");
    }
    if(PLR_FLAGGED(this, PLR_WRITING)) {
        commaSeparated.emplace_back("is writing");
    }
    if (PRF_FLAGGED(this, PRF_BUILDWALK)) {
        commaSeparated.emplace_back("is buildwalking");
    }
    if(auto fight = FIGHTING(this); fight) {
        exclamation = true;
        if(spar) {
            if(fight == viewer) {
                commaSeparated.emplace_back("is sparring with YOU");
            } else {
                commaSeparated.emplace_back(fmt::sprintf("is sparring with %s", fight->getDisplayName(viewer)));
            }
        } else {
            if(fight == viewer) {
                commaSeparated.emplace_back("is fighting YOU");
            } else {
                commaSeparated.emplace_back(fmt::sprintf("is fighting %s", fight->getDisplayName(viewer)));
            }
        }
    }
    if(auto grap = GRAPPLED(this); grap) {
        exclamation = true;
        if(grap == viewer) {
            commaSeparated.emplace_back("is being grappled with by YOU");
        } else {
            commaSeparated.emplace_back(fmt::sprintf("is being grappled with by %s", grap->getDisplayName(viewer)));
        }
    }
    if(auto abs = ABSORBBY(this); abs) {
        exclamation = true;
        if(abs == viewer) {
            commaSeparated.emplace_back("is being absorbed from by YOU");
        } else {
            commaSeparated.emplace_back(fmt::sprintf("is being absorbed from by %s", abs->getDisplayName(viewer)));
        }
    }
    if(auto carry = CARRYING(this); carry) {
        if(carry == viewer) {
            commaSeparated.emplace_back("is carrying YOU");
        } else {
            commaSeparated.emplace_back(fmt::sprintf("is carrying %s", carry->getDisplayName(viewer)));
        }
    }
    if(auto carryby = CARRIED_BY(this); carryby) {
        if(carryby == viewer) {
            commaSeparated.emplace_back("is being carried by YOU");
        } else {
            commaSeparated.emplace_back(fmt::sprintf("is being carried by %s", carryby->getDisplayName(viewer)));
        }
    }
    if (auto chair = SITS(this); chair) {
        if (PLR_FLAGGED(this, PLR_HEALT)) {
            commaSeparated.emplace_back("is floating inside a healing tank");
        } else if (PLR_FLAGGED(this, PLR_PILOTING)) {
            commaSeparated.emplace_back("is sitting in the pilot's chair");
        } else {
            commaSeparated.emplace_back(fmt::format("{} on {}", positions[(int) GET_POS(this)], chair->getDisplayName(viewer)));
        }
    }

    // If there are commaSeparated sections, we want result to open up as:
    // <name>, who <value, value, value>, and <last value>
    // But there might only be one value, or maybe none at all.
    if(commaSeparated.empty()) {
        if(isNPC()) {
            result = getRoomDesc();
        } else {
            // TODO: Add a custom posture for Player characters?
            result = fmt::sprintf("@w%s is standing here.", shd);
        }
    } else if(commaSeparated.size() == 1) {
        result += fmt::sprintf("@w%s, who %s%s", shd, commaSeparated[0], exclamation ? "!" : ".");
    } else {
        for(size_t i = 0; i < commaSeparated.size(); ++i) {
            if(i == commaSeparated.size() - 1) {
                result += fmt::sprintf("and %s%s", commaSeparated[i], exclamation ? "!" : ".");
            } else {
                result += fmt::sprintf("%s, ", commaSeparated[i]);
            }
        }
    }

    // Lastly, we will append any suffix states, if needed.
    // There shouldn't be many of these.
    // TODO: Add a new Linkdead detection, the old one was dumb.
    std::vector<std::string> states;

    if (viewer->checkFlag(FlagType::Affect, AFF_DETECT_ALIGN)) {
        if (IS_EVIL(this))
            states.emplace_back("(@rRed@[3] Aura)@n");
        else if (IS_GOOD(this))
            states.emplace_back("(@bBlue@[3] Aura)@n");
    }
    if (checkFlag(FlagType::Pref, PRF_AFK))
        states.emplace_back("@D(@RAFK@D)@n");
    else if (timer > 3)
        states.emplace_back("@D(@RIDLE@D)@n");

    if(!states.empty()) {
        result += " ";
        result += join(states, " ");
    }

    return result;
}

std::string BaseCharacter::renderRoomListingFor(GameEntity* viewer) {
    std::vector<std::string> results;
    results.emplace_back(renderListPrefixFor(viewer));
    if(IS_MAJIN(this) && checkFlag(FlagType::Affect, AFF_LIQUEFIED)) {
        results.emplace_back(fmt::sprintf("@wSeveral blobs of %s colored goo spread out here.@n\n", skin_types[(int)GET_SKIN(this)]));
    } else {
        results.emplace_back(renderRoomListingHelper(viewer));
        results.emplace_back(renderStatusLines(viewer));
    }
    
    return join(results, "@n ") + "@n";
}