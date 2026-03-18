#pragma once
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include "Typedefs.hpp"
#include "Command.hpp"
#include "HasPicky.hpp"


struct HasOrganizationInfo : public HasPicky {
    int vnum{NOTHING};        /* Virtual number of this shop		*/

    mob_vnum keeper{NOBODY};                   /* GM's vnum */
    std::vector<std::weak_ptr<Character>> getKeepers();
    SpecialFunc func{};        /* Secondary spec_proc for keeper	*/
    std::string customerString();
};

using org_data = HasOrganizationInfo;

void to_json(nlohmann::json& j, const org_data& o);
void from_json(const nlohmann::json& j, org_data& o);
