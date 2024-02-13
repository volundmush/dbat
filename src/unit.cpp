#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"

base_proto::~base_proto() {
    if(name) free(name);
    if(short_description) free(short_description);
    if(room_description) free(room_description);
    while(ex_description) {
        auto next = ex_description->next;
        delete ex_description;
        ex_description = next;
    }
}

extra_descr_data::extra_descr_data(const extra_descr_data& other) {
    if(other.keyword) keyword = strdup(other.keyword);
    if(other.description) description = strdup(other.description);
    if(other.next) next = new extra_descr_data(*other.next);
}

base_proto::base_proto(const base_proto& other) {
    vn = other.vn;
    if(other.name) name = strdup(other.name);
    if(other.short_description) short_description = strdup(other.short_description);
    if(other.look_description) look_description = strdup(other.look_description);
    if(other.room_description) room_description = strdup(other.room_description);
    proto_script = other.proto_script;
    if(other.ex_description) ex_description = new extra_descr_data(*other.ex_description);
}

base_proto& base_proto::operator=(const base_proto& other) {
    if(this == &other) return *this;

    if(name) free(name);
    if(short_description) free(short_description);
    if(look_description) free(look_description);
    if(room_description) free(room_description);
    while(ex_description) {
        auto next = ex_description->next;
        delete ex_description;
        ex_description = next;
    }

    vn = other.vn;
    if(other.name) name = strdup(other.name);
    if(other.short_description) short_description = strdup(other.short_description);
    if(other.look_description) look_description = strdup(other.look_description);
    if(other.room_description) room_description = strdup(other.room_description);
    proto_script = other.proto_script;
    if(other.ex_description) ex_description = new extra_descr_data(*other.ex_description);

    return *this;
}

npc_proto::npc_proto(const npc_proto& other) : base_proto(other) {
    race = other.race;
    chclass = other.chclass;
    weight = other.weight;
    nums = other.nums;
    mob_specials = other.mob_specials;
    size = other.size;
    attributes = other.attributes;
    appearances = other.appearances;
    moneys = other.moneys;
    aligns = other.aligns;
    affected_by = other.affected_by;
    stats = other.stats;
    playerFlags = other.playerFlags;
    mobFlags = other.mobFlags;
    armor = other.armor;
    damage_mod = other.damage_mod;
    speaking = other.speaking;
    transforms = other.transforms;
}

npc_proto& npc_proto::operator=(const npc_proto& other) {
    if(this == &other) return *this;

    base_proto::operator=(other);

    race = other.race;
    chclass = other.chclass;
    weight = other.weight;
    nums = other.nums;
    mob_specials = other.mob_specials;
    size = other.size;
    attributes = other.attributes;
    appearances = other.appearances;
    moneys = other.moneys;
    aligns = other.aligns;
    affected_by = other.affected_by;
    stats = other.stats;
    playerFlags = other.playerFlags;
    mobFlags = other.mobFlags;
    armor = other.armor;
    damage_mod = other.damage_mod;
    speaking = other.speaking;
    transforms = other.transforms;
}

item_proto::item_proto(const item_proto& other) : base_proto(other) {
    type_flag = other.type_flag;
    level = other.level;
    wear_flags = other.wear_flags;
    extra_flags = other.extra_flags;
    weight = other.weight;
    cost = other.cost;
    cost_per_day = other.cost_per_day;
    bitvector = other.bitvector;
    size = other.size;
    affected = other.affected;
    timer = other.timer;
}

item_proto& item_proto::operator=(const item_proto& other) {
    if(this == &other) return *this;

    base_proto::operator=(other);

    type_flag = other.type_flag;
    level = other.level;
    wear_flags = other.wear_flags;
    extra_flags = other.extra_flags;
    weight = other.weight;
    cost = other.cost;
    cost_per_day = other.cost_per_day;
    bitvector = other.bitvector;
    size = other.size;
    affected = other.affected;
    timer = other.timer;

    return *this;
}

std::vector<obj_data *> unit_data::getInventory() {
    std::vector<obj_data*> out;
    for(auto u = contents; u; u = u->next_content) {
        auto o = dynamic_cast<obj_data*>(u);
        if(o && o->locationType == 0) out.push_back(o);
    }
    return out;
}

std::unordered_map<int, obj_data*> unit_data::getEquipment() {
    std::unordered_map<int, obj_data*> out;
    for(auto u = contents; u; u = u->next_content) {
        auto o = dynamic_cast<obj_data*>(u);
        if(o && o->locationType > 0) out[o->locationType] = o;
    }
    return out;
}

unit_data* unit_data::getLocation() {
    if(auto found = world.find(location); found != world.end()) return found->second;
    return nullptr;
}

