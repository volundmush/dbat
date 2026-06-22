return {
    id      = "genkidama",
    aliases = { { "genkidama", 8 } },

    can_execute = function(ch)
        if (ch:skill_get("genkidama") or 0) <= 0 then
            return false, "You do not know the genkidama."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Direct the genkidama at who?")
            return
        end
        ch:launch_attack("genkidama", target)
    end,
}
