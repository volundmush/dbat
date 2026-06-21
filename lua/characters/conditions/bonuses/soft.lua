return {
    id         = "bonus_soft",
    name       = "Soft",
    tags       = { "bonus" },
    persistent = false,
    description = "Soft Touch - Body specialist: 2x body, no head crit bonus, 0.25x limbs",

    on_check_attack_offense = function(ch, cond, inst)
        local base = inst.base_damage
        local loc  = inst.hit_location
        if loc == "head" then
            inst.damage = math.floor(base * 1.0)
        elseif loc == "arm" or loc == "leg" then
            inst.damage = math.floor(base * 0.25)
        elseif loc == "body" then
            inst.damage = math.floor(base * 2.0)
        end
    end,
}
