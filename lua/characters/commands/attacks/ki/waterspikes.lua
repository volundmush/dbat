return {
    id      = "waterspikes",
    aliases = { { "waterspikes", 9 } },

    can_execute = function(ch)
        if (ch:skill_get("water spikes") or 0) <= 0 then
            return false, "You do not know water spikes."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire water spikes at who?")
            return
        end
        ch:launch_attack("waterspikes", target)
    end,
}
