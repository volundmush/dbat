local function act() return require("dbat").lib.act end

local WP = nil
local function wear()
    if not WP then WP = require("dbat").consts.wear_positions end
    return WP
end

local WLVL_MULT = { [1]=1.05, [2]=1.10, [3]=1.20, [4]=1.30, [5]=1.50 }

local HIT_MSGS = {
    body = {
        actor  = "@WYou stab @C$N's@W chest!@n",
        target = "@c$n@W stabs your chest!@n",
        room   = "@c$n@W stabs @C$N's@W chest!@n",
    },
    head = {
        actor  = "@WYou stab @C$N@W in the face!@n",
        target = "@c$n@W stabs you in the face!@n",
        room   = "@c$n@W stabs @C$N@W in the face!@n",
    },
    arm = {
        actor  = "@WYou stab @C$N's@W arm!@n",
        target = "@c$n@W stabs your arm!@n",
        room   = "@c$n@W stabs @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou stab @C$N's@W leg!@n",
        target = "@c$n@W stabs your leg!@n",
        room   = "@c$n@W stabs @C$N's@W leg!@n",
    },
}

return {
    id   = "stab",
    family = "weapon",
    name = "Stab",
    skill = "spear",
    weapon_type = "stab",
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
            return false, "You need to wield a weapon to stab."
        end
        if weap:weapon_damtype_get() ~= "stab" then
            return false, "You need a stabbing weapon (spear) for that."
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

        -- Spear mastery bonus: +4% at 100, +10% at 50
        local skill = ch:skill_get("spear")
        if skill >= 100 then
            inst.damage = math.floor(inst.damage * 1.04)
        elseif skill >= 50 then
            inst.damage = math.floor(inst.damage * 1.10)
        end
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
            actor  = "@WYou thrust your spear at @C$N@W but miss!@n",
            target = "@c$n@W thrusts a spear at you but misses!@n",
            room   = "@c$n@W thrusts a spear at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W dodges your stab!@n",
            target = "@WYou dodge @c$n's@W stab!@n",
            room   = "@C$N@W dodges @c$n's@W stab!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W blocks your stab!@n",
            target = "@WYou block @c$n's@W stab!@n",
            room   = "@C$N@W blocks @c$n's@W stab!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your stab!@n",
            target = "@WYou parry @c$n's@W stab!@n",
            room   = "@C$N@W parries @c$n's@W stab!@n",
        }, ctx)
    end,
}
