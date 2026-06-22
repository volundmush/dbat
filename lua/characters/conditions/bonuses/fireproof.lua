return {
    id         = "bonus_fireproof",
    name       = "Fireproof",
    tags       = { "bonus" },
    persistent = false,
    description = "Fireproof - -50% Fire Dmg taken, -10% ki, immunity to burn",

    on_check_attack_defense = function(ch, cond, inst)
        local fire_dmg = inst.damage_by_element.fire or 0
        if fire_dmg == 0 then return end
        local reduction = math.floor(fire_dmg * 0.5)
        inst.damage = math.max(0, inst.damage - reduction)
        inst.damage_by_element.fire = math.max(0, fire_dmg - reduction)
        inst.fire_immune = true
    end,
}
