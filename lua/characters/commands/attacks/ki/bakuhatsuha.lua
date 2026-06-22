return {
    id      = "bakuhatsuha",
    aliases = { { "bakuhatsuha", 4 } },

    can_execute = function(ch)
        if (ch:skill_get("bakuhatsuha") or 0) <= 0 then
            return false, "You do not know bakuhatsuha."
        end
        return true
    end,

    execute = function(ctx)
        ctx.ch:launch_attack("bakuhatsuha", nil)
    end,
}
