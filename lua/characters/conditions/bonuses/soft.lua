return {
    id         = "bonus_soft",
    name       = "Soft",
    tags       = { "bonus" },
    persistent = false,
    description = "Soft Touch - Half damage for all hit locations",

    on_check_attack_offense = function(ch, cond, inst)
        if inst.hit_location == "body" then
            inst.damage = math.floor(inst.damage * 1.5)
        end
    end,
}
