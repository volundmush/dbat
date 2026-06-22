return {
    id      = "kakusanha",
    aliases = { { "kakusanha", 7 } },

    can_execute = function(ch)
        if (ch:skill_get("kakusanha") or 0) <= 0 then
            return false, "You do not know the kakusanha."
        end
        return true
    end,

    execute = function(ctx)
        ctx.ch:launch_attack("kakusanha", nil)
    end,
}
