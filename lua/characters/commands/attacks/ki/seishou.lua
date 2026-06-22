return {
    id      = "seishou",
    aliases = { { "seishou", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("seishou enko") or 0) <= 0 then
            return false, "You do not know the seishou enko."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a seishou enko at who?")
            return
        end
        ch:launch_attack("seishou", target)
    end,
}
