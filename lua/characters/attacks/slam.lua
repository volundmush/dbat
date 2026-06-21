local function act() return require("dbat").lib.act end

return {
    id   = "slam",
    name = "Slam",
    skill = "slam",
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
    base_accuracy = 0.9,
    base_power    = 1.5,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 100)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then return false end
        return ch:skill_known("slam")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou grab @C$N@W and slam $M headfirst into the ground!@n",
                target = "@C$n@W grabs you and slams you headfirst into the ground!@n",
                room   = "@c$n@W grabs @C$N@W and slams $M headfirst into the ground!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou grab @C$N@W by the arm and hurl $M to the ground!@n",
                target = "@C$n@W grabs you by the arm and hurls you to the ground!@n",
                room   = "@c$n@W grabs @C$N@W by the arm and hurls $M to the ground!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou sweep @C$N's@W legs and slam $M down!@n",
                target = "@C$n@W sweeps your legs and slams you down!@n",
                room   = "@c$n@W sweeps @C$N's@W legs and slams $M down!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou grab @C$N@W and body slam $M into the ground!@n",
                target = "@C$n@W grabs you and body slams you into the ground!@n",
                room   = "@c$n@W grabs @C$N@W and body slams $M into the ground!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou lunge to grab @C$N@W but $E slips away!@n",
            target = "@C$n@W lunges to grab you but you slip away!@n",
            room   = "@c$n@W lunges at @C$N@W but $E slips away!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W twists free before you can slam $M!@n",
            target = "@WYou twist free before @C$n@W can slam you!@n",
            room   = "@C$N@W twists free from @c$n's@W slam attempt!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W braces $Sself and absorbs the slam!@n",
            target = "@WYou brace yourself and absorb @C$n's@W slam!@n",
            room   = "@C$N@W braces and absorbs @c$n's@W slam!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W reverses your slam and throws YOU to the ground!@n",
            target = "@WYou reverse @C$n's@W slam and throw $M to the ground!@n",
            room   = "@C$N@W reverses @c$n's@W slam!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 3)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
