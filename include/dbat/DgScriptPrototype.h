#pragma once
#include "sysdep.h"
#include "defs.h"

enum class ScriptLineType : uint8_t {
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

template <>
struct fmt::formatter<DgScriptPrototype> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const DgScriptPrototype& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "({}) DgScript {} '{}'", z.attach_type, z.vn, z.name);
    }
};