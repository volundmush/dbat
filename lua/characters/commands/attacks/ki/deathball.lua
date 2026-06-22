return {
    id      = "deathball",
    aliases = { { "deathball", 8 } },

    can_execute = function(ch)
        if (ch:skill_get("deathball") or 0) <= 0 then
            return false, "You do not know the deathball."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Form a deathball for who?")
            return
        end
        ch:launch_attack("deathball", target)
    end,
}
