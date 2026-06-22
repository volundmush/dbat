local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou draw your arms back and release a blinding @Yfinal flash@W that engulfs @C$N's@W body!@n",
        target = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs your body!@n",
        room   = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou draw your arms back and release a blinding @Yfinal flash@W that engulfs @C$N's@W head!@n",
        target = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs your head!@n",
        room   = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou draw your arms back and release a blinding @Yfinal flash@W that engulfs @C$N's@W arm!@n",
        target = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs your arm!@n",
        room   = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou draw your arms back and release a blinding @Yfinal flash@W that engulfs @C$N's@W leg!@n",
        target = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs your leg!@n",
        room   = "@c$n@W draws $s arms back and releases a blinding @Yfinal flash@W that engulfs @C$N's@W leg!@n",
    },
}

return {
    id     = "finalflash",
    family = "ki",
    name   = "Final Flash",
    skill  = "final flash",
    tier   = 4,
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
    base_power    = 2.5,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou release your final flash at @C$N@W but miss!@n",
            target = "@c$n@W releases a final flash at you but misses!@n",
            room   = "@c$n@W releases a final flash at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your final flash!@n",
            target = "@WYou dodge @c$n@W's final flash!@n",
            room   = "@C$N@W dodges @c$n@W's final flash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your final flash!@n",
            target = "@WYou block @c$n@W's final flash!@n",
            room   = "@C$N@W blocks @c$n@W's final flash!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
