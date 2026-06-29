return {
    id = "konatsu",
    legacy_id = 3,
    name = "Konatsu",
    abbreviation = "kon",
    size = "medium",
    pc_ok = true,
    mimic_ok = true,
    modifiers = function(ch)
        local mods = {
            { target = { "derived", "lifeforce" }, kind = "multiplier", value = 8500, label = "Konatsu" },
            { target = { "derived", "ki_regen" }, kind = "multiplier", value = -2000, label = "Konatsu" },
        }
        if ch:condition_has("flying") and
           not (ch:condition_has("grappling") or ch:condition_has("grappled")) then
            mods[#mods + 1] = { target = { "derived", "speed_index" }, kind = "multiplier", value = 12500, label = "Konatsu Flight" }
        end
        return mods
    end,
}
