return {
    id      = "galikgun",
    aliases = { { "galikgun", 5 } },

    can_execute = function(ch)
        if (ch:skill_get("galik gun") or 0) <= 0 then
            return false, "You do not know the galik gun."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a galik gun at who?")
            return
        end
        ch:launch_attack("galikgun", target)
    end,
}
