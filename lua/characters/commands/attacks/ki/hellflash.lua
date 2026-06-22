return {
    id      = "hellflash",
    aliases = { { "hellflash", 8 } },

    can_execute = function(ch)
        if (ch:skill_get("hell flash") or 0) <= 0 then
            return false, "You do not know the hell flash."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Unleash a hell flash at who?")
            return
        end
        ch:launch_attack("hellflash", target)
    end,
}
