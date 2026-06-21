return {
    id      = "beam",
    aliases = { { "beam", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("beam") or 0) <= 0 then
            return false, "You do not know how to fire a beam."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a beam at who?")
            return
        end
        ch:launch_attack("beam", target)
    end,
}
