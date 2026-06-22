return {
    id         = "flaw_fireprone",
    name       = "Fire Prone",
    tags       = { "flaw" },
    persistent = false,
    description = "Fireprone - +50% Fire Dmg taken, +10% ki, always burned",

    on_check_attack_defense = function(ch, cond, inst)
        local fire_dmg = inst.damage_by_element.fire or 0
        if fire_dmg == 0 then return end
        local bonus = math.floor(fire_dmg * 0.5)
        inst.damage = inst.damage + bonus
        inst.damage_by_element.fire = fire_dmg + bonus
        inst.fire_bonus = true
    end,
}
