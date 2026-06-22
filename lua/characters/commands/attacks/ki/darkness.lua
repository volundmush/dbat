return {
    id      = "darkness",
    aliases = { { "darkness", 7 } },

    can_execute = function(ch)
        if (ch:skill_get("darkness dragon slash") or 0) <= 0 then
            return false, "You do not know the darkness dragon slash."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use darkness dragon slash on who?")
            return
        end
        ch:launch_attack("ddslash", target)
    end,
}
