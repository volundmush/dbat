local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou aim your mouth at @C$N@W and fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S body with searing heat!@n",
        target = "@c$n@W aims $s mouth at YOU and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! It blasts into YOUR body with searing heat!@n",
        room   = "@c$n@W aims $s mouth at @c$N@W and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S body with searing heat!@n",
    },
    head = {
        actor  = "@WYou aim your mouth at @C$N@W and fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S head with searing heat!@n",
        target = "@c$n@W aims $s mouth at YOU and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! It blasts into YOUR head with searing heat!@n",
        room   = "@c$n@W aims $s mouth at @c$N@W and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S head with searing heat!@n",
    },
    arm = {
        actor  = "@WYou aim your mouth at @C$N@W and fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S arm with searing heat!@n",
        target = "@c$n@W aims $s mouth at YOU and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! It blasts into YOUR arm with searing heat!@n",
        room   = "@c$n@W aims $s mouth at @c$N@W and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S arm with searing heat!@n",
    },
    leg = {
        actor  = "@WYou aim your mouth at @C$N@W and fire a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S leg with searing heat!@n",
        target = "@c$n@W aims $s mouth at YOU and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at YOU! It blasts into YOUR leg with searing heat!@n",
        room   = "@c$n@W aims $s mouth at @c$N@W and fires a large @Rr@re@Rd@W @rS@Re@Wi@ws@rh@Ro@Wu @wE@rn@Rk@Wo at $M! It blasts into $S leg with searing heat!@n",
    },
}

return {
    id     = "seishou",
    family = "ki",
    name   = "Seishou Enko",
    skill  = "seishou enko",
    tier   = 1,
    elements = { ki = 1.0 },
    limbs_required = {},
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 1.0,

    on_check = function(inst)
        local ch = inst.attacker
        if ch:condition_has("mystic_melody") then
            return false, "You are currently playing a song! Enter the song command in order to stop!"
        end
        return ki.can_grav(ch)
    end,

    on_calculate_damage = function(inst)
        -- Arlian molt level 150+ doubles damage
        if (inst.attacker:stat_get("molt_level") or 0) >= 150 then
            inst.damage = inst.damage * 2
        end
    end,

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your Seishou Enko misses, flying through the air harmlessly!@n",
            target = "@c$n@W fires a Seishou Enko at you, but misses!@n",
            room   = "@c$n@W fires a Seishou Enko at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Seishou Enko, letting it fly harmlessly by!@n",
            target = "@WYou dodge @C$n's@W Seishou Enko, letting it fly harmlessly by!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Seishou Enko, letting it fly harmlessly by!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