room_data* unit_data::getRoom() {
    return dynamic_cast<room_data*>(getLocation());
}

room_data* unit_data::getAbsoluteRoom() {
    if(auto room = getRoom(); room) {
        return room;
    }
    if(auto loc = getLocation(); loc) return loc->getAbsoluteRoom();
    return nullptr;
}

nlohmann::json base_proto::serialize() {
    nlohmann::json j;
    if(vn != NOTHING) j["vn"] = vn;
    if(name && strlen(name)) j["name"] = name;
    if(short_description && strlen(short_description)) j["short_description"] = short_description;
    if(look_description && strlen(look_description)) j["look_description"] = look_description;
    if(room_description && strlen(room_description)) j["room_description"] = room_description;
    for(auto ex = ex_description; ex; ex = ex->next) {
        if(ex->keyword && strlen(ex->keyword) && ex->description && strlen(ex->description)) {
            j["ex_description"].push_back(std::make_pair(ex->keyword, ex->description));
        }
    }
    for(auto t : proto_script) j["proto_script"].push_back(t);
    return j;
}

void base_proto::deserialize(const nlohmann::json& j) {
    if(j.contains("vn")) vn = j["vn"];
    if(j.contains("name")) {
        if(name) free(name);
        name = strdup(j["name"].get<std::string>().c_str());
    }
    if(j.contains("short_description")) {
        if(short_description) free(short_description);
        short_description = strdup(j["short_description"].get<std::string>().c_str());
    }
    if(j.contains("look_description")) {
        if(look_description) free(look_description);
        look_description = strdup(j["look_description"].get<std::string>().c_str());
    }
    if(j.contains("room_description")) {
        if(room_description) free(room_description);
        room_description = strdup(j["room_description"].get<std::string>().c_str());
    }
    if(j.contains("ex_description")) {
        // free existing ones.
        while(ex_description) {
            auto next = ex_description->next;
            delete ex_description;
            ex_description = next;
        }
        auto &e = j["ex_description"];
        for(auto ex = e.rbegin(); ex != e.rend(); ex++) {
            auto new_ex = new extra_descr_data();
            new_ex->keyword = strdup((*ex)[0].get<std::string>().c_str());
            new_ex->description = strdup((*ex)[1].get<std::string>().c_str());
            new_ex->next = ex_description;
            ex_description = new_ex;
        }
    }
    if(j.contains("proto_script")) {
        for(auto t : j["proto_script"]) proto_script.push_back(t);
    }
}

nlohmann::json npc_proto::serialize() {
    nlohmann::json j = base_proto::serialize();
    j["race"] = race;
    j["chclass"] = chclass;
    if(weight != 0.0) j["weight"] = weight;
    for(auto &[id, attr] : attributes) if(attr) j["attributes"].push_back(std::make_pair(id, attr));
    for(auto &[id, mon] : moneys) if(mon) j["moneys"].push_back(std::make_pair(id, mon));

    for(auto &[id, align] : aligns) if(align) j["aligns"].push_back(std::make_pair(id, align));

    for(auto &[id, app] : appearances) if(app) j["appearances"].push_back(std::make_pair(id, app));

    for(auto &[id, app] : stats) if(app) j["stats"].push_back(std::make_pair(id, app));

    for(auto &[id, app] : nums) if(app) j["nums"].push_back(std::make_pair(id, app));

    for(auto i = 0; i < mobFlags.size(); i++) if(mobFlags.test(i)) j["mobFlags"].push_back(i);

    for(auto i = 0; i < playerFlags.size(); i++) if(playerFlags.test(i)) j["playerFlags"].push_back(i);

    for(auto i = 0; i < affected_by.size(); i++) if(affected_by.test(i)) j["affected_by"].push_back(i);
    
    if(armor) j["armor"] = armor;
    if(damage_mod) j["damage_mod"] = damage_mod;
    if(speaking) j["speaking"] = speaking;

    return j;
}

void npc_proto::deserialize(const nlohmann::json& j) {
    base_proto::deserialize(j);
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

    if(j.contains("race")) race = j["race"].get<RaceID>();

    if(j.contains("chclass")) chclass = static_cast<SenseiID>(std::min(14, j["chclass"].get<int>()));

    if(j.contains("weight")) weight = j["weight"];
    if(j.contains("mobFlags")) for(auto &i : j["mobFlags"]) mobFlags.set(i.get<int>());
    if(j.contains("playerFlags")) for(auto &i : j["playerFlags"]) playerFlags.set(i.get<int>());
    
    if(j.contains("affected_by"))
        for(auto &i : j["affected_by"])
            affected_by.set(i.get<int>());
    if(j.contains("armor")) armor = j["armor"];
    if(j.contains("damage_mod")) damage_mod = j["damage_mod"];
    if(j.contains("speaking")) speaking = j["speaking"];
}

