return {
    id      = "bite",
    aliases = { { "bite", 3 } },

    can_execute = function(ch)
        if ch:is_npc() then return true end
        if ch:race_get() == "mutant" and (ch:genome_get(0) == 7 or ch:genome_get(1) == 7) then
            return true
        end
        return false, "You don't want to put that in your mouth, you don't know where it has been!"
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local arg    = ctx.argparams.tokens[1]
        local target = ch:acquire_room_target(arg)
        if not target then
            ch:send_line("Bite who?")
            return
        end
        ch:launch_attack("bite", target)
    end,
}
