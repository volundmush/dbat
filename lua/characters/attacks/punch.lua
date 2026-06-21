local function act() return require("dbat").lib.act end

return {
    id   = "punch",
    name = "Punch",
    skill = "punch",
    family = "melee",
    tier  = 1,
    elements = { blunt = 1.0 },
    limbs_required = { "arms" },
    damages_limbs = true,
    can_combo = true,
    in_combo  = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block  = true,
    can_parry  = true,
    can_dodge  = true,
    consumes_charge = false,
    min_charge = 0,
    max_charge = 0,
    base_accuracy = 1.0,
    base_power    = 1.0,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 500)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then return false end
        return ch:skill_known("punch")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou slam your fist into @C$N's@W face!@n",
                target = "@C$n@W slams $s fist into your face!@n",
                room   = "@c$n@W slams $s fist into @C$N's@W face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou punch @C$N@W in the arm!@n",
                target = "@C$n@W punches you in the arm!@n",
                room   = "@c$n@W punches @C$N@W in the arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou punch @C$N@W in the leg!@n",
                target = "@C$n@W punches you in the leg!@n",
                room   = "@c$n@W punches @C$N@W in the leg!@n",
            }, ctx)
        else
            if math.random(1, 2) == 1 then
                a.message({
                    actor  = "@WYou slam your fist into @C$N's@W body!@n",
                    target = "@C$n@W slams $s fist into your body!@n",
                    room   = "@c$n@W slams $s fist into @C$N's@W body!@n",
                }, ctx)
            else
                a.message({
                    actor  = "@WYou punch @C$N@W directly in the gut!@n",
                    target = "@C$n@W punches you directly in the gut!@n",
                    room   = "@c$n@W punches @C$N@W directly in the gut!@n",
                }, ctx)
            end
        end
        -- TODO: dam_eq_loc
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou can't believe it but your punch misses!@n",
            target = "@C$n@W throws a punch at you but somehow misses!@n",
            room   = "@c$n@W throws a punch at @C$N@W but somehow misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W manages to dodge your punch!@n",
            target = "@WYou dodge @C$n's@W punch!@n",
            room   = "@C$N@W manages to dodge @c$n's@W punch!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W moves quickly and blocks your punch!@n",
            target = "@WYou move quickly and block @C$n's@W punch!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W punch!@n",
        }, ctx)
        -- TODO: blocked punch deals 1/4 base_damage to attacker
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your punch with a punch of $S own!@n",
            target = "@WYou parry @C$n's@W punch with a punch of your own!@n",
            room   = "@C$N@W parries @c$n's@W punch with a punch of $S own!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
