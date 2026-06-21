return {
    id         = "bonus_wall",
    name       = "Wall",
    tags       = { "bonus" },
    persistent = false,
    description = "The Wall - +20% chance to block",

    on_check_attack_defense = function(ch, cond, inst)
        if not inst.hit or inst.target_is_object then return end
        local block = ch:skill_get("block") or 0
        if math.random(1, 100) <= math.floor(block * 0.20) then
            inst.blocked = true
        end
    end,
}
