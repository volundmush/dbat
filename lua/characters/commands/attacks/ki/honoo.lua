return {
    id      = "honoo",
    aliases = { { "honoo", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("honoo") or 0) <= 0 then
            return false, "You do not know honoo."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Unleash honoo at who?")
            return
        end
        ch:launch_attack("honoo", target)
    end,
}
