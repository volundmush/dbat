local dbat = require("dbat")
local T = require("lua.libs.pcommand_toggles")

return {
    id = "slowns",
    aliases = { { "slowns", 6 } },
    can_execute = T.admin_can(T.ADMLVL.IMPL),
    execute = function(ctx)
        local on = dbat.config.nameserver_slow_toggle()
        ctx.ch:send(on
            and "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"
            or "Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n")
    end,
}
