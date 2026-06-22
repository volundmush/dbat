local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou raise a finger skyward and bring down a massive @Rdeathball@W upon @C$N's@W body!@n",
        target = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon your body!@n",
        room   = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou raise a finger skyward and bring down a massive @Rdeathball@W upon @C$N's@W head!@n",
        target = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon your head!@n",
        room   = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou raise a finger skyward and bring down a massive @Rdeathball@W upon @C$N's@W arm!@n",
        target = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon your arm!@n",
        room   = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou raise a finger skyward and bring down a massive @Rdeathball@W upon @C$N's@W leg!@n",
        target = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon your leg!@n",
        room   = "@c$n@W raises a finger skyward and brings down a massive @Rdeathball@W upon @C$N's@W leg!@n",
    },
}

return {
    id     = "deathball",
    family = "ki",
    name   = "Deathball",
    skill  = "deathball",
    tier   = 5,
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
    base_power    = 3.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_modify_accuracy = function(inst)
        -- Deathball is slow and unwieldy — harder to aim
        inst.accuracy_modifier = -math.random(8, 10)
    end,

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour deathball misses @C$N@W and crashes into the ground!@n",
            target = "@c$n@W's deathball misses you and crashes into the ground!@n",
            room   = "@c$n@W's deathball misses @C$N@W and crashes into the ground!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W narrowly dodges your deathball!@n",
            target = "@WYou narrowly dodge @c$n@W's deathball!@n",
            room   = "@C$N@W narrowly dodges @c$n@W's deathball!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your deathball!@n",
            target = "@WYou block @c$n@W's deathball!@n",
            room   = "@C$N@W blocks @c$n@W's deathball!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
