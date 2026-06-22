return {
    id      = "zen",
    aliases = { { "zen", 2 } },

    can_execute = function(ch)
        if (ch:skill_get("zen blade strike") or 0) <= 0 then
            return false, "You do not know the zen blade strike."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use zen blade strike on who?")
            return
        end
        ch:launch_attack("zen", target)
    end,
}
