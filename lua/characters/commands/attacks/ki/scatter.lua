return {
    id      = "scatter",
    aliases = { { "scatter", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("scatter shot") or 0) <= 0 then
            return false, "You do not know the scatter shot."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a scatter shot at who?")
            return
        end
        ch:launch_attack("scatter", target)
    end,
}
