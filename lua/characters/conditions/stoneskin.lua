local STONE_TIERS = {
    { limit = 20,  mult = 250   },
    { limit = 30,  mult = 500   },
    { limit = 50,  mult = 1000  },
    { limit = 60,  mult = 2000  },
    { limit = 70,  mult = 5000  },
    { limit = 90,  mult = 10000 },
    { limit = 101, mult = 25000 },
}

local function on_check_attack_defense(ch, cond, ctx)
    if ctx.absorbed or ctx.damage <= 0 then return end
    local level = ch:stat_get("level") or 0
    local mult = STONE_TIERS[#STONE_TIERS].mult
    for _, tier in ipairs(STONE_TIERS) do
        if level < tier.limit then
            mult = tier.mult
            break
        end
    end
    ctx.damage = math.max(0, ctx.damage - level * mult)
end

return {
    id = "stoneskin",
    name = "Stoneskin",
    tags = { "stoneskin" },
    persistent = true,
    on_check_attack_defense = on_check_attack_defense,
}