npc_proto::npc_proto(const nlohmann::json& j) {
    deserialize(j);
}

nlohmann::json item_proto::serialize() {
    nlohmann::json j = base_proto::serialize();
    for(auto i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
        if(value[i]) j["value"].push_back(std::make_pair(i, value[i]));
    }
    j["type_flag"] = type_flag;
    j["level"] = level;
    for(auto i = 0; i < wear_flags.size(); i++)
        if(wear_flags.test(i)) j["wear_flags"].push_back(i);

    for(auto i = 0; i < extra_flags.size(); i++)
        if(extra_flags.test(i)) j["extra_flags"].push_back(i);

    if(weight != 0.0) j["weight"] = weight;
    if(cost != 0) j["cost"] = cost;
    if(cost_per_day != 0) j["cost_per_day"] = cost_per_day;
    for(auto i = 0; i < bitvector.size(); i++)
        if(bitvector.test(i)) j["bitvector"].push_back(i);
    if(size) j["size"] = size;
    if(timer) j["timer"] = timer;
    for(auto & i : affected) {
        if(i.location == APPLY_NONE) continue;
        j["affected"].push_back(i.serialize());
    }
    return j;
}

void item_proto::deserialize(const nlohmann::json& j) {
    base_proto::deserialize(j);
    if(j.contains("value")) {
        for(auto j2 : j["value"]) {
            auto id = j2[0].get<int>();
            value[id] = j2[1].get<int>();
        }
    }
    if(j.contains("type_flag")) type_flag = j["type_flag"];
    if(j.contains("level")) level = j["level"];
    if(j.contains("wear_flags")) for(auto &i : j["wear_flags"]) wear_flags.set(i.get<int>());
    if(j.contains("extra_flags")) for(auto &i : j["extra_flags"]) extra_flags.set(i.get<int>());
    if(j.contains("weight")) weight = j["weight"];
    if(j.contains("cost")) cost = j["cost"];
    if(j.contains("cost_per_day")) cost_per_day = j["cost_per_day"];
    if(j.contains("bitvector")) for(auto &i : j["bitvector"]) bitvector.set(i.get<int>());
    if(j.contains("size")) size = j["size"];
    if(j.contains("timer")) timer = j["timer"];
    if(j.contains("affected")) {
        int counter = 0;
        for(auto & i : j["affected"]) {
            affected[counter].deserialize(i);
            counter++;
        }
    }
}

nlohmann::json unit_data::serialize() {
    nlohmann::json j;

    if(vn != NOTHING) j["vn"] = vn;

    if(name && strlen(name)) j["name"] = name;
    if(room_description && strlen(room_description)) j["room_description"] = room_description;
    if(look_description && strlen(look_description)) j["look_description"] = look_description;
    if(short_description && strlen(short_description)) j["short_description"] = short_description;

    for(auto ex = ex_description; ex; ex = ex->next) {
        if(ex->keyword && strlen(ex->keyword) && ex->description && strlen(ex->description)) {
            j["ex_description"].push_back(std::make_pair(ex->keyword, ex->description));
        }
    }

    if(uid != NOTHING) j["uid"] = uid;
    if(zone != NOTHING) j["zone"] = zone;

    if(script && (!script->dgScripts.empty() || !script->vars.empty())) {
        j["dgScripts"] = script->serialize();
    }

    return j;
}


void unit_data::deserialize(const nlohmann::json& j) {
    if(j.contains("vn")) vn = j["vn"];
    if(j.contains("name")) {
        if(name) free(name);
        name = strdup(j["name"].get<std::string>().c_str());
    }
    if(j.contains("room_description")) {
        if(room_description) free(room_description);
        room_description = strdup(j["room_description"].get<std::string>().c_str());
    }
    if(j.contains("look_description")) {
        if(look_description) free(look_description);
        look_description = strdup(j["look_description"].get<std::string>().c_str());
    }
    if(j.contains("short_description")) {
        if(short_description) free(short_description);
        short_description = strdup(j["short_description"].get<std::string>().c_str());
    }

    if(j.contains("ex_description")) {
        auto &e = j["ex_description"];
        for(auto ex = e.rbegin(); ex != e.rend(); ex++) {
            auto new_ex = new extra_descr_data();
            new_ex->keyword = strdup((*ex)[0].get<std::string>().c_str());
            new_ex->description = strdup((*ex)[1].get<std::string>().c_str());
            new_ex->next = ex_description;
            ex_description = new_ex;
        }
    }

    if(j.contains("uid")) uid = j["uid"];
    if(j.contains("zone")) zone = j["zone"];

    if(j.contains("dgScripts")) {
        if(!script) script = std::make_shared<script_data>(this);
        script->deserialize(j["dgScripts"]);
    }

}

