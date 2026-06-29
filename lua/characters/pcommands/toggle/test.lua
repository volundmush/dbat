local T = require("lua.libs.pcommand_toggles")

return {
    id = "test",
    aliases = { { "test", 4 } },
    can_execute = function(ch)
        return not ch:is_npc() and ch:admin_level_get() >= 1
    end,
    execute = function(ctx)
        local ch = ctx.ch
        local on = ch:pref_flag_toggle(T.PRF.TEST)
        ch:send_line("Okay. Testing is now: %s", on and "On" or "Off")
        if on then ch:send_line("Make sure to remove nohassle as well.") end
    end,
}
