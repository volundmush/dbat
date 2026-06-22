local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")
local attack = require("lua.libs.attack")

local ENV_OUTDOOR_MSGS = {
    [1] = "Debris is thrown into the air and showers down thunderously!",
    [3] = "A cloud of dust envelopes the entire area!",
    [4] = "The surrounding area roars and shudders from the impact!",
    [5] = "The ground shatters apart from the stress of the impact!",
    [6] = "The explosion continues to burn spreading out and devouring some more of the ground before dying out.",
}

local ENV_UNDERWATER_MSGS = {
    [1] = "The water churns violently!",
    [2] = "Large bubbles rise from the movement!",
    [3] = "The water collapses in on the hole created!",
}

local ENV_WATER_MSGS = {
    [1] = "A huge column of water erupts from the impact!",
    [2] = "The impact briefly causes a swirling vortex of water!",
    [3] = "A huge depression forms in the water and erupts into a wave from the impact!",
}

local ENV_INSIDE_MSGS = {
    [1] = "Debris is thrown into the air and showers down thunderously!",
    [2] = "The structure of the surrounding room cracks and quakes from the blast!",
    [3] = "Parts of the ceiling collapse, crushing into the floor!",
    [4] = "The surrounding area roars and shudders from the impact!",
    [5] = "The ground shatters apart from the stress of the impact!",
    [6] = "The walls of the surrounding room crack in the same instant!",
}

local function apply_env_effects(ch, count)
    local room = ch:room_get()
    if not room then return end
    local RF = require("dbat").consts.room_flags
    if room:flagged(RF.SPACE) then return end

    room:send_all("The rest of the beams slam into the ground!\r\n")
    room:send_all("@wBright explosions erupt from the impacts!\r\n")

    local SC = require("dbat").consts.sector_types
    local sect = room:sector_type_get()

    if sect ~= SC.INSIDE then
        -- outdoor
        local roll = math.random(1, 8)
        if roll == 2 then
            if math.random(1, 4) == 4 and room:geffect_get() == 0 then
                room:geffect_set(5)
                room:send_all("Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!\r\n")
            end
        else
            local msg = ENV_OUTDOOR_MSGS[roll]
            if msg then room:send_all(msg .. "\r\n") end
        end
    end

    if sect == SC.UNDERWATER then
        local roll = math.random(1, 3)
        local msg = ENV_UNDERWATER_MSGS[roll]
        if msg then room:send_all(msg .. "\r\n") end
    elseif sect == SC.WATER_SWIM or sect == SC.WATER_NOSWIM then
        local roll = math.random(1, 3)
        local msg = ENV_WATER_MSGS[roll]
        if msg then room:send_all(msg .. "\r\n") end
    elseif sect == SC.INSIDE then
        local roll = math.random(1, 8)
        local msg = ENV_INSIDE_MSGS[roll]
        if msg then room:send_all(msg .. "\r\n") end
    end

    local remaining = 5 - count
    local cap = 100 - remaining * 5
    if room:damage_get() <= cap then
        room:damage_mod(remaining * 5)
    end
end

return {
    id     = "kakusanha",
    family = "ki",
    name   = "Kakusanha",
    skill  = "kakusanha",
    tier   = 3,
    is_aoe = true,
    elements = { ki = 1.0 },
    consumes_charge = true,
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = false,
    can_dodge = false,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 3.0,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_miss_aoe = function(inst)
        local ch = inst.attacker
        act().message({
            actor = "@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. The beam flies off harmlessly through the air as you lose control over it!@n",
            room  = "@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. The beam flies off harmlessly through the air as $e loses control over it!@n",
        }, { actor = ch })
    end,

    on_aoe = function(inst, count)
        local ch  = inst.attacker
        local dbat_m = require("dbat")
        local AFF = dbat_m.consts.aff_flags

        -- Launch message
        act().message({
            actor = "@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. Above your targets the beam breaks apart into five seperate pieces that follow their victims!@n",
            room  = "@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. Above you the beam breaks apart into five seperate pieces that follow their victims!@n",
        }, { actor = ch })

        -- Scale damage by target count
        local dmg = inst.base_damage
        if count >= 3 then
            dmg = math.floor(dmg * 0.40)
        elseif count > 1 then
            dmg = math.floor(dmg * 0.60)
        end

        local hits = 0
        local room = ch:room_get()
        if not room then return end

        for person in room:people() do
            if hits >= 5 then break end
            if person:id_get() == ch:id_get() then goto continue end
            if not person:is_npc() and person:aff_flagged(AFF.SPIRIT) then goto continue end
            if not person:is_npc() and (person:stat_get("level") or 0) <= 8 then goto continue end
            if person:mob_flagged(require("dbat").consts.mob_flags.NOKILL) then goto continue end

            -- Group ally filter
            if person:condition_has("group") and not person:is_npc() then
                local their_master = person:following_get()
                local my_master    = ch:following_get()
                if their_master and their_master:id_get() == ch:id_get() then goto continue end
                if my_master and my_master:id_get() == person:id_get() then goto continue end
            end

            if not ch:fighting_get() then ch:start_fighting(person) end
            if not person:fighting_get() then person:start_fighting(ch) end
            hits = hits + 1

            -- Zanzoken / icer dodge
            local icer_dodge = not person:is_npc() and person:race_get() == "icer" and math.random(1, 30) >= 28
            local has_zan = person:condition_has("zanzoken")
            local st_ok = person:meter_get("stamina") >= 1
            local not_sleeping = person:position_get() ~= dbat_m.consts.positions.SLEEPING
            if (icer_dodge or has_zan) and st_ok and not_sleeping then
                if has_zan then person:condition_remove("zanzoken") end
                person:meter_mod_int("stamina", -math.floor(person:meter_max("powerlevel") / 200))
                act().message({
                    actor  = "@C$N@c disappears, avoiding the beam chasing $M!@n",
                    target = "@cYou disappear, avoiding the beam chasing you!@n",
                    room   = "@C$N@c disappears, avoiding the beam chasing $M!@n",
                }, { actor = ch, target = person })
            elseif (person:skill_get("dodge") or 0) + math.random(-10, 5) > inst.skill_level then
                -- Dodge
                act().message({
                    actor  = "@c$N@W manages to escape the attack!@n",
                    target = "@WYou manage to escape the attack!@n",
                    room   = "@c$N@W manages to escape the attack!@n",
                }, { actor = ch, target = person })
            else
                -- Hit
                act().message({
                    actor  = "@R$N@r is slammed by one of the beams!@n",
                    target = "@RYou are slammed by one of the beams!@n",
                    room   = "@R$N@r is slammed by one of the beams!@n",
                }, { actor = ch, target = person })
                person:meter_mod_int("powerlevel", -math.max(0, dmg))
            end

            ::continue::
        end

        -- Environmental effects for unused beams
        if count < 5 then
            apply_env_effects(ch, count)
        end
    end,
}
