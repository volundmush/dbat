#pragma once
#include <cstdint>
#include <tuple>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <magic_enum/magic_enum_format.hpp>
#include <fmt/format.h>

#include "const/UnitType.h"

#include "Typedefs.h"

enum class ScriptLineType : std::uint8_t {
    COMMAND = 0,
    IF = 1,
    ELSEIF = 2,
    ELSE = 3,
    END = 4,
    SWITCH = 5,
    CASE = 6,
    BREAK = 7,
    DEFAULT = 8,
    WHILE = 9,
    DONE = 10,
    COMMENT = 11
};

using ScriptLine = std::tuple<ScriptLineType, std::string>;

struct DgScriptPrototype {
    trig_vnum vn{NOTHING};
    UnitType attach_type{UnitType::unknown};            /* mob/obj/wld intentions          */
    std::string name{};                    /* name of trigger                 */
    long trigger_type{};            /* type of trigger (for bitvector) */
    std::vector<ScriptLine> lines; /* list of commands in trigger     */
    int narg{};                /* numerical argument              */
    std::string arglist{};            /* argument list                   */

    ScriptLine getLine(int line) const;

    std::string scriptString() const;
    void setBody(const std::string& body);
};

inline std::string format_as(const DgScriptPrototype& dg) {
    return fmt::format("({}) DgScriptPrototype {} '{}'", magic_enum::enum_name(dg.attach_type), dg.vn, dg.name);
}

extern std::map<trig_vnum, std::shared_ptr<DgScriptPrototype>> trig_index;