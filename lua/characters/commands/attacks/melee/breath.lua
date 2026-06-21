return {
    id      = "breath",
    aliases = { { "breath", 6 } },

    can_execute = function(ch)
        if ch:is_npc() then return true end
        return false, "You cannot do that."
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local arg    = ctx.argparams.tokens[1]
        local target = ch:acquire_room_target(arg)
        if not target then
            ch:send_line("Breathe fire on who?")
            return
        end
        ch:launch_attack("breath", target)
    end,
}
