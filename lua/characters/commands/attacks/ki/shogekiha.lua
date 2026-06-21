return {
    id      = "shogekiha",
    aliases = { { "shogekiha", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("shogekiha") or 0) <= 0 then
            return false, "You do not know the shogekiha."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Use shogekiha on who?")
            return
        end
        ch:launch_attack("shogekiha", target)
    end,
}
