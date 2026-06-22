return {
    id      = "barrage",
    aliases = { { "barrage", 7 } },

    can_execute = function(ch)
        if (ch:skill_get("psychic barrage") or 0) <= 0 then
            return false, "You do not know the psychic barrage."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a psychic barrage at who?")
            return
        end
        ch:launch_attack("pbarrage", target)
    end,
}
