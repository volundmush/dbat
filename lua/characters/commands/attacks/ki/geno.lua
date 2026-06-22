return {
    id      = "geno",
    aliases = { { "genocide", 7 }, { "geno", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("genocide") or 0) <= 0 then
            return false, "You do not know the genocide attack."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Direct it at who?")
            return
        end
        ch:launch_attack("genocide", target)
    end,
}
