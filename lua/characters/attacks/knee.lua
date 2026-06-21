local function act() return require("dbat").lib.act end

return {
    id   = "knee",
    name = "Knee",
    skill = "knee",
    family = "melee",
    tier  = 2,
    elements = { blunt = 1.0 },
    limbs_required = { "legs" },
    damages_limbs = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block  = true,
    can_parry  = true,
    can_dodge  = true,
    consumes_charge = false,
    base_accuracy = 1.0,
    base_power    = 1.2,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 250)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then return false end
        return ch:skill_get("knee")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou bring your knee up into @C$N's@W face!@n",
                target = "@C$n@W brings $s knee up into your face!@n",
                room   = "@c$n@W brings $s knee up into @C$N's@W face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou knee @C$N@W in the arm!@n",
                target = "@C$n@W knees you in the arm!@n",
                room   = "@c$n@W knees @C$N@W in the arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou slam your knee into @C$N's@W leg!@n",
                target = "@C$n@W slams $s knee into your leg!@n",
                room   = "@c$n@W slams $s knee into @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou drive your knee hard into @C$N's@W gut!@n",
                target = "@C$n@W drives $s knee hard into your gut!@n",
                room   = "@c$n@W drives $s knee hard into @C$N's@W gut!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou try to drive your knee but it goes wide!@n",
            target = "@C$n@W tries to drive $s knee into you but misses!@n",
            room   = "@c$n@W tries to knee @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W steps back from your knee!@n",
            target = "@WYou step back from @C$n's@W knee!@n",
            room   = "@C$N@W steps back from @c$n's@W knee!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W catches your knee with $S hands!@n",
            target = "@WYou catch @C$n's@W knee with your hands!@n",
            room   = "@C$N@W catches @c$n's@W knee!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W counters your knee with a knee of $S own!@n",
            target = "@WYou counter @C$n's@W knee with a knee of your own!@n",
            room   = "@C$N@W counters @c$n's@W knee!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
