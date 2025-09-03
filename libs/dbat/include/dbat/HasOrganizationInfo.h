#pragma once
#include <memory>
#include "Typedefs.h"
#include "Command.h"
#include "HasPicky.h"


struct HasOrganizationInfo : public HasPicky {
    int vnum{NOTHING};        /* Virtual number of this shop		*/

    mob_vnum keeper{NOBODY};                   /* GM's vnum */
    std::vector<std::weak_ptr<Character>> getKeepers();
    SpecialFunc func{};        /* Secondary spec_proc for keeper	*/
    std::string customerString();
};

using org_data = HasOrganizationInfo;