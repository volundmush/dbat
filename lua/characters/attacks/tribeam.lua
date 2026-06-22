local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou cross your fingers and unleash a @Ytri-beam@W that slams into @C$N's@W body!@n",
        target = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into your body!@n",
        room   = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou cross your fingers and unleash a @Ytri-beam@W that slams into @C$N's@W head!@n",
        target = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into your head!@n",
        room   = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou cross your fingers and unleash a @Ytri-beam@W that slams into @C$N's@W arm!@n",
        target = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into your arm!@n",
        room   = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou cross your fingers and unleash a @Ytri-beam@W that slams into @C$N's@W leg!@n",
        target = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into your leg!@n",
        room   = "@c$n@W crosses $s fingers and unleashes a @Ytri-beam@W that slams into @C$N's@W leg!@n",
    },
}

return {
    id     = "tribeam",
    family = "ki",
    name   = "Tri-Beam",
    skill  = "tribeam",
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
            actor  = "@WYou fire a tri-beam at @C$N@W but miss!@n",
            target = "@c$n@W fires a tri-beam at you but misses!@n",
            room   = "@c$n@W fires a tri-beam at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your tri-beam!@n",
            target = "@WYou dodge @c$n@W's tri-beam!@n",
            room   = "@C$N@W dodges @c$n@W's tri-beam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your tri-beam!@n",
            target = "@WYou block @c$n@W's tri-beam!@n",
            room   = "@C$N@W blocks @c$n@W's tri-beam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
