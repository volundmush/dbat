local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou thrust your palm forward and release a massive @Ybig bang@W that engulfs @C$N's@W body!@n",
        target = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs your body!@n",
        room   = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou thrust your palm forward and release a massive @Ybig bang@W that engulfs @C$N's@W head!@n",
        target = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs your head!@n",
        room   = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou thrust your palm forward and release a massive @Ybig bang@W that engulfs @C$N's@W arm!@n",
        target = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs your arm!@n",
        room   = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou thrust your palm forward and release a massive @Ybig bang@W that engulfs @C$N's@W leg!@n",
        target = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs your leg!@n",
        room   = "@c$n@W thrusts $s palm forward and releases a massive @Ybig bang@W that engulfs @C$N's@W leg!@n",
    },
}

return {
    id     = "bigbang",
    family = "ki",
    name   = "Big Bang",
    skill  = "big bang",
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

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou release a big bang at @C$N@W but miss!@n",
            target = "@c$n@W releases a big bang at you but misses!@n",
            room   = "@c$n@W releases a big bang at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your big bang!@n",
            target = "@WYou dodge @c$n@W's big bang!@n",
            room   = "@C$N@W dodges @c$n@W's big bang!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your big bang!@n",
            target = "@WYou block @c$n@W's big bang!@n",
            room   = "@C$N@W blocks @c$n@W's big bang!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
