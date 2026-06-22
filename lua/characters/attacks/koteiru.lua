local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@CYou create a huge floating wave with your ki and slam it at @W$N@C! As it hits $S chest it freezes solid around $M!@n",
        target = "@c$n@C creates a huge floating wave with $s ki and slams it at YOU! As it hits YOUR chest it freezes solid around YOU!@n",
        room   = "@c$n@C creates a huge floating wave with $s ki and slams it at @W$N@C! As it hits $S chest it freezes solid around @W$N@C!@n",
    },
    head = {
        actor  = "@CYou create a huge floating wave with your ki and slam it at @W$N@C! As it hits $S head it freezes solid around $M!@n",
        target = "@c$n@C creates a huge floating wave with $s ki and slams it at YOU! As it hits YOUR head it freezes solid around YOU!@n",
        room   = "@c$n@C creates a huge floating wave with $s ki and slams it at @W$N@C! As it hits $S head it freezes solid around @W$N@C!@n",
    },
    arm = {
        actor  = "@CYou create a huge floating wave with your ki and slam it at @W$N@C! As it hits $S arm it freezes solid around $M!@n",
        target = "@c$n@C creates a huge floating wave with $s ki and slams it at YOU! As it hits YOUR arm it freezes solid around YOU!@n",
        room   = "@c$n@C creates a huge floating wave with $s ki and slams it at @W$N@C! As it hits $S arm it freezes solid around @W$N@C!@n",
    },
    leg = {
        actor  = "@CYou create a huge floating wave with your ki and slam it at @W$N@C! As it hits $S leg it freezes solid around $M!@n",
        target = "@c$n@C creates a huge floating wave with $s ki and slams it at YOU! As it hits YOUR leg it freezes solid around YOU!@n",
        room   = "@c$n@C creates a huge floating wave with $s ki and slams it at @W$N@C! As it hits $S leg it freezes solid around @W$N@C!@n",
    },
}

return {
    id     = "koteiru",
    family = "ki",
    name   = "Koteiru Bakuha",
    skill  = "koteiru bakuha",
    tier   = 3,
    elements = { ki = 1.0 },
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
    base_power    = 2.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ch   = inst.attacker
        local vict = inst.target
        ki.aqua_barrier_boost(ch, inst.damage)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = ch, target = vict })
        -- 25% chance to freeze (blocked if already frozen or demon)
        if inst.damage > 0 and math.random(4) == 1
            and not vict:condition_has("frozen")
            and vict:race_get() ~= "demon" then
            vict:condition_apply("frozen", "combat", "frozen")
            act().message({
                actor  = "@CYour attack freezes $N solid!@n",
                target = "@CYour body completely freezes!@n",
                room   = "@c$n@C freezes $s body completely!@n",
            }, { actor = ch, target = vict })
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your Koteiru Bakuha misses, flying through the air harmlessly!@n",
            target = "@c$n@W fires a Koteiru Bakuha at you, but misses!@n",
            room   = "@c$n@W fires a Koteiru Bakuha at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Koteiru Bakuha, letting it fly harmlessly by!@n",
            target = "@WYou dodge @C$n's@W Koteiru Bakuha, letting it fly harmlessly by!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Koteiru Bakuha, letting it fly harmlessly by!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your Koteiru Bakuha!@n",
            target = "@WYou block @C$n's@W Koteiru Bakuha!@n",
            room   = "@C$N@W blocks @c$n's@W Koteiru Bakuha!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
