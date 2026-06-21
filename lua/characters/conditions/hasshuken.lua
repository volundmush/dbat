local HASSHUKEN_ATTACKS = { punch = true, elbow = true, uppercut = true }

local function modifiers(ch, cond)
    return {}
end

return {
    id = "hasshuken",
    name = "Hasshuken",
    tags = { "hasshuken" },
    persistent = true,
    modifiers = modifiers,
    status_line = function(ch, cond) return "Your arms are moving fast." end,

    on_check_attack_offense = function(ch, cond, inst)
        if not HASSHUKEN_ATTACKS[inst.def.id] then return end
        inst.damage = inst.damage * 2
    end,
}
