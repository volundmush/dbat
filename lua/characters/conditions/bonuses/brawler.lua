return {
    id         = "bonus_brawler",
    name       = "Brawler",
    tags       = { "bonus" },
    persistent = false,
    description = "Brawler - Physical attacks do 20% more damage",

    on_check_attack_offense = function(ch, cond, inst)
        if inst.def.family == "melee" then
            inst.damage = math.floor(inst.damage * 1.2)
        end
    end,
}
