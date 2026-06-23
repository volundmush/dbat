local act = dbat.lib.act

return {
    id = "dark_metamorphosis",
    name = "Dark Metamorphosis",
    tags = { "power_amp", "dark_metamorphosis" },
    persistent = false,
    modifiers = function()
        return {
            { target = { "derived", "powerlevel" }, kind = "percent", value = 6000, label = "Dark Metamorphosis" },
        }
    end,

    -- Lifesteal on kill: drain 12% of victim's max powerlevel.
    on_kill = function(ch, cond, ctx)
        local heal = math.floor(ctx.target:meter_max("powerlevel") * 0.12)
        if heal <= 0 then return end
        act.message({
            actor  = "@RYour dark aura saps some of @r$N's@R life energy!@n",
            target = "@r$n@R's dark aura saps some of your life energy!@n",
        }, { actor = ch, target = ctx.target })
        ch:meter_mod_int("powerlevel", heal)
    end,
}
