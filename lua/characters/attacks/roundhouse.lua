local function act() return require("dbat").lib.act end

return {
    id   = "roundhouse",
    name = "Roundhouse",
    skill = "roundhouse",
    family = "melee",
    tier  = 3,
    elements = { blunt = 1.0 },
    limbs_required = { "legs" },
    damages_limbs = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block  = true,
    can_parry  = true,
    can_dodge  = true,
    consumes_charge = false,
    base_accuracy = 0.9,
    base_power    = 1.0,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 200)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then return false end
        return ch:skill_known("roundhouse")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYour spinning heel connects with @C$N's@W temple!@n",
                target = "@C$n's@W spinning heel connects with your temple!@n",
                room   = "@c$n's@W spinning heel connects with @C$N's@W temple!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYour spinning kick catches @C$N@W across the arm!@n",
                target = "@C$n's@W spinning kick catches you across the arm!@n",
                room   = "@c$n's@W spinning kick catches @C$N@W across the arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYour roundhouse sweep catches @C$N's@W leg!@n",
                target = "@C$n's@W roundhouse sweep catches your leg!@n",
                room   = "@c$n's@W roundhouse sweep catches @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou spin and your heel crashes into @C$N's@W side!@n",
                target = "@C$n@W spins and $s heel crashes into your side!@n",
                room   = "@c$n@W spins and $s heel crashes into @C$N's@W side!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou spin with a roundhouse kick but miss entirely!@n",
            target = "@C$n@W spins with a roundhouse kick and misses!@n",
            room   = "@c$n@W spins with a roundhouse kick at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W ducks under your roundhouse!@n",
            target = "@WYou duck under @C$n's@W roundhouse kick!@n",
            room   = "@C$N@W ducks under @c$n's@W roundhouse kick!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W raises $S guard and absorbs your roundhouse!@n",
            target = "@WYou raise your guard and absorb @C$n's@W roundhouse!@n",
            room   = "@C$N@W raises $S guard and absorbs @c$n's@W roundhouse!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W catches your spinning heel and returns it!@n",
            target = "@WYou catch @C$n's@W spinning heel and return it!@n",
            room   = "@C$N@W catches @c$n's@W roundhouse and counters!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
