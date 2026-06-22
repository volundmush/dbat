return {
    id      = "waterrazor",
    aliases = { { "waterrazor", 8 } },

    can_execute = function(ch)
        if (ch:skill_get("water razor") or 0) <= 0 then
            return false, "You do not know the water razor."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use the water razor on who?")
            return
        end
        ch:launch_attack("waterrazor", target)
    end,
}
