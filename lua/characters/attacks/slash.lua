local function act() return require("dbat").lib.act end

local WP = nil
local function wear()
    if not WP then WP = require("dbat").consts.wear_positions end
    return WP
end

local WLVL_MULT = { [1]=1.05, [2]=1.10, [3]=1.20, [4]=1.30, [5]=1.50 }

local HIT_MSGS = {
    body = {
        actor  = "@WYou slash @C$N@W across the chest!@n",
        target = "@c$n@W slashes you across the chest!@n",
        room   = "@c$n@W slashes @C$N@W across the chest!@n",
    },
    head = {
        actor  = "@WYou slash @C$N@W across the face!@n",
        target = "@c$n@W slashes you across the face!@n",
        room   = "@c$n@W slashes @C$N@W across the face!@n",
    },
    arm = {
        actor  = "@WYou slash @C$N@W across the arm!@n",
        target = "@c$n@W slashes you across the arm!@n",
        room   = "@c$n@W slashes @C$N@W across the arm!@n",
    },
    leg = {
        actor  = "@WYou slash @C$N@W across the leg!@n",
        target = "@c$n@W slashes you across the leg!@n",
        room   = "@c$n@W slashes @C$N@W across the leg!@n",
    },
}

return {
    id   = "slash",
    family = "weapon",
    name = "Slash",
    skill = "sword",
    weapon_type = "slash",
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
            return false, "You need to wield a weapon to slash."
        end
        if weap:weapon_damtype_get() ~= "slash" then
            return false, "You need a slashing weapon for that."
        end
        if weap:is_broken() then
            return false, "Your weapon is broken!"
        end
        inst.weapon = weap
    end,

    on_check_attack_offense = function(inst)
        local wlvl = inst.weapon and inst.weapon:weapon_level_get() or 0
        local mult = WLVL_MULT[wlvl]
        if mult then
            inst.damage = math.floor(inst.damage * mult)
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
        -- TODO: cut_limb(attacker, vict, wlvl, hitspot) — needs C++ Lua API
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
            actor  = "@C$N@W dodges your slash!@n",
            target = "@WYou dodge @c$n's@W slash!@n",
            room   = "@C$N@W dodges @c$n's@W slash!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W blocks your slash!@n",
            target = "@WYou block @c$n's@W slash!@n",
            room   = "@C$N@W blocks @c$n's@W slash!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your slash!@n",
            target = "@WYou parry @c$n's@W slash!@n",
            room   = "@C$N@W parries @c$n's@W slash!@n",
        }, ctx)
    end,
}
