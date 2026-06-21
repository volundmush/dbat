return {
    id      = "dodonpa",
    aliases = { { "dodonpa", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("dodonpa") or 0) <= 0 then
            return false, "You do not know the dodonpa."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire a dodonpa at who?")
            return
        end
        ch:launch_attack("dodonpa", target)
    end,
}
