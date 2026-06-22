return {
    id      = "bigbang",
    aliases = { { "bigbang", 6 } },

    can_execute = function(ch)
        if (ch:skill_get("big bang") or 0) <= 0 then
            return false, "You do not know the big bang."
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local target = ch:acquire_room_target(ctx.argparams.tokens[1])
        if not target then
            ch:send_line("Release a big bang at who?")
            return
        end
        ch:launch_attack("bigbang", target)
    end,
}
