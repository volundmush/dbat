return {
    id      = "ram",
    aliases = { { "ram", 3 } },

    can_execute = function(ch)
        if ch:is_npc() then return true end
        return false, "You cannot do that."
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local arg    = ctx.argparams.tokens[1]
        local target = ch:acquire_room_target(arg)
        if not target then
            ch:send_line("Ram who?")
            return
        end
        ch:launch_attack("ram", target)
    end,
}
