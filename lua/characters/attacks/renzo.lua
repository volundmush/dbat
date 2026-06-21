local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

-- Count tier messages: gap between effective accuracy and hit threshold
local COUNT_MSGS = {
    [100] = {
        actor  = "@WEvery single one of your @Yrenzokou energy blasts@W finds its mark on @C$N@W!@n",
        target = "@wEvery single one of @c$n@W's @Yrenzokou energy blasts@W finds its mark on you!@n",
        room   = "@wEvery single one of @c$n@W's @Yrenzokou energy blasts@W finds its mark on @C$N@W!@n",
    },
    [75] = {
        actor  = "@WMost of your @Yrenzokou energy blasts@W slam into @C$N@W!@n",
        target = "@wMost of @c$n@W's @Yrenzokou energy blasts@W slam into you!@n",
        room   = "@wMost of @c$n@W's @Yrenzokou energy blasts@W slam into @C$N@W!@n",
    },
    [50] = {
        actor  = "@WHalf of your @Yrenzokou energy blasts@W strike @C$N@W!@n",
        target = "@wHalf of @c$n@W's @Yrenzokou energy blasts@W strike you!@n",
        room   = "@wHalf of @c$n@W's @Yrenzokou energy blasts@W strike @C$N@W!@n",
    },
    [25] = {
        actor  = "@WA quarter of your @Yrenzokou energy blasts@W hit @C$N@W!@n",
        target = "@wA quarter of @c$n@W's @Yrenzokou energy blasts@W hit you!@n",
        room   = "@wA quarter of @c$n@W's @Yrenzokou energy blasts@W hit @C$N@W!@n",
    },
    [10] = {
        actor  = "@WOnly a few of your @Yrenzokou energy blasts@W connect with @C$N@W!@n",
        target = "@wOnly a few of @c$n@W's @Yrenzokou energy blasts@W connect with you!@n",
        room   = "@wOnly a few of @c$n@W's @Yrenzokou energy blasts@W connect with @C$N@W!@n",
    },
}

local function get_count(inst)
    local gap = (inst.effective_accuracy or 0) - (inst.hit_threshold - 20)
    local count = gap >= 20 and 100
               or gap >= 15 and 75
               or gap >= 10 and 50
               or gap >= 5  and 25
               or 10
    -- Nail sensei bonus: increases hit count at higher skill
    if inst.attacker:sensei_get() == "nail" then
        local skill = inst.skill_level
        local bonus = skill >= 100 and 200 or skill >= 60 and 100 or 40
        count = math.min(count + bonus, 100)
    end
    return count
end

return {
    id    = "renzo",
    family = "ki",
    name  = "Renzokou Energy Dan",
    skill = "renzo",
    tier  = 3,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = true,
    can_parry = true,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 2.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_calculate_cost = function(inst)
        -- Mastery has a chance to reduce ki cost
        local skill = inst.skill_level
        local roll  = math.random(100)
        local tier1 = skill >= 100 and 10 or skill >= 75 and 5 or 5
        local tier2 = skill >= 100 and 20 or skill >= 75 and 10 or 5
        if roll <= tier1 then
            inst.cost.ki = math.floor((inst.cost.ki or 0) * 0.25)
        elseif roll <= tier1 + tier2 then
            inst.cost.ki = math.floor((inst.cost.ki or 0) * 0.50)
        end
    end,

    on_hit = function(inst)
        local count = get_count(inst)
        inst.damage = math.floor(inst.damage * count / 100)
        local ctx   = { actor = inst.attacker, target = inst.target }
        act().message(COUNT_MSGS[count] or COUNT_MSGS[10], ctx)
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a barrage of @Yrenzokou energy blasts@W at @C$N@W but all miss!@n",
            target = "@c$n@W fires a barrage of @Yrenzokou energy blasts@W at you but all miss!@n",
            room   = "@c$n@W fires a barrage of @Yrenzokou energy blasts@W at @C$N@W but all miss!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W evades your @Yrenzokou energy blasts@W entirely!@n",
            target = "@WYou evade @c$n@W's @Yrenzokou energy blasts@W entirely!@n",
            room   = "@C$N@W evades @c$n@W's @Yrenzokou energy blasts@W entirely!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_parried = function(inst)
        act().message({
            actor  = "@C$N@W deflects your @Yrenzokou energy blasts@W!@n",
            target = "@WYou deflect @c$n@W's @Yrenzokou energy blasts@W!@n",
            room   = "@C$N@W deflects @c$n@W's @Yrenzokou energy blasts@W!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W braces and blocks your @Yrenzokou energy blasts@W!@n",
            target = "@WYou brace and block @c$n@W's @Yrenzokou energy blasts@W!@n",
            room   = "@C$N@W braces and blocks @c$n@W's @Yrenzokou energy blasts@W!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
