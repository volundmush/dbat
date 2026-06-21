return {
    id         = "bonus_accurate",
    name       = "Accurate",
    tags       = { "bonus" },
    persistent = false,
    description = "Accurate - +20% chance to hit physical, +10% to hit with ki",

    on_check_attack_offense = function(ch, cond, inst)
        if inst.hit or inst.target_is_object then return end
        local bonus_pct = inst.def.family == "melee" and 0.20 or 0.10
        local bonus = math.floor(inst.skill_level * bonus_pct)
        if inst.accuracy_roll + bonus < inst.hit_threshold - 20 then return end
        inst.hit = true
        local atk = require("lua.libs.attack")
        inst.hit_location = atk._roll_hitloc(ch, inst.target, inst.skill_level)
        inst.crit, inst.crit_multiplier = atk._calc_crit(ch, inst.hit_location)
        inst.base_damage = atk._base_damage(ch, inst.def, inst)
        if inst.def.on_hit_location then inst.def.on_hit_location(inst) end
        inst.damage = math.floor(inst.base_damage * inst.crit_multiplier)
        for elem, portion in pairs(inst.def.elements or {}) do
            inst.damage_by_element[elem] = math.floor(inst.damage * portion)
        end
    end,
}
