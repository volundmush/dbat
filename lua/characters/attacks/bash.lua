local function act() return require("dbat").lib.act end

return {
    id   = "bash",
    name = "Bash",
    skill = "bash",
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
    base_accuracy = 0.85,
    base_power    = 1.2,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 150)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then return false end
        return ch:skill_get("bash")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou bash @C$N@W over the head with a massive blow!@n",
                target = "@C$n@W bashes you over the head with a massive blow!@n",
                room   = "@c$n@W bashes @C$N@W over the head!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou smash @C$N's@W arm with both fists!@n",
                target = "@C$n@W smashes your arm with both fists!@n",
                room   = "@c$n@W smashes @C$N's@W arm with both fists!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou crash your fists into @C$N's@W leg!@n",
                target = "@C$n@W crashes $s fists into your leg!@n",
                room   = "@c$n@W crashes $s fists into @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou crash into @C$N@W with a powerful double-fisted bash!@n",
                target = "@C$n@W crashes into you with a powerful double-fisted bash!@n",
                room   = "@c$n@W crashes into @C$N@W with a double-fisted bash!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou swing both fists at @C$N@W but miss!@n",
            target = "@C$n@W swings both fists at you but misses!@n",
            room   = "@c$n@W swings at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W leaps back from your bash!@n",
            target = "@WYou leap back from @C$n's@W bash!@n",
            room   = "@C$N@W leaps back from @c$n's@W bash!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W sets $S feet and absorbs your bash!@n",
            target = "@WYou set your feet and absorb @C$n's@W bash!@n",
            room   = "@C$N@W absorbs @c$n's@W bash!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your bash and staggers you!@n",
            target = "@WYou parry @C$n's@W bash and stagger $M!@n",
            room   = "@C$N@W parries @c$n's@W bash!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
