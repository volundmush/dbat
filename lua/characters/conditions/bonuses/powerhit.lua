return {
    id         = "bonus_powerhit",
    name       = "Power Hit",
    tags       = { "bonus" },
    persistent = false,
    description = "Powerhitter - 15% chance for a 4x head crit instead of 2x",

    on_check_attack_offense = function(ch, cond, inst)
        if inst.hit_location == "head" and math.random(1, 100) <= 15 then
            inst.damage = math.floor(inst.base_damage * 4.0)
        end
    end,
}
