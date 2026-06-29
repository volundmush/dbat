local T = require("lua.libs.pcommand_toggles")
local act = require("lua.libs.act")

return {
    id = "afk",
    aliases = { { "afk", 3 } },
    can_execute = T.admin_can(T.ADMLVL.NONE),
    execute = function(ctx)
        local ch = ctx.ch
        local on = ch:pref_flag_toggle(T.PRF.AFK)
        if on then
            act.around(ch, "$n has gone AFK.", { actor = ch })
            ch:send("AFK flag is now on.\r\n")
        else
            act.around(ch, "$n has come back from AFK.", { actor = ch })
            if ch:has_mail() then ch:send_line("You have mail waiting.") end
            ch:send("AFK flag is now off.\r\n")
        end
    end,
}
