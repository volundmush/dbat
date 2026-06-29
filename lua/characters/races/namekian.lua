return {
    id = "namekian",
    legacy_id = 4,
    name = "Namekian",
    abbreviation = "Nam",
    size = "medium",
    pc_ok = true,
    mimic_ok = false,
    valid_sexes = {"neutral"},
    modifiers = function(ch)
        return {
            { target = { "derived", "powerlevel_regen" }, kind = "percent", value = 27500, label = "Namekian" },
            { target = { "derived", "stamina_regen" }, kind = "percent", value = -5000, label = "Namekian" },
        }
    end,
}
