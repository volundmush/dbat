return {
    id      = "phoenix",
    aliases = { { "phoenix", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("phoenix slash") or 0) <= 0 then
            return false, "You do not know the phoenix slash."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use phoenix slash on who?")
            return
        end
        ch:launch_attack("pslash", target)
    end,
}
