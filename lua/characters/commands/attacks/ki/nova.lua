return {
    id      = "nova",
    aliases = { { "nova", 3 } },

    can_execute = function(ch)
        if (ch:skill_get("starnova") or 0) <= 0 then
            return false, "You do not know the starnova."
        end
        return true
    end,

    execute = function(ctx)
        ctx.ch:launch_attack("starnova", nil)
    end,
}
