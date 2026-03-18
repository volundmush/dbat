#include "dbat/game/DgScriptPrototype.hpp"
#include <nlohmann/json.hpp>

std::map<trig_vnum, std::shared_ptr<DgScriptPrototype>> trig_index; /* index table for triggers      */

void to_json(nlohmann::json &j, const DgScriptPrototype &t)
{
    j["vn"] = t.vn;
    j["name"] = t.name;
    j["attach_type"] = t.attach_type;
    j["trigger_type"] = t.trigger_type;
    j["narg"] = t.narg;
    j["arglist"] = t.arglist;
    j["body"] = t.scriptString();
}

void from_json(const nlohmann::json &j, DgScriptPrototype &t)
{
    if (j.contains(+"vn"))
        t.vn = j["vn"].get<int>();
    if (j.contains(+"name"))
        t.name = strdup(j["name"].get<std::string>().c_str());
    if (j.contains(+"attach_type"))
        t.attach_type = j["attach_type"].get<UnitType>();
    if (j.contains(+"trigger_type"))
        t.trigger_type = j["trigger_type"].get<int>();
    if (j.contains(+"narg"))
        t.narg = j["narg"].get<int>();
    if (j.contains(+"arglist"))
        t.arglist = strdup(j["arglist"].get<std::string>().c_str());
    if (j.contains(+"body"))
        t.setBody(j["body"].get<std::string>());
}
