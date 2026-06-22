local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou fix your gaze on @C$N@W and fire a piercing @Ykousengan@W beam into $S body!@n",
        target = "@c$n@W fixes $s gaze on you and fires a piercing @Ykousengan@W beam into your body!@n",
        room   = "@c$n@W fixes $s gaze on @C$N@W and fires a piercing @Ykousengan@W beam into $S body!@n",
    },
    head = {
        actor  = "@WYou fix your gaze on @C$N@W and fire a piercing @Ykousengan@W beam into $S head!@n",
        target = "@c$n@W fixes $s gaze on you and fires a piercing @Ykousengan@W beam into your head!@n",
        room   = "@c$n@W fixes $s gaze on @C$N@W and fires a piercing @Ykousengan@W beam into $S head!@n",
    },
    arm = {
        actor  = "@WYou fix your gaze on @C$N@W and fire a piercing @Ykousengan@W beam into $S arm!@n",
        target = "@c$n@W fixes $s gaze on you and fires a piercing @Ykousengan@W beam into your arm!@n",
        room   = "@c$n@W fixes $s gaze on @C$N@W and fires a piercing @Ykousengan@W beam into $S arm!@n",
    },
    leg = {
        actor  = "@WYou fix your gaze on @C$N@W and fire a piercing @Ykousengan@W beam into $S leg!@n",
        target = "@c$n@W fixes $s gaze on you and fires a piercing @Ykousengan@W beam into your leg!@n",
        room   = "@c$n@W fixes $s gaze on @C$N@W and fires a piercing @Ykousengan@W beam into $S leg!@n",
    },
}

return {
    id     = "kousengan",
    family = "ki",
    name   = "Kousengan",
    skill  = "kousengan",
    tier   = 1,
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
    base_power    = 0.8,

    on_check = function(inst)
        if inst.attacker:condition_has("mystic_melody") then
            return false, "Your mystic melody prevents you from using kousengan."
        end
        return ki.can_grav(inst.attacker)
    end,

    on_modify_accuracy = function(inst)
        inst.accuracy_modifier = 15
    end,

    on_hit = function(inst)
        -- Sanctuary bonus: energy beams amplified against sanctuary field
        if inst.target:condition_has("sanctuary") then
            inst.damage = math.floor(inst.damage * 3)
        end
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYour kousengan misses @C$N@W!@n",
            target = "@c$n@W fires kousengan beams at you but misses!@n",
            room   = "@c$n@W fires kousengan beams at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your kousengan!@n",
            target = "@WYou dodge @c$n@W's kousengan!@n",
            room   = "@C$N@W dodges @c$n@W's kousengan!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your kousengan!@n",
            target = "@WYou block @c$n@W's kousengan!@n",
            room   = "@C$N@W blocks @c$n@W's kousengan!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
