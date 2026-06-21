local COMBO_SEQUENCE = {
    "punch", "kick", "elbow", "knee",
    "roundhouse", "uppercut", "slam", "heeldrop",
}

return {
    id = "combo",
    name = "Combo",
    tags = { "combo" },
    persistent = false,

    -- Returns the next attack id in the combo chain, or nil if the chain is exhausted.
    -- Advances state even on nil so callers don't loop forever.
    next_attack = function(ch, cond)
        local count = cond:number_get("count")
        if count >= 3 then
            cond:number_set("count", 0)
            cond:number_set("state", 0)
            return nil
        end
        local state = cond:number_get("state")
        local id    = COMBO_SEQUENCE[(state % #COMBO_SEQUENCE) + 1]
        cond:number_set("state", state + 1)
        cond:number_set("count", count + 1)
        return id
    end,
}
