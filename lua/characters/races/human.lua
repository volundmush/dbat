return {
    id = "human",
    legacy_id = 0,
    name = "Human",
    abbreviation = "Hum",
    size = "medium",
    pc_ok = true,
    mimic_ok = true,
    modifiers = function(ch)
        return {
            { target = { "derived", "powerlevel_regen" }, kind = "percent", value = -5000, label = "Human" },
        }
    end,
}
