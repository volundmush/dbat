return {
    id      = "dualbeam",
    aliases = { { "dualbeam", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("dual beam") or 0) <= 0 then
            return false, "You do not know the dual beam."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a dual beam at who?")
            return
        end
        ch:launch_attack("dualbeam", target)
    end,
}
