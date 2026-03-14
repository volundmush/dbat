#include "dbat/game/HasMisc.hpp"
#include "dbat/util/FilterWeak.hpp"


vnum HasVnum::getVnum() const
{
    return vn;
}

std::string HasProtoScript::scriptString() const
{
    std::vector<std::string> vnums;
    for (auto p : proto_script)
        vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}
