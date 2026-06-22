local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou focus your ki into @C$N's@W body and clench your fist! The @Bwater@W in $S body takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n",
        target = "@c$n@W focuses $s ki into YOUR body and clenches $s fist! The @Bwater@W in your body takes the shape of millions of microscopic @Dblades@W that cut up YOUR insides!@n",
        room   = "@c$n@W focuses $s ki into @c$N's@W body and clenches $s fist! The @Bwater@W in @c$N's@W body takes the shape of millions of microscopic @Dblades@W that cut up $S insides!@n",
    },
    head = {
        actor  = "@WYou focus your ki into @C$N's@W head and clench your fist! Blood sprays out in a mist from every pore of $S head!@n",
        target = "@c$n@W focuses $s ki into YOUR head and clenches $s fist! Blood sprays out of your head pores into a fine mist!@n",
        room   = "@c$n@W focuses $s ki into @c$N's@W head and clenches $s fist! Blood sprays out of $S head pores into a fine mist!@n",
    },
    arm = {
        actor  = "@WYou focus your ki into @C$N's@W arm and clench your fist! The @Bwater@W in $S arm takes the shape of millions of microscopic @Dblades@W cutting $M apart from the inside!@n",
        target = "@c$n@W focuses $s ki into YOUR arm and clenches $s fist! The @Bwater@W in your arm takes the shape of millions of microscopic @Dblades@W cutting you apart from the inside!@n",
        room   = "@c$n@W focuses $s ki into @c$N's@W arm and clenches $s fist! The @Bwater@W in $S arm takes the shape of millions of microscopic @Dblades@W cutting $M apart from the inside!@n",
    },
    leg = {
        actor  = "@WYou focus your ki into @C$N's@W leg and clench your fist! The @Bwater@W in $S leg takes the shape of millions of microscopic @Dblades@W cutting $M apart from the inside!@n",
        target = "@c$n@W focuses $s ki into YOUR leg and clenches $s fist! The @Bwater@W in your leg takes the shape of millions of microscopic @Dblades@W cutting you apart from the inside!@n",
        room   = "@c$n@W focuses $s ki into @c$N's@W leg and clenches $s fist! The @Bwater@W in $S leg takes the shape of millions of microscopic @Dblades@W cutting $M apart from the inside!@n",
    },
}

return {
    id     = "waterrazor",
    family = "ki",
    name   = "Water Razor",
    skill  = "water razor",
    tier   = 2,
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
    base_power    = 1.8,

    on_check = function(inst)
        local ch     = inst.attacker
        local target = inst.target
        if target and target:race_get() == "android" then
            return false, "There is not a necessary amount of water in cybernetic creatures."
        end
        return ki.can_grav(ch)
    end,

    on_hit = function(inst)
        local ch   = inst.attacker
        local vict = inst.target
        ki.aqua_barrier_boost(ch, inst.damage)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = ch, target = vict })
        -- Internal water blades also drain ki and stamina equal to damage
        local drain = inst.damage
        vict:meter_mod_int("ki",      -drain)
        vict:meter_mod_int("stamina", -drain)
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your Water Razor misses, flying through the air harmlessly!@n",
            target = "@c$n@W fires a Water Razor at you, but misses!@n",
            room   = "@c$n@W fires a Water Razor at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Water Razor, letting it fly harmlessly by!@n",
            target = "@WYou dodge @C$n's@W Water Razor, letting it fly harmlessly by!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Water Razor, letting it fly harmlessly by!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
