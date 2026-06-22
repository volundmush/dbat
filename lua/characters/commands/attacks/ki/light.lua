return {
    id      = "light",
    aliases = { { "light", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("light grenade") or 0) <= 0 then
            return false, "You do not know the light grenade."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Direct the light grenade at who?")
            return
        end
        ch:launch_attack("lightgrenade", target)
    end,
}
