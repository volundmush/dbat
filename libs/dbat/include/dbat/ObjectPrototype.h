#pragma once
#include "ObjectShared.h"
#include "ThingPrototype.h"
#include "HasPicky.h"
#include "affect.h"

struct ObjectPrototype : public ThingPrototype, public picky_data {
    ObjectPrototype() = default;
    ObjectPrototype(const Object& other);
    
    ObjectPrototype& operator=(const ObjectPrototype& other);

    ItemType type_flag{ItemType::unknown};      /* Type of item                        */
    std::array<affected_type, MAX_OBJ_AFFECT> affected;  /* affects */
    FlagHandler<WearFlag> wear_flags{}; /* Where you can wear it     */
    FlagHandler<ItemFlag> item_flags{}; /* If it hums, glows, etc.  */
    Size size{Size::medium};           /* Size class of object                */
    
    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return itemProtoStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return itemProtoStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return itemProtoStats.modBase<R>(this, stat, val);
    }
};

template <>
struct fmt::formatter<ObjectPrototype> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ObjectPrototype& z, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "ObjectPrototype {} '{}'", z.vn, z.short_description ? z.short_description : "<unnamed>");
    }
};