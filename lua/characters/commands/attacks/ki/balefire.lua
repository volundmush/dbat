return {
    id      = "balefire",
    aliases = { { "balefire", 5 } },

    can_execute = function(ch)
        if (ch:skill_get("balefire") or 0) <= 0 then
            return false, "You do not know balefire."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Unleash balefire at who?")
            return
        end
        ch:launch_attack("balefire", target)
    end,
}
