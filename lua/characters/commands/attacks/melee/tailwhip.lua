local function db() return require("dbat") end

return {
    id      = "tailwhip",
    aliases = { { "tailwhip", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("tailwhip") or 0) <= 0 then
            return false, "You do not know how to use your tail as a weapon."
        end
        if not ch:is_npc() and not ch:player_flagged(db().consts.player_flags.TAIL) then
            return false, "You don't have a tail!"
        end
        return true
    end,

    execute = function(ctx)
        local ch     = ctx.ch
        local arg    = ctx.argparams.tokens[1]
        local target = ch:acquire_room_target(arg)
        if not target then
            ch:send_line("Tailwhip who?")
            return
        end
        ch:launch_attack("tailwhip", target)
    end,
}
