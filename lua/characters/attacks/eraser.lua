local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou raise your hand and unleash a massive @Geraser cannon@W that slams into @C$N's@W body!@n",
        target = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into your body!@n",
        room   = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou raise your hand and unleash a massive @Geraser cannon@W that slams into @C$N's@W head!@n",
        target = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into your head!@n",
        room   = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou raise your hand and unleash a massive @Geraser cannon@W that slams into @C$N's@W arm!@n",
        target = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into your arm!@n",
        room   = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou raise your hand and unleash a massive @Geraser cannon@W that slams into @C$N's@W leg!@n",
        target = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into your leg!@n",
        room   = "@c$n@W raises $s hand and unleashes a massive @Geraser cannon@W that slams into @C$N's@W leg!@n",
    },
}

return {
    id    = "eraser",
    family = "ki",
    name  = "Eraser Cannon",
    skill = "eraser cannon",
    tier  = 3,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = true,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 2.2,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)
    end,

    on_after_cost = function(inst)
        -- Ki refund mastery: drain some ki back from the technique's charge
        local skill = inst.skill_level
        local amt   = inst.cost.ki or 0
        if amt <= 0 then return end
        local refund
        if     skill >= 100 then refund = math.floor(amt * 0.15)
        elseif skill >= 60  then refund = math.floor(amt * 0.10)
        elseif skill >= 40  then refund = math.floor(amt * 0.05)
        end
        if refund and refund > 0 then
            inst.attacker:meter_mod_int("ki", refund)
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour eraser cannon misses @C$N@W completely!@n",
            target = "@c$n@W fires an eraser cannon at you but misses!@n",
            room   = "@c$n@W fires an eraser cannon at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your eraser cannon!@n",
            target = "@WYou dodge @c$n@W's eraser cannon!@n",
            room   = "@C$N@W dodges @c$n@W's eraser cannon!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your eraser cannon!@n",
            target = "@WYou block @c$n@W's eraser cannon!@n",
            room   = "@C$N@W blocks @c$n@W's eraser cannon!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_absorbed = function(inst)
        act().message({
            actor  = "@C$N@W absorbs your eraser cannon into $S android systems!@n",
            target = "@WYou absorb @c$n@W's eraser cannon into your android systems!@n",
            room   = "@C$N@W absorbs @c$n@W's eraser cannon into $S android systems!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
