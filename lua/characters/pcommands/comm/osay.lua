local dbat = require("dbat")

return {
    id       = "osay",
    priority = 0,
    aliases  = { { "osay", 1 } },
    can_execute = function(ch)
        if ch:is_npc() then return false, "NPCs cannot osay." end
        return true
    end,
    execute = function(ctx)
        local ch  = ctx.ch
        local msg = ctx.arguments
        if not msg or msg == "" then
            ch:send_line("Yes, but WHAT do you want to osay?")
            return
        end
        local PRF = dbat.consts.prf_flags
        local display_name
        if ch:pref_flagged(PRF.HIDE) then
            display_name = "Anonymous"
        elseif ch:admin_level_get() > 0 then
            display_name = ch:name_get()
        else
            display_name = ch:user_get() or ch:name_get()
        end
        ch:send_line("@WYou @D[@mOSAY@D] @W'@w%s@W'@n", msg)
        ch:send_line_around("@W%s @D[@mOSAY@D] @W'@w%s@W'@n", display_name, msg)
    end,
}
