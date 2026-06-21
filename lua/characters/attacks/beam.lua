local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local DIRS = { "north", "east", "south", "west", "up", "down" }

local HIT_MSGS = {
    body = {
        actor  = "@WYou send a brilliant @Cbeam@W crashing into @C$N's@W body!@n",
        target = "@c$n@W sends a brilliant @Cbeam@W crashing into your body!@n",
        room   = "@c$n@W sends a brilliant @Cbeam@W crashing into @C$N's@W body!@n",
    },
    head = {
        actor  = "@WYou send a brilliant @Cbeam@W crashing into @C$N's@W head!@n",
        target = "@c$n@W sends a brilliant @Cbeam@W crashing into your head!@n",
        room   = "@c$n@W sends a brilliant @Cbeam@W crashing into @C$N's@W head!@n",
    },
    arm = {
        actor  = "@WYou send a brilliant @Cbeam@W crashing into @C$N's@W arm!@n",
        target = "@c$n@W sends a brilliant @Cbeam@W crashing into your arm!@n",
        room   = "@c$n@W sends a brilliant @Cbeam@W crashing into @C$N's@W arm!@n",
    },
    leg = {
        actor  = "@WYou send a brilliant @Cbeam@W crashing into @C$N's@W leg!@n",
        target = "@c$n@W sends a brilliant @Cbeam@W crashing into your leg!@n",
        room   = "@c$n@W sends a brilliant @Cbeam@W crashing into @C$N's@W leg!@n",
    },
}

return {
    id    = "beam",
    family = "ki",
    name  = "Beam",
    skill = "beam",
    tier  = 2,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = true,
    can_parry = true,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 1.5,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        local ctx   = { actor = inst.attacker, target = inst.target }
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body, ctx)

        -- Knockback mastery: chance to send target flying in a random direction
        local skill  = inst.skill_level
        local chance = skill >= 100 and 20 or skill >= 75 and 10 or 5
        if not inst.spar and math.random(100) <= chance then
            local dir = DIRS[math.random(#DIRS)]
            if not inst.target:try_move(dir) then
                -- Slammed into a wall — deal bonus damage
                inst.target:deal_damage({ powerlevel = math.floor(inst.damage * 0.25) })
                act().message({
                    actor  = "@C$N@W is slammed into a wall by the force of your beam!@n",
                    target = "@WYou are slammed into a wall by @c$n@W's beam!@n",
                    room   = "@C$N@W is slammed into a wall by @c$n@W's beam!@n",
                }, ctx)
            else
                act().message({
                    actor  = "@C$N@W is knocked back by your beam!@n",
                    target = "@WYou are knocked back by @c$n@W's beam!@n",
                    room   = "@C$N@W is knocked back by @c$n@W's beam!@n",
                }, ctx)
            end
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou fire a beam at @C$N@W but it misses!@n",
            target = "@c$n@W fires a beam at you but it misses!@n",
            room   = "@c$n@W fires a beam at @C$N@W but it misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W sidesteps your beam!@n",
            target = "@WYou sidestep @c$n@W's beam!@n",
            room   = "@C$N@W sidesteps @c$n@W's beam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_parried = function(inst)
        act().message({
            actor  = "@C$N@W deflects your beam into the surroundings!@n",
            target = "@WYou deflect @c$n@W's beam into the surroundings!@n",
            room   = "@C$N@W deflects @c$n@W's beam into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your beam!@n",
            target = "@WYou block @c$n@W's beam!@n",
            room   = "@C$N@W blocks @c$n@W's beam!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
