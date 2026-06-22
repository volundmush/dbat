return {
    id      = "malice",
    aliases = { { "malice", 5 } },

    can_execute = function(ch)
        if (ch:skill_get("malice breaker") or 0) <= 0 then
            return false, "You do not know the malice breaker."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use malice breaker on who?")
            return
        end
        ch:launch_attack("malice", target)
    end,
}
