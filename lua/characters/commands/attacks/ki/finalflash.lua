return {
    id      = "finalflash",
    aliases = { { "finalflash", 10 } },

    can_execute = function(ch)
        if (ch:skill_get("final flash") or 0) <= 0 then
            return false, "You do not know the final flash."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a final flash at who?")
            return
        end
        ch:launch_attack("finalflash", target)
    end,
}
