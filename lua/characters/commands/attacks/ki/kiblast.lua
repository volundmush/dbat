return {
    id      = "kiblast",
    aliases = { { "kiblast", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("kiblast") or 0) <= 0 then
            return false, "You do not know how to fire a ki blast."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a ki blast at who?")
            return
        end
        ch:launch_attack("kiblast", target)
    end,
}
