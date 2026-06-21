local function act() return require("dbat").lib.act end

local function can_bite(ch)
    if ch:is_npc() then return true end
    if ch:race_get() == "mutant" and (ch:genome_get(0) == 7 or ch:genome_get(1) == 7) then
        return true
    end
    return false
end

return {
    id   = "bite",
    name = "Bite",
    skill = "punch",
    family = "melee",
    tier  = 1,
    elements = { blunt = 1.0 },
    damages_limbs = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block  = true,
    can_parry  = true,
    can_dodge  = true,
    consumes_charge = false,
    base_accuracy = 1.0,
    base_power    = 1.25,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = 20 + math.floor(inst.attacker:meter_max("powerlevel") / 500)
    end,

    on_check = function(inst)
        if not can_bite(inst.attacker) then
            return false, "You don't want to put that in your mouth, you don't know where it has been!"
        end
        return true
    end,

    on_check_combo = function(ch)
        return can_bite(ch)
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou bite @C$N's@W face!@n",
                target = "@C$n@W bites your face!@n",
                room   = "@c$n@W bites into $N's face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou bite @C$N's@W arm!@n",
                target = "@C$n@W bites you on the arm!@n",
                room   = "@c$n@W bites @C$N@W on the arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou bite @C$N's@W leg!@n",
                target = "@C$n@W bites into your leg!@n",
                room   = "@c$n@W bites into @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou bite @C$N's@W body!@n",
                target = "@C$n@W bites you on the body, sending blood flying!@n",
                room   = "@C$n@W bites @C$N@W on the body, sending blood flying!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou move to bite @C$N@W, but miss!@n",
            target = "@C$n@W moves to bite you, but misses!@n",
            room   = "@c$n@W moves to bite @C$N@W, but somehow misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W manages to dodge your bite!@n",
            target = "@WYou dodge @C$n's@W bite!@n",
            room   = "@C$N@W manages to dodge @c$n's@W bite!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W moves quickly and blocks your bite!@n",
            target = "@WYou move quickly and block @C$n's@W bite!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W bite!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your bite with a punch of their own!@n",
            target = "@WYou parry @C$n's@W bite with a punch of your own!@n",
            room   = "@C$N@W parries @c$n's@W bite with a punch of $S own!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
