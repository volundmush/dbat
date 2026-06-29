return {
    id = "demon",
    legacy_id = 10,
    name = "Demon",
    abbreviation = "Dem",
    size = "medium",
    pc_ok = true,
    mimic_ok = false,
    modifiers = function(ch)
        local mods = {
            { target = { "derived", "lifeforce" }, kind = "multiplier", value = 7500, label = "Demon" },
        }
        if ch:condition_has("flying") and
           not (ch:condition_has("grappling") or ch:condition_has("grappled")) then
            mods[#mods + 1] = { target = { "derived", "speed_index" }, kind = "multiplier", value = 12500, label = "Demon Flight" }
        end
        return mods
    end,
}
