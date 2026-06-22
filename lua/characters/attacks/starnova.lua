local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local function is_group_ally(ch, person)
    if not person:condition_has("group") then return false end
    local person_leader = person:following_get()
    local ch_leader     = ch:following_get()
    if person_leader and person_leader:id_get() == ch:id_get() then return true end
    if ch_leader and ch_leader:id_get() == person:id_get() then return true end
    if person_leader and ch_leader and person_leader:id_get() == ch_leader:id_get() then return true end
    return false
end

return {
    id     = "starnova",
    family = "ki",
    name   = "Starnova",
    skill  = "starnova",
    tier   = 3,
    is_aoe = true,
    elements = { ki = 1.0 },
    consumes_charge = true,
    can_block = false,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 2.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_calculate_cost = function(inst)
        -- perf type 1: +5% ki cost
        if inst.attacker:perf_get() == 1 then
            local max_ki = inst.attacker:meter_max("ki")
            inst.cost.ki = (inst.cost.ki or 0) + math.floor(max_ki * 0.05)
        end
    end,

    on_modify_accuracy = function(inst)
        -- perf type 2: +5 effective skill for accuracy and damage
        if inst.attacker:perf_get() == 2 then
            inst.skill_level = inst.skill_level + 5
        end
    end,

    on_calculate_damage = function(inst)
        local ch    = inst.attacker
        local atk   = require("lua.libs.attack")
        local base  = atk._base_damage(ch, inst.def, inst)
        local hours = require("dbat").time().hours
        local mult  = hours <= 15 and 1.25 or (hours <= 22 and 1.4 or 1.0)
        return math.floor(base * mult)
    end,

    on_aoe_miss = function(inst)
        act().message({
            actor = "@WYou release a Starnova but it dissipates without finding a target!@n",
            room  = "@c$n@W releases a Starnova but it misses!@n",
        }, { actor = inst.attacker })
    end,

    on_aoe = function(inst, count)
        local ch   = inst.attacker
        local room = ch:room_get()
        local dbat = require("dbat")

        act().message({
            actor = "@W[@YStarnova@W]@W You concentrate your energy into a massive @Ystarburst@W that blasts outward!@n",
            room  = "@W[@YStarnova@W] @c$n@W concentrates $s energy into a massive @Ystarburst@W that blasts outward!@n",
        }, { actor = ch })

        for person in room:people() do
            if not require("lua.libs.attack")._aoe_valid_target(ch, person) then goto continue end
            if is_group_ally(ch, person) then goto continue end

            if not ch:fighting_get() then ch:start_fighting(person) end
            if not person:fighting_get() then person:start_fighting(ch) end

            local dodge_roll = person:skill_get("dodge") + math.random(-15, 5)
            if (not person:is_npc() and person:race_get() == "icer" and math.random(1, 30) >= 28)
               or person:condition_has("zanzoken") then
                person:condition_remove("zanzoken")
                person:meter_mod_int("stamina", -math.floor(person:meter_max("powerlevel") / 200))
                act().message({
                    actor  = "@C$N@W zanzokens out of your Starnova!@n",
                    target = "@WYou zanzoken away from @c$n@W's Starnova!@n",
                    room   = "@C$N@W zanzokens out of @c$n@W's Starnova!@n",
                }, { actor = ch, target = person })
            elseif dodge_roll > inst.skill_level then
                act().message({
                    actor  = "@C$N@W dodges your Starnova!@n",
                    target = "@WYou dodge @c$n@W's Starnova!@n",
                    room   = "@C$N@W dodges @c$n@W's Starnova!@n",
                }, { actor = ch, target = person })
            else
                act().message({
                    actor  = "@YStarlight@W erupts over @C$N@W from your Starnova!@n",
                    target = "@YStarlight@W erupts over you from @c$n@W's Starnova!@n",
                    room   = "@YStarlight@W erupts over @C$N@W from @c$n@W's Starnova!@n",
                }, { actor = ch, target = person })
                local dmg = inst.spar and 0 or math.max(0, inst.damage)
                person:meter_mod_int("powerlevel", -dmg)
                if not person:condition_has("flying")
                   and person:position_get() == dbat.consts.positions.STANDING then
                    person:condition_apply("knocked", "combat", "stunned")
                end
            end
            ::continue::
        end
    end,

    on_after_cost = function(inst)
        -- perf type 3: refund 5% ki
        if inst.attacker:perf_get() == 3 and inst.hit then
            local max_ki = inst.attacker:meter_max("ki")
            inst.attacker:meter_mod_int("ki", math.floor(max_ki * 0.05))
        end
    end,
}
