return {
    id      = "rogafufuken",
    aliases = { { "rogafufuken", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("rogafufuken") or 0) <= 0 then
            return false, "You do not know the rogafufuken."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use rogafufuken on who?")
            return
        end
        ch:launch_attack("rogafufuken", target)
    end,
}
