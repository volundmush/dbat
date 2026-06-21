return {
    id      = "psychic",
    aliases = { { "psychic", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("psychic blast") or 0) <= 0 then
            return false, "You do not know the psychic blast."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a psychic blast at who?")
            return
        end
        ch:launch_attack("psyblast", target)
    end,
}
