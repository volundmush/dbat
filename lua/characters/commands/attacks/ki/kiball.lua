return {
    id      = "kiball",
    aliases = { { "kiball", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("kiball") or 0) <= 0 then
            return false, "You do not know how to fire a ki ball."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a ki ball at who?")
            return
        end
        ch:launch_attack("kiball", target)
    end,
}
