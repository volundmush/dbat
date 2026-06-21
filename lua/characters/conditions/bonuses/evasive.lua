return {
    id         = "bonus_evasive",
    name       = "Evasive",
    tags       = { "bonus" },
    persistent = false,
    description = "Evasive - +15% to dodge rolls",

    on_check_attack_defense = function(ch, cond, inst)
        if not inst.hit or inst.target_is_object then return end
        local dodge = ch:skill_get("dodge") or 0
        if math.random(1, 100) <= math.floor(dodge * 0.15) then
            inst.dodged = true
        end
    end,
}
