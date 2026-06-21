local COMBO_SEQUENCE = {
    "punch", "kick", "elbow", "knee",
    "roundhouse", "uppercut", "slam", "heeldrop",
}

return {
    id = "combo",
    name = "Combo",
    tags = { "combo" },
    persistent = false,

    -- Returns the suggested next attack id. Advances state first so the
    -- returned suggestion matches what the HUD will display after this call.
    next_attack = function(ch, cond)
        local state     = cond:number_get("state")
        local count     = cond:number_get("count")
        local new_state = (state + 1) % #COMBO_SEQUENCE
        cond:number_set("state", new_state)
        cond:number_set("count", count + 1)
        return COMBO_SEQUENCE[new_state + 1]
    end,
}
