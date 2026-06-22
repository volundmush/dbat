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
    id     = "bakuhatsuha",
    family = "ki",
    name   = "Bakuhatsuha",
    skill  = "bakuhatsuha",
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

    on_calculate_damage = function(inst)
        local ch  = inst.attacker
        local sk  = inst.skill_level
        local atk = require("lua.libs.attack")
        local base = atk._base_damage(ch, inst.def, inst)
        local bonus_pct = sk >= 100 and 0.08 or sk >= 60 and 0.04 or sk >= 40 and 0.02 or 0
        return base + math.floor(ch:meter_max("powerlevel") * bonus_pct)
    end,

    on_aoe_miss = function(inst)
        act().message({
            actor = "@WYou release a Bakuhatsuha but hit nothing of consequence!@n",
            room  = "@c$n@W releases a Bakuhatsuha but hits nothing!@n",
        }, { actor = inst.attacker })
    end,

    on_aoe = function(inst, count)
        local ch   = inst.attacker
        local room = ch:room_get()
        local dbat = require("dbat")

        -- Damage scale by number of valid targets
        local scale = count == 1 and 1.0 or count == 2 and 0.75 or count == 3 and 0.50 or 0.25

        act().message({
            actor = "@WYou extend your arms outward, @Renergy@W building around you before erupting into a massive @RBakuhatsuha@W!@n",
            room  = "@c$n@W extends $s arms outward as @Renergy@W erupts into a massive @RBakuhatsuha@W!@n",
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
                    actor  = "@C$N@W disappears, avoiding your Bakuhatsuha!@n",
                    target = "@WYou zanzoken out of the way of @c$n@W's Bakuhatsuha!@n",
                    room   = "@C$N@W disappears, avoiding @c$n@W's Bakuhatsuha!@n",
                }, { actor = ch, target = person })
            elseif dodge_roll > inst.skill_level then
                act().message({
                    actor  = "@C$N@W dodges your Bakuhatsuha!@n",
                    target = "@WYou dodge @c$n@W's Bakuhatsuha!@n",
                    room   = "@C$N@W dodges @c$n@W's Bakuhatsuha!@n",
                }, { actor = ch, target = person })
            else
                act().message({
                    actor  = "@REnergy erupts around @C$N@W from your Bakuhatsuha!@n",
                    target = "@REnergy erupts around you from @c$n@W's Bakuhatsuha!@n",
                    room   = "@REnergy erupts around @C$N@W from @c$n@W's Bakuhatsuha!@n",
                }, { actor = ch, target = person })
                local dmg = inst.spar and 0 or math.max(0, math.floor(inst.damage * scale))
                person:meter_mod_int("powerlevel", -dmg)
                -- always knockdown
                if not person:condition_has("flying")
                   and person:position_get() == dbat.consts.positions.STANDING then
                    person:condition_apply("knocked", "combat", "stunned")
                end
            end
            ::continue::
        end
    end,
}