void unit_data::activateContents() {
    for(auto obj = contents; obj; obj = obj->next_content) {
        obj->activate();
    }
}

void unit_data::deactivateContents() {
    for(auto obj = contents; obj; obj = obj->next_content) {
        obj->deactivate();
    }
}

std::string unit_data::scriptString() {
    return "";
}

std::string base_proto::scriptString() {
    std::vector<std::string> vnums;
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

double unit_data::getInventoryWeight() {
    double weight = 0;
    for(auto obj = contents; obj; obj = obj->next_content) {
        weight += obj->getTotalWeight();
    }
    return weight;
}

int64_t unit_data::getInventoryCount() {
    int64_t total = 0;
    for(auto obj = contents; obj; obj = obj->next_content) {
        total++;
    }
    return total;
}

struct obj_data* unit_data::findObject(const std::function<bool(struct obj_data*)> &func, bool working) {
    for(auto obj = contents; obj; obj = obj->next_content) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            return obj;
        }
        if(auto p = obj->findObject(func, working); p) return p;
    }
    return nullptr;
}

struct obj_data* unit_data::findObjectVnum(obj_vnum objVnum, bool working) {
    return findObject([objVnum](auto o) {return o->vn == objVnum;}, working);
}

std::set<struct obj_data*> unit_data::gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working) {
    std::set<struct obj_data*> out;
    for(auto obj = contents; obj; obj = obj->next_content) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            out.insert(obj);
        }
        auto contents = obj->gatherObjects(func, working);
        out.insert(contents.begin(), contents.end());
    }
    return out;
}

std::string unit_data::getUID(bool active) {
    return fmt::format("#{}{}", uid, active ? "" : "!");
}

DgResults unit_data::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    return "";
}

bool unit_data::isActive() {
    return false;
}

void unit_data::save() {

}

std::string unit_data::getName() {
    return name ? name : "";
}

void unit_data::setName(const std::string& n) {
    if(name) free(name);
    name = strdup(n.c_str());
}

std::string unit_data::getShortDesc() {
    return short_description ? short_description : "";
}

void unit_data::setShortDesc(const std::string& n) {
    if(short_description) free(short_description);
    short_description = strdup(n.c_str());
}

std::string unit_data::getRoomDesc() {
    return room_description ? room_description : "";
}

void unit_data::setRoomDesc(const std::string& n) {
    if(room_description) free(room_description);
    room_description = strdup(n.c_str());
}

std::string unit_data::getLookDesc() {
    return look_description ? look_description : "";
}

void unit_data::setLookDesc(const std::string& n) {
    if(look_description) free(look_description);
    look_description = strdup(n.c_str());
}


void unit_data::checkMyID() {
    if(uid == -1) {
        uid = getNextUID();
        basic_mud_log("Unit Found with ID -1. Automatically fixed to ID %d", uid);
    }
}

std::string unit_data::getDisplayName(struct char_data* ch) {
    return "Nameless";
}

std::string unit_data::renderAppearance(struct char_data* ch) {
    return "You see nothing special.";
}

std::vector<std::string> unit_data::getKeywords(struct char_data* ch) {
    return {};
}

extra_descr_data::~extra_descr_data() {
    if(keyword) free(keyword);
    if(description) free(description);
}

unit_data::~unit_data() {
    if(name) free(name);
    if(short_description) free(short_description);
    if(room_description) free(room_description);
    if(look_description) free(look_description);
    while(ex_description) {
        auto next = ex_description->next;
        delete ex_description;
        ex_description = next;
    }
    if(script) script.reset();
}


void unit_data::assignTriggers() {
    // does nothing.
}

bool unit_data::checkFlag(FlagType type, int flag) {
    if(auto foundType = flags.find(type); foundType != flags.end()) {
        return foundType->second.contains(flag);
    }
    return false;
}

void unit_data::setFlag(FlagType type, int flag, bool value) {
    auto &f = flags[type];
    if(value) {
        f.insert(flag);
    } else {
        f.erase(flag);
    }
}

void unit_data::clearFlag(FlagType type, int flag) {
    setFlag(type, flag, false);
}

bool unit_data::flipFlag(FlagType type, int flag) {
    auto &f = flags[type];
    if(f.contains(flag)) {
        f.erase(flag);
        return false;
    } else {
        f.insert(flag);
        return true;
    }
}


nlohmann::json unit_data::serializeLocation() {
    nlohmann::json j = nlohmann::json::object();

    if(location != NOWHERE) {
        j["location"] = location;
        if(locationType) j["locationType"] = locationType;
    }

    return j;
}

nlohmann::json unit_data::serializeRelations() {
    nlohmann::json j = nlohmann::json::object();


    return j;
}