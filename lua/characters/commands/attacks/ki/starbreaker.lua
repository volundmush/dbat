return {
    id      = "starbreaker",
    aliases = { { "starbreaker", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("star breaker") or 0) <= 0 then
            return false, "You do not know the star breaker."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a star breaker at who?")
            return
        end
        ch:launch_attack("starbreaker", target)
    end,
}
