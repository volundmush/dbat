return {
    id      = "masenko",
    aliases = { { "masenko", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("masenko") or 0) <= 0 then
            return false, "You do not know the masenko."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a masenko at who?")
            return
        end
        ch:launch_attack("masenko", target)
    end,
}
