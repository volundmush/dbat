local function act() return require("dbat").lib.act end

return {
    id   = "strike",
    name = "Fang Strike",
    skill = "punch",
    family = "melee",
    tier  = 1,
    elements = { pierce = 1.0 },
    damages_limbs        = true,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block  = true,
    can_parry  = true,
    can_dodge  = true,
    base_accuracy = 1.05,
    base_power    = 1.5,
    spar_safe = true,

    on_check = function(inst)
        if not inst.attacker:is_npc() then
            return false, "You cannot do that."
        end
        return true
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        -- drain victim stamina (25% of damage dealt)
        if inst.damage > 0 then
            inst.target:meter_mod_int("stamina", -math.floor(inst.damage * 0.25))
        end
        if loc == "head" then
            a.message({
                actor  = "@CYou@W launch your body at @C$N@W and sink your fang strike into $S face!@n",
                target = "@C$n@W launches $s body at you and sinks $s fang strike into YOUR face!@n",
                room   = "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@CYou@W launch your body at @C$N@W and sink your fang strike into $S arm!@n",
                target = "@C$n@W launches $s body at you and sinks $s fang strike into YOUR arm!@n",
                room   = "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@CYou@W launch your body at @C$N@W and sink your fang strike into $S leg!@n",
                target = "@C$n@W launches $s body at you and sinks $s fang strike into YOUR leg!@n",
                room   = "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@CYou@W launch your body at @C$N@W and sink your fang strike into $S body!@n",
                target = "@C$n@W launches $s body at you and sinks $s fang strike into YOUR body!@n",
                room   = "@c$n@W launches $s body at @C$N@W and sinks $s fang strike into $S body!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@CYou@W move to fang strike @C$N@W, but miss!@n",
            target = "@C$n@W moves to fang strike you, but misses!@n",
            room   = "@c$n@W moves to fang strike @C$N@W, but somehow misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W manages to dodge your fang strike!@n",
            target = "@WYou dodge @C$n's@W fang strike!@n",
            room   = "@C$N@W manages to dodge @c$n's@W fang strike!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W moves quickly and blocks your fang strike!@n",
            target = "@WYou move quickly and block @C$n's@W fang strike!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W fang strike!@n",
        }, ctx)
    end,

    on_parried = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W parries your fang strike with a punch of $S own!@n",
            target = "@WYou parry @C$n's@W fang strike with a punch of your own!@n",
            room   = "@C$N@W parries @c$n's@W fang strike with a punch of $S own!@n",
        }, ctx)
        local counter = math.floor(inst.base_damage / 4)
        if counter > 0 then
            inst.attacker:damage({ powerlevel = counter, spar = inst.spar })
        end
    end,
}
