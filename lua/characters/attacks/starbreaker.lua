local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou combine dark @rred@W energy and @mpurple@W ki electricity, then toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S chest!@n",
        target = "@c$n@W combines dark @rred@W energy and @mpurple@W ki electricity, then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs YOUR chest!@n",
        room   = "@c$n@W combines dark @rred@W energy and @mpurple@W ki electricity, then tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S chest!@n",
    },
    head = {
        actor  = "@WYou toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S head!@n",
        target = "@c$n@W tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs YOUR head!@n",
        room   = "@c$n@W tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S head!@n",
    },
    arm = {
        actor  = "@WYou toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S arm!@n",
        target = "@c$n@W tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs YOUR arm!@n",
        room   = "@c$n@W tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S arm!@n",
    },
    leg = {
        actor  = "@WYou toss your @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S leg!@n",
        target = "@c$n@W tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at YOU! It engulfs YOUR leg!@n",
        room   = "@c$n@W tosses a @YS@yta@Yr @rB@Rr@De@ra@Rk@De@rr@W at @c$N@W! It engulfs $S leg!@n",
    },
}

local function exp_theft(ch, vict)
    local cl = ch:stat_get("level") or 1
    local vl = vict:stat_get("level") or 1
    local vexp = vict:stat_get("experience") or 0
    if vexp <= 0 then return end
    local diff  = cl - vl
    local frac  = diff > 30 and 1
        or diff > 20 and 1000
        or diff > 10 and 100
        or diff >= 0  and 50
        or diff >= -10 and 500
        or diff >= -20 and 1000
        or 2000
    local theft = frac == 1 and 1 or math.floor(vexp / frac)
    if theft <= 0 then return end
    vict:stat_mod("experience", -theft)
end

return {
    id     = "starbreaker",
    family = "ki",
    name   = "Star Breaker",
    skill  = "star breaker",
    tier   = 2,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = true,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 1.8,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ch   = inst.attacker
        local vict = inst.target
        exp_theft(ch, vict)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = ch, target = vict })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your Star Breaker misses, flying through the air harmlessly!@n",
            target = "@c$n@W fires a Star Breaker at you, but misses!@n",
            room   = "@c$n@W fires a Star Breaker at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Star Breaker, letting it slam into the surroundings!@n",
            target = "@WYou dodge @C$n's@W Star Breaker, letting it slam into the surroundings!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Star Breaker, letting it slam into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
        inst.attacker:room_get():send_line("@wA bright explosion erupts from the impact!@n")
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W moves quickly and blocks your Star Breaker!@n",
            target = "@WYou move quickly and block @C$n's@W Star Breaker!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W Star Breaker!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
