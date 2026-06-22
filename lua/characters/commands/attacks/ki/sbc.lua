return {
    id      = "sbc",
    aliases = { { "sbc", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("special beam cannon") or 0) <= 0 then
            return false, "You do not know the special beam cannon."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Direct it at who?")
            return
        end
        ch:launch_attack("sbc", target)
    end,
}
