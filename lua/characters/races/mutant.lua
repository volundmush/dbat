return {
    id = "mutant",
    legacy_id = 5,
    name = "Mutant",
    abbreviation = "Mut",
    size = "medium",
    pc_ok = true,
    mimic_ok = true,
    modifiers = function(ch)
        return {
            { target = { "regen", "vitals" }, kind = "percent", value = -625, label = "Mutant" },
        }
    end,
}
