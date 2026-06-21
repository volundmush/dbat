local function act() return require("dbat").lib.act end

local WP = nil
local function wear()
    if not WP then WP = require("dbat").consts.wear_positions end
    return WP
end

local WLVL_MULT = { [1]=1.05, [2]=1.10, [3]=1.20, [4]=1.30, [5]=1.50 }

-- club_stamina drain fraction by wlvl
local STAM_DRAIN = { [0]=0, [1]=0.05, [2]=0.10, [3]=0.15, [4]=0.20, [5]=0.25 }

local HIT_MSGS = {
    body = {
        actor  = "@WYou crush @C$N's@W chest!@n",
        target = "@c$n@W crushes your chest!@n",
        room   = "@c$n@W crushes @C$N's@W chest!@n",
    },
    head = {
        actor  = "@WYou crush @C$N@W in the face!@n",
        target = "@c$n@W crushes you in the face!@n",
        room   = "@c$n@W crushes @C$N@W in the face!@n",
    },
    arm = {
        actor  = "@WYou crush @C$N's@W arm!@n",
        target = "@c$n@W crushes your arm!@n",
        room   = "@c$n@W crushes @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou crush @C$N's@W leg!@n",
        target = "@c$n@W crushes your leg!@n",
        room   = "@c$n@W crushes @C$N's@W leg!@n",
    },
}

return {
    id   = "crush",
    name = "Crush",
    skill = "club",
    weapon_type = "crush",
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
            return false, "You need to wield a weapon to crush."
        end
        if weap:weapon_damtype_get() ~= "crush" then
            return false, "You need a crushing weapon for that."
        end
        if weap:is_broken() then
            return false, "Your weapon is broken!"
        end
        inst.weapon = weap
    end,

    on_check_attack_offense = function(inst)
        local wlvl = inst.weapon and inst.weapon:weapon_level_get() or 0
        local mult = WLVL_MULT[wlvl]
        if mult then inst.damage = math.floor(inst.damage * mult) end
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
        local a    = act()
        local ctx  = { actor = inst.attacker, target = inst.target }
        local msg  = HIT_MSGS[inst.hit_location] or HIT_MSGS.body
        a.message(msg, ctx)

        -- Stamina drain on victim (club_stamina equivalent)
        local ch   = inst.attacker
        local vict = inst.target
        local skill = ch:skill_get("club")
        local wlvl  = inst.weapon and inst.weapon:weapon_level_get() or 0
        local drain = (STAM_DRAIN[wlvl] or 0)
                    + (skill >= 100 and 0.10 or skill >= 50 and 0.05 or 0)
        if drain > 0 then
            local drained = math.floor(inst.damage * drain)
            vict:meter_mod_int("stamina", -drained)
            ch:send_line("@D[@YVictim's @GStamina @cLoss@W: @g%d@D]@n", drained)
            vict:send_line("@D[@rYour @GStamina @cLoss@W: @g%d@D]@n", drained)
        end

        if inst.weapon then inst.weapon:weapon_damage(1) end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou swing your club at @C$N@W but miss!@n",
            target = "@c$n@W swings a club at you but misses!@n",
            room   = "@c$n@W swings a club at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W dodges your crushing blow!@n",
            target = "@WYou dodge @c$n's@W crushing blow!@n",
            room   = "@C$N@W dodges @c$n's@W crushing blow!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W blocks your crushing blow!@n",
            target = "@WYou block @c$n's@W crushing blow!@n",
            room   = "@C$N@W blocks @c$n's@W crushing blow!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your crushing blow!@n",
            target = "@WYou parry @c$n's@W crushing blow!@n",
            room   = "@C$N@W parries @c$n's@W crushing blow!@n",
        }, ctx)
    end,
}
