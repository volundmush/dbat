local function act() return require("dbat").lib.act end

local WP = nil
local function wear()
    if not WP then WP = require("dbat").consts.wear_positions end
    return WP
end

-- gun_dam formula: fixed damage table scaled by wlvl, then DEX * level, capped at 40% max PL
local GUN_BASE = { [0]=100, [1]=50, [2]=200, [3]=750, [4]=2000, [5]=7500 }

-- Zenni cost per shot by wlvl (0/1 = free)
local GUN_COST = { [0]=0, [1]=0, [2]=2, [3]=4, [4]=6, [5]=12 }

local HIT_MSGS = {
    body = {
        actor  = "@WYou blast @C$N's@W chest!@n",
        target = "@c$n@W blasts your chest!@n",
        room   = "@c$n@W blasts @C$N's@W chest!@n",
    },
    head = {
        actor  = "@WYou blast @C$N@W in the face!@n",
        target = "@c$n@W blasts you in the face!@n",
        room   = "@c$n@W blasts @C$N@W in the face!@n",
    },
    arm = {
        actor  = "@WYou blast @C$N's@W arm!@n",
        target = "@c$n@W blasts your arm!@n",
        room   = "@c$n@W blasts @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou blast @C$N's@W leg!@n",
        target = "@c$n@W blasts your leg!@n",
        room   = "@c$n@W blasts @C$N's@W leg!@n",
    },
}

return {
    id   = "shoot",
    family = "weapon",
    name = "Shoot",
    skill = "gun",
    weapon_type = "blast",
    tier  = 2,
    elements = { physical = 1.0 },
    limbs_required = {},
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.2,
    base_power    = 1.0,

    on_check = function(inst)
        local ch = inst.attacker
        local W  = wear()
        local weap = ch:equipment_get(W.WIELD1) or ch:equipment_get(W.WIELD2)
        if not weap then
            return false, "You need to wield a gun to shoot."
        end
        if weap:weapon_damtype_get() ~= "blast" then
            return false, "You need a gun (blast weapon) for that."
        end
        if weap:is_broken() then
            return false, "Your gun is broken!"
        end
        inst.weapon = weap

        -- Zenni (gold) check: TODO — gold_get/gold_mod not yet exposed to Lua.
        -- When exposed: check ch:gold_get() >= GUN_COST[wlvl] here.
    end,

    -- Gun damage is independent of powerlevel — fixed formula based on wlvl + DEX + level
    on_calculate_damage = function(inst)
        local ch    = inst.attacker
        local wlvl  = inst.weapon and inst.weapon:weapon_level_get() or 0
        local skill = ch:skill_get("gun")
        local base  = GUN_BASE[wlvl] or 100
        if skill >= 100 then
            base = base * 2
        elseif skill >= 50 then
            base = math.floor(base * 1.5)
        end
        local dex   = ch:stat_get("dexterity") or 0
        local lvl   = ch:stat_get("level") or 1
        local maxpl = ch:meter_max("lifeforce")
        local dmg   = math.floor((base * dex) * ((lvl / 5) + 1))
        return math.min(dmg, math.floor(maxpl * 0.4))
    end,

    on_calculate_cost = function(inst)
        -- TODO: add zenni cost when gold_get/gold_mod is exposed
        -- inst.cost.zenni = GUN_COST[inst.weapon and inst.weapon:weapon_level_get() or 0]
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local msg = HIT_MSGS[inst.hit_location] or HIT_MSGS.body
        a.message(msg, ctx)
        -- Guns do not call weapon_damage (no durability loss per shot)
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou fire at @C$N@W but miss!@n",
            target = "@c$n@W fires at you but misses!@n",
            room   = "@c$n@W fires at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W dodges your shot!@n",
            target = "@WYou dodge @c$n's@W shot!@n",
            room   = "@C$N@W dodges @c$n's@W shot!@n",
        }, ctx)
    end,
}
