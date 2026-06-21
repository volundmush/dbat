local function act() return require("dbat").lib.act end

return {
    id   = "headbutt",
    name = "Headbutt",
    skill = "headbutt",
    family = "melee",
    tier  = 3,
    elements = { blunt = 1.0 },
    damages_limbs = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block  = true,
    can_parry  = true,
    can_dodge  = true,
    consumes_charge = false,
    base_accuracy = 1.0,
    base_power    = 1.1,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 100)
    end,

    on_check_combo = function(ch)
        return ch:skill_known("headbutt")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou drive your skull straight into @C$N's@W face!@n",
                target = "@C$n@W drives $s skull straight into your face!@n",
                room   = "@c$n@W drives $s skull straight into @C$N's@W face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou headbutt @C$N@W in the arm!@n",
                target = "@C$n@W headbutts you in the arm!@n",
                room   = "@c$n@W headbutts @C$N@W in the arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou drop your head into @C$N's@W leg!@n",
                target = "@C$n@W drops $s head into your leg!@n",
                room   = "@c$n@W drops $s head into @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou slam your head into @C$N's@W chest!@n",
                target = "@C$n@W slams $s head into your chest!@n",
                room   = "@c$n@W slams $s head into @C$N's@W chest!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou can't believe it, your headbutt misses!@n",
            target = "@C$n@W throws a headbutt at you, but thankfully misses!@n",
            room   = "@c$n@W throws a headbutt at @C$N@W, but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W dodges your headbutt!@n",
            target = "@WYou dodge @C$n's@W headbutt!@n",
            room   = "@C$N@W dodges @c$n's@W headbutt!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W blocks your headbutt!@n",
            target = "@WYou block @C$n's@W headbutt!@n",
            room   = "@C$N@W blocks @c$n's@W headbutt!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your headbutt with an attack of $S own!@n",
            target = "@WYou parry @C$n's@W headbutt with an attack of your own!@n",
            room   = "@C$N@W parries @c$n's@W headbutt with an attack of $S own!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
