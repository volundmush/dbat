return {
    id      = "kousengan",
    aliases = { { "kousengan", 9 } },

    can_execute = function(ch)
        if (ch:skill_get("kousengan") or 0) <= 0 then
            return false, "You do not know the kousengan."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire kousengan at who?")
            return
        end
        ch:launch_attack("kousengan", target)
    end,
}
