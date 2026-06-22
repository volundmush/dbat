local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

return {
    id     = "hellspear",
    family = "ki",
    name   = "Hell Spear Blast",
    skill  = "hell spear blast",
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

    on_aoe_miss = function(inst)
        act().message({
            actor = "@WYou fire off a @BHell Spear Blast@W but it goes wide!@n",
            room  = "@c$n@W fires off a @BHell Spear Blast@W but it goes wide!@n",
        }, { actor = inst.attacker })
    end,

    on_aoe = function(inst, count)
        local ch   = inst.attacker
        local AFF  = require("dbat").consts.aff_flags
        local room = ch:room_get()

        act().message({
            actor = "@WYou charge up a volley of @Bdeep blue spears@W of ki and launch them in all directions!@n",
            room  = "@c$n@W charges up a volley of @Bdeep blue spears@W of ki and launches them in all directions!@n",
        }, { actor = ch })

        for person in room:people() do
            if not require("lua.libs.attack")._aoe_valid_target(ch, person) then goto continue end

            if not ch:fighting_get() then ch:start_fighting(person) end
            if not person:fighting_get() then person:start_fighting(ch) end

            local dodge_roll = person:skill_get("dodge") + math.random(-10, 5)
            if (not person:is_npc() and person:race_get() == "icer" and math.random(1, 30) >= 28)
               or person:condition_has("zanzoken") then
                person:condition_remove("zanzoken")
                person:meter_mod_int("stamina", -math.floor(person:meter_max("powerlevel") / 200))
                act().message({
                    actor  = "@C$N@W disappears, avoiding your Hell Spear Blast!@n",
                    target = "@WYou zanzoken out of the way of @c$n@W's Hell Spear Blast!@n",
                    room   = "@C$N@W disappears, avoiding @c$n@W's Hell Spear Blast!@n",
                }, { actor = ch, target = person })
            elseif dodge_roll > inst.skill_level then
                act().message({
                    actor  = "@C$N@W dodges your Hell Spear Blast!@n",
                    target = "@WYou dodge @c$n@W's Hell Spear Blast!@n",
                    room   = "@C$N@W dodges @c$n@W's Hell Spear Blast!@n",
                }, { actor = ch, target = person })
            else
                act().message({
                    actor  = "@BHell spears@W of energy skewer @C$N@W from all sides!@n",
                    target = "@B@c$n@W's@W hell spears of energy skewer you from all sides!@n",
                    room   = "@BHell spears@W of energy skewer @C$N@W from all sides!@n",
                }, { actor = ch, target = person })
                local dmg = inst.spar and 0 or math.max(0, inst.damage)
                person:meter_mod_int("powerlevel", -dmg)
                -- 25% knockdown
                if math.random(1, 4) == 4
                   and not person:condition_has("flying")
                   and person:position_get() == require("dbat").consts.positions.STANDING then
                    person:condition_apply("knocked", "combat", "stunned")
                end
            end
            ::continue::
        end
    end,
}
