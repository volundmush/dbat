local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou form a sphere of crackling energy and hurl a @Rcrusher ball@W that slams into @C$N's@W body!@n",
        target = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into your body!@n",
        room   = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou form a sphere of crackling energy and hurl a @Rcrusher ball@W that slams into @C$N's@W head!@n",
        target = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into your head!@n",
        room   = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou form a sphere of crackling energy and hurl a @Rcrusher ball@W that slams into @C$N's@W arm!@n",
        target = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into your arm!@n",
        room   = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou form a sphere of crackling energy and hurl a @Rcrusher ball@W that slams into @C$N's@W leg!@n",
        target = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into your leg!@n",
        room   = "@c$n@W forms a sphere of crackling energy and hurls a @Rcrusher ball@W that slams into @C$N's@W leg!@n",
    },
}

return {
    id     = "crusher",
    family = "ki",
    name   = "Crusher Ball",
    skill  = "crusher ball",
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

    on_modify_accuracy = function(inst)
        -- Perf type 2: +5 flat accuracy bonus
        if inst.attacker:skill_perf_get("crusher ball") >= 2 then
            inst.accuracy_modifier = 5
        end
    end,

    on_calculate_cost = function(inst)
        -- Perf type 3: reduce minimum ki cost
        if inst.attacker:skill_perf_get("crusher ball") >= 3 then
            local reduction = math.floor(inst.attacker:meter_max("ki") * 0.05)
            inst.cost.ki = math.max(0, (inst.cost.ki or 0) - reduction)
        end
    end,

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou hurl a crusher ball at @C$N@W but miss!@n",
            target = "@c$n@W hurls a crusher ball at you but misses!@n",
            room   = "@c$n@W hurls a crusher ball at @C$N@W but misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W dodges your crusher ball!@n",
            target = "@WYou dodge @c$n@W's crusher ball!@n",
            room   = "@C$N@W dodges @c$n@W's crusher ball!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your crusher ball!@n",
            target = "@WYou block @c$n@W's crusher ball!@n",
            room   = "@C$N@W blocks @c$n@W's crusher ball!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
