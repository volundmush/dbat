local function act() return require("dbat").lib.act end

local WP = nil
local function wear()
    if not WP then WP = require("dbat").consts.wear_positions end
    return WP
end

local WLVL_MULT = { [1]=1.05, [2]=1.10, [3]=1.20, [4]=1.30, [5]=1.50 }

local HIT_MSGS = {
    body = {
        actor  = "@WYou whack @C$N's@W chest!@n",
        target = "@c$n@W whacks your chest!@n",
        room   = "@c$n@W whacks @C$N's@W chest!@n",
    },
    head = {
        actor  = "@WYou whack @C$N@W in the face!@n",
        target = "@c$n@W whacks you in the face!@n",
        room   = "@c$n@W whacks @C$N@W in the face!@n",
    },
    arm = {
        actor  = "@WYou whack @C$N's@W arm!@n",
        target = "@c$n@W whacks your arm!@n",
        room   = "@c$n@W whacks @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou whack @C$N's@W leg!@n",
        target = "@c$n@W whacks your leg!@n",
        room   = "@c$n@W whacks @C$N's@W leg!@n",
    },
}

-- Brawl skill bonus: +50% at 100, +20% at 50
local function brawl_skill_mult(skill)
    if     skill >= 100 then return 1.50
    elseif skill >= 50  then return 1.20
    else return 1.0 end
end

return {
    id   = "brawl",
    name = "Brawl",
    skill = "brawl",
    weapon_type = "brawl",
    tier  = 2,
    elements = { physical = 1.0 },
    limbs_required = {},
    damages_limbs = false,
    can_combo = true,
    in_combo  = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block = true,
    can_parry = true,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 1.0,

    on_check = function(inst)
        local ch = inst.attacker
        local W  = wear()
        local weap = ch:equipment_get(W.WIELD1) or ch:equipment_get(W.WIELD2)
        if not weap then
            return false, "You need to be wielding something to brawl with."
        end
        -- Brawl is the fallback for unrecognized weapon types
        local dt = weap:weapon_damtype_get()
        if dt == "slash" or dt == "crush" or dt == "pierce" or dt == "stab" or dt == "blast" then
            return false, "Use the appropriate weapon technique instead of brawling."
        end
        if weap:is_broken() then
            return false, "Your weapon is broken!"
        end
        inst.weapon = weap
    end,

    on_check_attack_offense = function(inst)
        local ch   = inst.attacker
        local wlvl = inst.weapon and inst.weapon:weapon_level_get() or 0
        local mult = WLVL_MULT[wlvl]
        if mult then inst.damage = math.floor(inst.damage * mult) end

        local skill = ch:skill_get("brawl")
        inst.damage = math.floor(inst.damage * brawl_skill_mult(skill))
    end,

    on_calculate_cost = function(inst)
        local W    = wear()
        local ch   = inst.attacker
        local weap = ch:equipment_get(W.WIELD1) or ch:equipment_get(W.WIELD2)
        if weap then
            inst.cost.stamina = (inst.cost.stamina or 0) + weap:weight_get()
        end
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local msg = HIT_MSGS[inst.hit_location] or HIT_MSGS.body
        a.message(msg, ctx)
        if inst.weapon then inst.weapon:weapon_damage(1) end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou swing at @C$N@W but miss!@n",
            target = "@c$n@W swings at you but misses!@n",
            room   = "@c$n@W swings at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W dodges your swing!@n",
            target = "@WYou dodge @c$n's@W swing!@n",
            room   = "@C$N@W dodges @c$n's@W swing!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W blocks your swing!@n",
            target = "@WYou block @c$n's@W swing!@n",
            room   = "@C$N@W blocks @c$n's@W swing!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your swing!@n",
            target = "@WYou parry @c$n's@W swing!@n",
            room   = "@C$N@W parries @c$n's@W swing!@n",
        }, ctx)
    end,
}
