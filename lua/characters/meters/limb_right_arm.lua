local WEAR = dbat.consts.wear_positions

local function drop_wield(ch)
    local obj = ch:equipment_get(WEAR.WIELD1)
    if not obj then return end
    ch:act("@WWithout your right arm you let go of @c$p@W!@n", false, obj, nil, "char")
    ch:act("@C$n@W lets go of @c$p@W!@n", false, obj, nil, "room")
    local removed = ch:unequip(WEAR.WIELD1)
    if removed then removed:to_char(ch) end
end

return {
    id = "limb_right_arm",
    name = "Right Arm Condition",
    derived_stat = "limb_right_arm",
    on_update = function(ch, old_value, new_value)
        if old_value > 0 and new_value <= 0 then drop_wield(ch) end
    end,
}
