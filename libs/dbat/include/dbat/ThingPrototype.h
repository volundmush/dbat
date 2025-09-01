#pragma once
#include "templates.h"

struct ThingPrototype {
    virtual ~ThingPrototype();
    vnum vn{NOTHING};
    char *name{};
    char *room_description{};      /* When thing is listed in room */
    char *look_description{};      /* what to show when looked at */
    char *short_description{};     /* when displayed in list or action message. */
    struct extra_descr_data *ex_description{}; /* extra descriptions     */
    std::vector<trig_vnum> proto_script; /* list of default triggers  */
    FlagHandler<AffectFlag> affect_flags{}; /* To set affect bits          */
    std::unordered_map<std::string, double> stats;

    std::string scriptString() const;

    ThingPrototype& operator=(const ThingPrototype& other);
};