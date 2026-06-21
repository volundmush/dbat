local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou leap forward with a ferocious @Ywolf fang fist@W and drive it into @C$N's@W body!@n",
        target = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into your body!@n",
        room   = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou leap forward with a ferocious @Ywolf fang fist@W and drive it into @C$N's@W head!@n",
        target = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into your head!@n",
        room   = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou leap forward with a ferocious @Ywolf fang fist@W and drive it into @C$N's@W arm!@n",
        target = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into your arm!@n",
        room   = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou leap forward with a ferocious @Ywolf fang fist@W and drive it into @C$N's@W leg!@n",
        target = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into your leg!@n",
        room   = "@c$n@W leaps forward with a ferocious @Ywolf fang fist@W and drives it into @C$N's@W leg!@n",
    },
}

return {
    id    = "rogafufuken",
    family = "ki",
    name  = "Rogafufuken",
    skill = "rogafufuken",
    tier  = 2,
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
    base_power    = 1.3,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_calculate_cost = function(inst)
        -- Rogafufuken also drains stamina
        inst.cost.stamina = math.floor(inst.attacker:meter_max("stamina") / 50)
    end,

    on_hit = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W deflects your rogafufuken and counters with a fierce strike!@n",
            target = "@WYou deflect @c$n@W's rogafufuken and counter with a fierce strike!@n",
            room   = "@C$N@W deflects @c$n@W's rogafufuken and counters!@n",
        }, ctx)
        -- Counter-damage dealt back to the attacker
        local counter = math.floor(inst.damage / 4)
        if counter > 0 then
            inst.attacker:meter_mod_int("powerlevel", -counter)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou lunge forward with a rogafufuken but miss @C$N@W!@n",
            target = "@c$n@W lunges at you with a rogafufuken but misses!@n",
            room   = "@c$n@W lunges at @C$N@W with a rogafufuken but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W sidesteps your rogafufuken!@n",
            target = "@WYou sidestep @c$n@W's rogafufuken!@n",
            room   = "@C$N@W sidesteps @c$n@W's rogafufuken!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your rogafufuken!@n",
            target = "@WYou block @c$n@W's rogafufuken!@n",
            room   = "@C$N@W blocks @c$n@W's rogafufuken!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
