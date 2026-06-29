local dbat = require("dbat")
local T = require("lua.libs.pcommand_toggles")

return {
    id = "nocompress",
    aliases = { { "nocompress", 10 } },
    can_execute = T.admin_can(T.ADMLVL.NONE),
    execute = function(ctx)
        local ch = ctx.ch
        if not dbat.config.compression_enabled() then
            ch:send_line("Sorry, compression is globally disabled.")
            return
        end
        local on = ch:pref_flag_toggle(T.PRF.NOCOMPRESS)
        ch:send(on and "Compression will not be used even if your client supports it.\r\n" or "Compression will be used if your client supports it.\r\n")
    end,
}
