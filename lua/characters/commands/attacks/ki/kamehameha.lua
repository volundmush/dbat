return {
    id      = "kamehameha",
    aliases = { { "kamehameha", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("kamehameha") or 0) <= 0 then
            return false, "You do not know the kamehameha."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a kamehameha at who?")
            return
        end
        ch:launch_attack("kamehameha", target)
    end,
}
