local function act() return require("dbat").lib.act end

return {
    id   = "tailwhip",
    name = "Tailwhip",
    skill = "tailwhip",
    family = "melee",
    tier  = 2,
    elements = { blunt = 1.0 },
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
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 120)
    end,

    on_check = function(inst)
        local db  = require("dbat")
        local PLR = db.consts.player_flags
        if not inst.attacker:is_npc() and not inst.attacker:player_flagged(PLR.TAIL) then
            return false, "You don't have a tail to whip with!"
        end
        return true
    end,

    on_check_combo = function(ch)
        local db  = require("dbat")
        local PLR = db.consts.player_flags
        if not ch:is_npc() and not ch:player_flagged(PLR.TAIL) then return false end
        return ch:skill_get("tailwhip")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou whip your tail across @C$N's@W face!@n",
                target = "@C$n@W whips $s tail across your face!@n",
                room   = "@c$n@W whips $s tail across @C$N's@W face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou lash your tail at @C$N's@W arm!@n",
                target = "@C$n@W lashes $s tail at your arm!@n",
                room   = "@c$n@W lashes $s tail at @C$N's@W arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou sweep @C$N's@W legs with your tail!@n",
                target = "@C$n@W sweeps your legs with $s tail!@n",
                room   = "@c$n@W sweeps @C$N's@W legs with $s tail!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou lash your tail hard into @C$N's@W body!@n",
                target = "@C$n@W lashes $s tail hard into your body!@n",
                room   = "@c$n@W lashes $s tail hard into @C$N's@W body!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou whip your tail at @C$N@W but it just cuts air!@n",
            target = "@C$n@W whips $s tail at you but it cuts air!@n",
            room   = "@c$n@W whips $s tail at @C$N@W but misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W jumps over your tail sweep!@n",
            target = "@WYou jump over @C$n's@W tail sweep!@n",
            room   = "@C$N@W jumps over @c$n's@W tail sweep!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W catches your tail!@n",
            target = "@WYou catch @C$n's@W tail!@n",
            room   = "@C$N@W catches @c$n's@W tail!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W grabs your tail and yanks you off balance!@n",
            target = "@WYou grab @C$n's@W tail and yank $M off balance!@n",
            room   = "@C$N@W grabs @c$n's@W tail!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
