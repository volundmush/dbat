local function act() return require("dbat").lib.act end

local WP = nil
local function wear()
    if not WP then WP = require("dbat").consts.wear_positions end
    return WP
end

local WLVL_MULT = { [1]=1.05, [2]=1.10, [3]=1.20, [4]=1.30, [5]=1.50 }

-- Backstab chance by wlvl (0-indexed: 0→0%, 1→10%, 2→15%, 3→20%, 4→25%, 5→30%)
local BACKSTAB_CHANCE = { [0]=0, [1]=10, [2]=15, [3]=20, [4]=25, [5]=30 }
-- Backstab damage bonus multiplier
local BACKSTAB_BONUS  = { [0]=0, [1]=0.5, [2]=2.0, [3]=3.0, [4]=4.0, [5]=4.0 }

local HIT_MSGS = {
    body = {
        actor  = "@WYou pierce @C$N's@W chest!@n",
        target = "@c$n@W pierces your chest!@n",
        room   = "@c$n@W pierces @C$N's@W chest!@n",
    },
    head = {
        actor  = "@WYou pierce @C$N's@W face!@n",
        target = "@c$n@W pierces your face!@n",
        room   = "@c$n@W pierces @C$N's@W face!@n",
    },
    arm = {
        actor  = "@WYou pierce @C$N's@W arm!@n",
        target = "@c$n@W pierces your arm!@n",
        room   = "@c$n@W pierces @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou pierce @C$N's@W leg!@n",
        target = "@c$n@W pierces your leg!@n",
        room   = "@c$n@W pierces @C$N's@W leg!@n",
    },
}

return {
    id   = "pierce",
    name = "Pierce",
    skill = "dagger",
    weapon_type = "pierce",
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
            return false, "You need to wield a weapon to pierce."
        end
        if weap:weapon_damtype_get() ~= "pierce" then
            return false, "You need a piercing weapon for that."
        end
        if weap:is_broken() then
            return false, "Your weapon is broken!"
        end
        inst.weapon = weap
    end,

    on_check_attack_offense = function(inst)
        local ch   = inst.attacker
        local wlvl = inst.weapon and inst.weapon:weapon_level_get() or 0

        -- Weapon level bonus
        local mult = WLVL_MULT[wlvl]
        if mult then inst.damage = math.floor(inst.damage * mult) end

        -- DEX bonus: +0.5% per point of DEX
        local dex = ch:stat_get("dexterity") or 0
        inst.damage = math.floor(inst.damage + inst.damage * 0.01 * (dex * 0.5))
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

        -- Backstab: check when not yet in combat
        local ch   = inst.attacker
        local vict = inst.target
        if not inst.is_multihit and not ch:fighting_get() then
            local skill = ch:skill_get("dagger")
            local wlvl  = inst.weapon and inst.weapon:weapon_level_get() or 0
            local chance = (BACKSTAB_CHANCE[wlvl] or 0)
                         + (skill >= 100 and 20 or skill >= 50 and 10 or 0)
            if math.random(1, 100) <= chance then
                local attk = ch:skill_get("move_silently") + ch:skill_get("spot")
                             + (ch:stat_get("dexterity") or 0) + math.random(-5, 5)
                local defn = vict:skill_get("spot") + vict:skill_get("listen")
                             + (vict:stat_get("dexterity") or 0) + math.random(-5, 5)
                if attk > defn then
                    a.message({
                        actor  = "@RYou manage to sneak behind @r$N@R and stab $M in the back!@n",
                        target = "@RYou feel @r$n's@R dagger thrust into your back unexpectantly!@n",
                        room   = "@r$n@R sneaks up behind @r$N@R and stabs $M in the back!@n",
                    }, ctx)
                    local bonus = BACKSTAB_BONUS[wlvl] or 0
                    inst.damage_to = { lifeforce = math.floor(inst.damage * (1 + bonus)) }
                    -- no further messages
                    if inst.weapon then inst.weapon:weapon_damage(1) end
                    return
                end
            end
        end

        local msg = HIT_MSGS[inst.hit_location] or HIT_MSGS.body
        a.message(msg, ctx)
        if inst.weapon then inst.weapon:weapon_damage(1) end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou thrust at @C$N@W but miss!@n",
            target = "@c$n@W thrusts at you but misses!@n",
            room   = "@c$n@W thrusts at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W dodges your pierce!@n",
            target = "@WYou dodge @c$n's@W pierce!@n",
            room   = "@C$N@W dodges @c$n's@W pierce!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W blocks your thrust!@n",
            target = "@WYou block @c$n's@W thrust!@n",
            room   = "@C$N@W blocks @c$n's@W thrust!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your thrust!@n",
            target = "@WYou parry @c$n's@W thrust!@n",
            room   = "@C$N@W parries @c$n's@W thrust!@n",
        }, ctx)
    end,
}
