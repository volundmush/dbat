local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou unleash a barrage of @Mpsychic blasts@W that tear into @C$N's@W body!@n",
        target = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into your body!@n",
        room   = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou unleash a barrage of @Mpsychic blasts@W that tear into @C$N's@W head with devastating force!@n",
        target = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into your head with devastating force!@n",
        room   = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into @C$N's@W head with devastating force!@n",
    },
    arm = {
        actor  = "@WYou unleash a barrage of @Mpsychic blasts@W that tear into @C$N's@W arm!@n",
        target = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into your arm!@n",
        room   = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou unleash a barrage of @Mpsychic blasts@W that tear into @C$N's@W leg!@n",
        target = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into your leg!@n",
        room   = "@c$n@W unleashes a barrage of @Mpsychic blasts@W that tear into @C$N's@W leg!@n",
    },
}

return {
    id     = "pbarrage",
    family = "ki",
    name   = "Psychic Barrage",
    skill  = "psychic barrage",
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
        -- Head hit applies a critical damage multiplier
        if inst.hit_location == "head" then
            inst.damage = math.floor(inst.damage * 1.5)
        end
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour psychic barrage misses @C$N@W completely!@n",
            target = "@c$n@W fires a psychic barrage at you but misses!@n",
            room   = "@c$n@W fires a psychic barrage at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your psychic barrage!@n",
            target = "@WYou dodge @c$n@W's psychic barrage!@n",
            room   = "@C$N@W dodges @c$n@W's psychic barrage!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your psychic barrage!@n",
            target = "@WYou block @c$n@W's psychic barrage!@n",
            room   = "@C$N@W blocks @c$n@W's psychic barrage!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
