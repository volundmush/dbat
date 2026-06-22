return {
    id      = "twinslash",
    aliases = { { "twinslash", 5 } },

    can_execute = function(ch)
        if (ch:skill_get("twin slash") or 0) <= 0 then
            return false, "You do not know the twin slash."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use twin slash on who?")
            return
        end
        ch:launch_attack("twinslash", target)
    end,
}
