#pragma once
#include <nlohmann/json_fwd.hpp>

#include "DgScript.hpp"
#include "HasVariables.hpp"
#include "HasMisc.hpp"

struct HasDgScripts : public HasVariables {

    virtual const char* getDgName() const = 0;

    UnitType type{UnitType::unknown};

    virtual UnitType getDgUnitType() const = 0;
    virtual vnum getDgVnum() const = 0;
    virtual bool isActive() const = 0;

    long trigger_types{};   /* bitvector of trigger types */
    std::optional<std::vector<vnum>> running_scripts; /* list of attached scripts. the order matters. Only used if differs from proto scripts.*/
    std::unordered_map<trig_vnum, std::shared_ptr<DgScript>> scripts; /* list of attached triggers. accessed in order of running_scripts */

    void activateScripts();
    void deactivateScripts();
    std::vector<trig_vnum> getScriptOrder() const; /* this will return running_scripts if said, or the results of getProtoScripts() */
    std::vector<std::weak_ptr<DgScript>> getScripts() const;
    virtual std::vector<trig_vnum> getProtoScript() const = 0;
    std::string getDgScriptString() const;

    // generates the persistent UID which will identify this object when it's saved
    // to a variable and which hopefully survives a reboot.
    virtual std::string getUID(bool active = false) const = 0;

    virtual DgReturn dgCallMember(DgScript* sc, std::string_view field, std::string_view subfield) = 0;

};

void to_json(nlohmann::json& j, const HasDgScripts& p);
void from_json(const nlohmann::json& j, HasDgScripts& p);

inline std::string format_as(const HasDgScripts& unit) {
    std::vector<std::string> scripts;
    for(const auto& scw : unit.getScripts()) {
        if(auto sc = scw.lock()) {
            scripts.push_back(format_as_diagnostic(*sc));
        }
    }
    scripts.emplace_back(format_as(static_cast<const HasVariables&>(unit)));
    return fmt::format("DgScripts: {}", fmt::join(scripts, ", "));
}