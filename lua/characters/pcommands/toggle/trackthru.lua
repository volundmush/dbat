local dbat = require("dbat")
local T = require("lua.libs.pcommand_toggles")

return {
    id = "trackthru",
    aliases = { { "trackthru", 9 } },
    can_execute = T.admin_can(T.ADMLVL.IMPL),
    execute = function(ctx)
        local on = dbat.config.track_through_doors_toggle()
        ctx.ch:send(on and "Will now track through doors.\r\n" or "Will no longer track through doors.\r\n")
    end,
}
