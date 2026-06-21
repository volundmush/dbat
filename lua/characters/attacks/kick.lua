local function act() return require("dbat").lib.act end

return {
    id   = "kick",
    name = "Kick",
    skill = "kick",
    family = "melee",
    tier  = 1,
    elements = { blunt = 1.0 },
    limbs_required = { "legs" },
    damages_limbs = true,
    can_combo = true,
    in_combo  = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block = true,
    can_parry = true,
    can_dodge = true,
    consumes_charge = false,
    min_charge = 0,
    max_charge = 0,
    base_accuracy = 1.0,
    base_power    = 1.1,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 400)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then return false end
        return ch:skill_known("kick")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou slam your foot into @C$N's@W face!@n",
                target = "@C$n@W slams $s foot into your face!@n",
                room   = "@c$n@W slams $s foot into @C$N's@W face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou land your foot against @C$N's@W arm!@n",
                target = "@C$n@W lands $s foot against your arm!@n",
                room   = "@C$n@W lands $s foot against @C$N's@W arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou land your foot against @C$N's@W leg!@n",
                target = "@C$n@W lands $s foot against your leg!@n",
                room   = "@C$n@W lands $s foot against @C$N's@W leg!@n",
            }, ctx)
        else
            if math.random(1, 2) == 1 then
                a.message({
                    actor  = "@WYou slam your foot into @C$N's@W body!@n",
                    target = "@C$n@W slams $s foot into your body!@n",
                    room   = "@c$n@W slams $s foot into @C$N's@W body!@n",
                }, ctx)
            else
                a.message({
                    actor  = "@WYou land your foot against @C$N's@W gut!@n",
                    target = "@C$n@W lands $s foot against your gut!@n",
                    room   = "@C$n@W lands $s foot against @C$N's@W gut!@n",
                }, ctx)
            end
        end
        -- TODO: dam_eq_loc
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou can't believe it, your kick misses!@n",
            target = "@C$n@W throws a kick at you, but thankfully misses!@n",
            room   = "@c$n@W throws a kick at @C$N@W, but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W dodges your kick!@n",
            target = "@WYou dodge @C$n's@W kick!@n",
            room   = "@C$N@W dodges @c$n's@W kick!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W blocks your kick!@n",
            target = "@WYou block @C$n's@W kick!@n",
            room   = "@C$N@W blocks @c$n's@W kick!@n",
        }, ctx)
        -- TODO: blocked kick deals 1/4 base_damage to attacker
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your kick with a kick of $S own!@n",
            target = "@WYou parry @C$n's@W kick with one of your own!@n",
            room   = "@C$N@W parries @c$n's@W kick with a kick of $S own!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
