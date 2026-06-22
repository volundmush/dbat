return {
    id      = "koteiru",
    aliases = { { "koteiru", 7 } },

    can_execute = function(ch)
        if (ch:skill_get("koteiru bakuha") or 0) <= 0 then
            return false, "You do not know the koteiru bakuha."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire koteiru bakuha at who?")
            return
        end
        ch:launch_attack("koteiru", target)
    end,
}
