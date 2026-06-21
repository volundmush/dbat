local function act() return require("dbat").lib.act end

return {
    id   = "uppercut",
    name = "Uppercut",
    skill = "uppercut",
    family = "melee",
    tier  = 3,
    elements = { blunt = 1.0 },
    limbs_required = { "arms" },
    damages_limbs = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block  = true,
    can_parry  = true,
    can_dodge  = true,
    consumes_charge = false,
    base_accuracy = 1.0,
    base_power    = 1.0,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 200)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then return false end
        return ch:skill_known("uppercut")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou drive an uppercut straight into @C$N's@W jaw!@n",
                target = "@C$n@W drives an uppercut straight into your jaw!@n",
                room   = "@c$n@W drives an uppercut straight into @C$N's@W jaw!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYour uppercut catches @C$N@W under the arm!@n",
                target = "@C$n's@W uppercut catches you under the arm!@n",
                room   = "@c$n's@W uppercut catches @C$N@W under the arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYour rising fist clips @C$N's@W leg!@n",
                target = "@C$n's@W rising fist clips your leg!@n",
                room   = "@c$n's@W rising fist clips @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou drive an uppercut hard into @C$N's@W chin!@n",
                target = "@C$n@W drives an uppercut hard into your chin!@n",
                room   = "@c$n@W drives an uppercut hard into @C$N's@W chin!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou swing an uppercut wildly and miss!@n",
            target = "@C$n@W swings an uppercut wildly and misses you!@n",
            room   = "@c$n@W swings an uppercut at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W leans back from your uppercut!@n",
            target = "@WYou lean back from @C$n's@W uppercut!@n",
            room   = "@C$N@W leans back from @c$n's@W uppercut!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W crosses $S arms and stops your uppercut!@n",
            target = "@WYou cross your arms and stop @C$n's@W uppercut!@n",
            room   = "@C$N@W crosses $S arms and stops @c$n's@W uppercut!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W deflects your uppercut and counters!@n",
            target = "@WYou deflect @C$n's@W uppercut and counter!@n",
            room   = "@C$N@W deflects @c$n's@W uppercut and counters!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
