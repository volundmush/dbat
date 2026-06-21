return {
    id      = "headbutt",
    aliases = { { "headbutt", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("headbutt") or 0) <= 0 then
            return false, "You do not know how to headbutt."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local arg    = ctx.argparams.tokens[1]
        local target = ch:acquire_room_target(arg)
        if not target then
            ch:send_line("Headbutt who?")
            return
        end
        ch:launch_attack("headbutt", target)
    end,
}
