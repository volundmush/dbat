local function act() return require("dbat").lib.act end

return {
    id   = "elbow",
    name = "Elbow",
    skill = "elbow",
    family = "melee",
    tier  = 2,
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
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 300)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then return false end
        return ch:skill_known("elbow")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou drive your elbow into @C$N's@W face!@n",
                target = "@C$n@W drives $s elbow into your face!@n",
                room   = "@c$n@W drives $s elbow into @C$N's@W face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou smash your elbow against @C$N's@W arm!@n",
                target = "@C$n@W smashes $s elbow against your arm!@n",
                room   = "@c$n@W smashes $s elbow against @C$N's@W arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou drive your elbow down into @C$N's@W leg!@n",
                target = "@C$n@W drives $s elbow down into your leg!@n",
                room   = "@c$n@W drives $s elbow down into @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou drive your elbow into @C$N's@W gut!@n",
                target = "@C$n@W drives $s elbow into your gut!@n",
                room   = "@c$n@W drives $s elbow into @C$N's@W gut!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou swing your elbow but completely miss!@n",
            target = "@C$n@W swings an elbow at you but misses!@n",
            room   = "@c$n@W swings an elbow at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W sidesteps your elbow!@n",
            target = "@WYou sidestep @C$n's@W elbow!@n",
            room   = "@C$N@W sidesteps @c$n's@W elbow!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W raises $S arm and blocks your elbow!@n",
            target = "@WYou raise your arm and block @C$n's@W elbow!@n",
            room   = "@C$N@W raises $S arm and blocks @c$n's@W elbow!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your elbow with $S own!@n",
            target = "@WYou parry @C$n's@W elbow with your own!@n",
            room   = "@C$N@W parries @c$n's@W elbow!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
