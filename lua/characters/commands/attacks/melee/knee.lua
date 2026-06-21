return {
    id      = "knee",
    aliases = { { "knee", 2 } },

    can_execute = function(ch)
        if (ch:skill_get("knee") or 0) <= 0 then
            return false, "You do not know how to knee."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local arg    = ctx.argparams.tokens[1]
        local target = ch:acquire_room_target(arg)
        if not target then
            ch:send_line("Knee who?")
            return
        end
        ch:launch_attack("knee", target)
    end,
}
