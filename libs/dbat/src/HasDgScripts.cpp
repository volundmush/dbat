#include "dbat/HasDgScripts.h"


std::vector<trig_vnum> HasDgScripts::getScriptOrder() const
{
    if (running_scripts.has_value())
    {
        return *running_scripts;
    }
    return getProtoScript();
}

std::vector<std::weak_ptr<DgScript>> HasDgScripts::getScripts() const
{
    std::vector<std::weak_ptr<DgScript>> out;
    auto proto = getScriptOrder();
    out.reserve(scripts.size() + proto.size());
    for (const auto &v : proto)
    {
        if (auto it = scripts.find(v); it != scripts.end())
        {
            out.push_back(it->second);
        }
        else
        {
            // basic_mud_log("Warning: script vnum %d not found in scripts map for %s", v, getDgName());
        }
    }
    return out;
}

std::string HasDgScripts::getDgScriptString() const
{
    std::vector<std::string> vnums;
    for (auto p : getProtoScript())
        vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

void HasDgScripts::activateScripts()
{
    for (auto &[vn, t] : scripts)
    {
        t->activate();
    }
}

void HasDgScripts::deactivateScripts()
{
    for (auto &[vn, t] : scripts)
    {
        t->deactivate();
    }
}