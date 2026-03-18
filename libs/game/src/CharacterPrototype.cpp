#include "dbat/game/CharacterPrototype.hpp"
#include <nlohmann/json.hpp>

std::map<mob_vnum, std::shared_ptr<CharacterPrototype>> mob_proto;

void to_json(nlohmann::json &j, const CharacterPrototype &c)
{
    to_json(j, static_cast<const CharacterBase &>(c));
    to_json(j, static_cast<const HasProtoScript&>(c));
}

void from_json(const nlohmann::json &j, CharacterPrototype &c)
{
    from_json(j, static_cast<CharacterBase &>(c));
    from_json(j, static_cast<HasProtoScript&>(c));
    if (c.race != Race::human)
        c.affect_flags.set(AFF_INFRAVISION, true);
    c.mob_flags.set(MOB_NOTDEADYET, false);
}
