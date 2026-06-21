local function ke() return require("lua.libs.ki_effects") end
local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYour tsuihidan homes in on @C$N@W and blasts $M in the chest!@n",
        target = "@c$n@W's tsuihidan homes in on you and blasts you in the chest!@n",
        room   = "@c$n@W's tsuihidan homes in on @C$N@W and blasts $M in the chest!@n",
    },
    head = {
        actor  = "@WYour tsuihidan homes in on @C$N@W and blasts $M in the head!@n",
        target = "@c$n@W's tsuihidan homes in on you and blasts you in the head!@n",
        room   = "@c$n@W's tsuihidan homes in on @C$N@W and blasts $M in the head!@n",
    },
    arm = {
        actor  = "@WYour tsuihidan homes in on @C$N@W and blasts $M in the arm!@n",
        target = "@c$n@W's tsuihidan homes in on you and blasts you in the arm!@n",
        room   = "@c$n@W's tsuihidan homes in on @C$N@W and blasts $M in the arm!@n",
    },
    leg = {
        actor  = "@WYour tsuihidan homes in on @C$N@W and blasts $M in the leg!@n",
        target = "@c$n@W's tsuihidan homes in on you and blasts you in the leg!@n",
        room   = "@c$n@W's tsuihidan homes in on @C$N@W and blasts $M in the leg!@n",
    },
}

local function spawn_and_announce(inst)
    ke().spawn_homing(inst)
    act().message({
        actor  = "@WThe tsuihidan turns around at the last second and begins to pursue @C$N@W!@n",
        target = "@WThe tsuihidan turns around at the last second and begins to pursue you!@n",
        room   = "@WThe tsuihidan turns around at the last second and begins to pursue @C$N@W!@n",
    }, { actor = inst.attacker, target = inst.target })
end

return {
    id    = "tsuihidan",
    family = "ki",
    name  = "Tsuihidan",
    skill = "tsuihidan",
    tier  = 2,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = true,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 1.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a tsuihidan at @C$N@W but it misses!@n",
            target = "@c$n@W fires a tsuihidan at you but it misses!@n",
            room   = "@c$n@W fires a tsuihidan at @C$N@W but it misses!@n",
        }, { actor = inst.attacker, target = inst.target })
        spawn_and_announce(inst)
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W rolls out of the way of your tsuihidan!@n",
            target = "@WYou roll out of the way of @c$n@W's tsuihidan!@n",
            room   = "@C$N@W rolls out of the way of @c$n@W's tsuihidan!@n",
        }, { actor = inst.attacker, target = inst.target })
        spawn_and_announce(inst)
    end,

    on_parried = function(inst)
        act().message({
            actor  = "@C$N@W deflects your tsuihidan into the surroundings!@n",
            target = "@WYou deflect @c$n@W's tsuihidan into the surroundings!@n",
            room   = "@C$N@W deflects @c$n@W's tsuihidan into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
        ke().ki_terrain_hit(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx   = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Stamina drain mastery: chance based on skill level
        local skill = inst.skill_level
        local chance = skill >= 100 and 20 or skill >= 75 and 10 or 5
        if math.random(100) <= chance then
            inst.target:meter_mod_int("stamina", -inst.damage)
            act().message({
                actor  = "@C$N@C's stamina takes a serious hit from the tsuihidan!@n",
                target = "@WThe tsuihidan hits a vital spot and saps your stamina!@n",
                room   = "@c$n@C's tsuihidan saps @C$N@C's stamina!@n",
            }, ctx)
        end
    end,
}
