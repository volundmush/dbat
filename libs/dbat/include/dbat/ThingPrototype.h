#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "Typedefs.h"
#include "const/AffectFlag.h"
#include "Flags.h"
#include "HasExtraDescriptions.h"


struct ThingPrototype : public HasExtraDescriptions {
    virtual ~ThingPrototype();
    vnum vn{NOTHING};
    char *name{};
    char *room_description{};      /* When thing is listed in room */
    char *look_description{};      /* what to show when looked at */
    char *short_description{};     /* when displayed in list or action message. */
    std::vector<trig_vnum> proto_script; /* list of default triggers  */
    FlagHandler<AffectFlag> affect_flags{}; /* To set affect bits          */
    std::unordered_map<std::string, double> stats;

    std::string scriptString() const;

    ThingPrototype& operator=(const ThingPrototype& other);
};