local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou raise both hands and aim the flats of your palms toward @c$N@W. As you concentrate your charged ki long arcing beams of blue energy shoot out and form a field around $M. With a quick motion you move your hands in opposite directions and wrench @c$N's@W body with the force of your energy!@n",
        target = "@C$n@W raises both hands and aims the flats of $s palms toward YOU. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around YOU. With a quick motion $e moves $s hands in opposite directions and wrenches YOUR body with the force of $s energy!@n",
        room   = "@C$n@W raises both hands and aims the flats of $s palms toward @c$N@W. As $e concentrates $s charged ki long arcing beams of blue energy shoot out and form a field around @c$N@W. With a quick motion @C$n@W moves $s hands in opposite directions and wrenches @c$N's@W body with the force of $s energy!@n",
    },
    head = {
        actor  = "@WYou raise both hands and wrench @c$N's@W head with arcing beams of ki energy!@n",
        target = "@C$n@W raises both hands and wrenches YOUR head with arcing beams of ki energy!@n",
        room   = "@C$n@W raises both hands and wrenches @c$N's@W head with arcing beams of ki energy!@n",
    },
    arm = {
        actor  = "@WYou raise both hands and wrench @c$N's@W arm with arcing beams of ki energy!@n",
        target = "@C$n@W raises both hands and wrenches YOUR arm with arcing beams of ki energy!@n",
        room   = "@C$n@W raises both hands and wrenches @c$N's@W arm with arcing beams of ki energy!@n",
    },
    leg = {
        actor  = "@WYou raise both hands and wrench @c$N's@W leg with arcing beams of ki energy!@n",
        target = "@C$n@W raises both hands and wrenches YOUR leg with arcing beams of ki energy!@n",
        room   = "@C$n@W raises both hands and wrenches @c$N's@W leg with arcing beams of ki energy!@n",
    },
}

return {
    id     = "sunder",
    family = "ki",
    name   = "Sundering Force",
    skill  = "sundering force",
    tier   = 3,
    elements = { ki = 1.0 },
    consumes_charge = true,
    limbs_required = { "arm" },
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
            actor  = "@WYou can't believe it but your Sundering Force misses, flying through the air harmlessly!@n",
            target = "@C$n@W fires a Sundering Force at you, but misses!@n",
            room   = "@c$n@W fires a Sundering Force at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Sundering Force, letting it fly harmlessly by!@n",
            target = "@WYou dodge @C$n's@W Sundering Force, letting it fly harmlessly by!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Sundering Force, letting it fly harmlessly by!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your Sundering Force!@n",
            target = "@WYou block @C$n's@W Sundering Force!@n",
            room   = "@C$N@W blocks @c$n's@W Sundering Force!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
