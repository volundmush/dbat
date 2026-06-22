return {
    id      = "hellspear",
    aliases = { { "hellspear", 8 } },

    can_execute = function(ch)
        if (ch:skill_get("hell spear blast") or 0) <= 0 then
            return false, "You do not know the hell spear blast."
        end
        return true
    end,

    execute = function(ctx)
        ctx.ch:launch_attack("hellspear", nil)
    end,
}
