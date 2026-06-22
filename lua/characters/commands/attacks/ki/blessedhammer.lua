return {
    id      = "blessedhammer",
    aliases = { { "blessedhammer", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("blessed hammer") or 0) <= 0 then
            return false, "You do not know the blessed hammer."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a blessed hammer at who?")
            return
        end
        ch:launch_attack("blessedhammer", target)
    end,
}
