return {
    id      = "deathbeam",
    aliases = { { "deathbeam", 5 } },

    can_execute = function(ch)
        if (ch:skill_get("deathbeam") or 0) <= 0 then
            return false, "You do not know the deathbeam."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a deathbeam at who?")
            return
        end
        ch:launch_attack("deathbeam", target)
    end,
}
