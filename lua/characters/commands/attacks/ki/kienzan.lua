return {
    id      = "kienzan",
    aliases = { { "kienzan", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("kienzan") or 0) <= 0 then
            return false, "You do not know the kienzan."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Throw a kienzan at who?")
            return
        end
        ch:launch_attack("kienzan", target)
    end,
}
