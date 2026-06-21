return {
    id      = "eraser",
    aliases = { { "eraser", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("eraser cannon") or 0) <= 0 then
            return false, "You do not know the eraser cannon."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Fire an eraser cannon at who?")
            return
        end
        ch:launch_attack("eraser", target)
    end,
}
