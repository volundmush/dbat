local function modifiers(ch)
    local dbat = require("dbat")
    local mods = {}

    local room = ch:room_get()
    if room and room:is_sunken() then
        mods[#mods + 1] = { target = { "regen", "vitals" }, kind = "percent", value = 16000, label = "Kanassan Underwater" }
    end
    
    -- Rainy bonus
    if ch:is_outside() and dbat.weather_info.sky == 2 then
        mods[#mods + 1] = { target = { "regen", "vitals" }, kind = "percent", value = 11000, label = "Kanassan in Rain" }
    end

    return mods
end

return {
    id = "kanassan",
    legacy_id = 6,
    name = "Kanassan",
    abbreviation = "Kan",
    size = "medium",
    pc_ok = true,
    mimic_ok = true,
    modifiers = modifiers
}
