local T = require("lua.libs.pcommand_toggles")
local dbat = require("dbat")

return {
    id = "buildwalk",
    aliases = { { "buildwalk", 9 } },
    can_execute = T.admin_can(T.ADMLVL.IMMORT),
    execute = function(ctx)
        local ch = ctx.ch
        local on = ch:pref_flag_toggle(T.PRF.BUILDWALK)
        dbat.log_imm_action(string.format("OLC: %s turned buildwalk %s.", ch:name_get(), on and "on" or "off"))
        ch:send(on and "Buildwalk On.\r\n" or "Buildwalk Off.\r\n")
    end,
}
