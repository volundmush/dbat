return {
    id      = "hspiral",
    aliases = { { "hspiral", 7 } },

    can_execute = function(ch)
        if (ch:skill_get("hell spiral") or 0) <= 0 then
            return false, "You do not know the hell spiral."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a hell spiral at who?")
            return
        end
        ch:launch_attack("hspiral", target)
    end,
}
