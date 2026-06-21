local M  = {}
local db = require("dbat")

local ST = nil
local function sector_types()
    if not ST then ST = db.consts.sector_types end
    return ST
end

local function act() return db.lib.act end

local INSIDE_MSGS = {
    "@wThe blast hits the wall and the whole room shakes!@n",
    "@wYou blast the ceiling and dust rains down all around!@n",
    "@wThe ground cracks under the blast!@n",
    "@wThe walls crack and crumble from the impact!@n",
}
local WATER_MSGS = {
    "@wThe blast hits the water sending a huge wave crashing!@n",
    "@wThe water around the impact bubbles and steams!@n",
    "@wA geyser of water erupts from the blast!@n",
}
local OUTDOOR_MSGS = {
    "@wThe blast sends chunks of earth flying everywhere!@n",
    "@wThe ground is scorched black from the impact!@n",
    "@wA crater forms where the blast struck!@n",
    "@wTrees splinter and rocks shatter from the impact!@n",
}
local UNDERWATER_MSG = "@wThe blast hits the ocean floor, sending rocks everywhere!@n"
local SPACE_MSG      = "@wThe blast detonates harmlessly in the void!@n"
local LAVA_MSG       = "@wThe blast hits the lava and sends molten rock flying!@n"

local function pick(msgs)
    return msgs[math.random(1, #msgs)]
end

local function terrain_msg(room)
    local st  = sector_types()
    local sec = room:sector_type_get()
    local msg
    if sec == st.INSIDE or sec == st.CITY or sec == st.SHOP then
        msg = pick(INSIDE_MSGS)
    elseif sec == st.WATER_SWIM or sec == st.WATER_NOSWIM then
        msg = pick(WATER_MSGS)
    elseif sec == st.UNDERWATER then
        msg = UNDERWATER_MSG
    elseif sec == st.SPACE then
        msg = SPACE_MSG
    elseif sec == st.LAVA then
        msg = LAVA_MSG
    else
        msg = pick(OUTDOOR_MSGS)
    end
    room:send_text(msg .. "\r\n")
end

-- ki blast hits environment: sector message + room damage +5 (capped at 100) + zone broadcast
function M.ki_terrain_hit(ch)
    local room = ch:room_get()
    local dmg  = room:damage_get()
    if dmg <= 95 then room:damage_set(dmg + 5) end
    terrain_msg(room)
    local zone = db.zones.by_id(room:zone_vnum_get())
    if zone then zone:send_text("@wAn explosion shakes the entire area!@n\r\n") end
end

-- On ki parry: 90% chance to redirect to each bystander in order; fall back to terrain
function M.ki_parry_redirect(inst, dmg)
    local ch   = inst.attacker
    local room = ch:room_get()
    for person in room:people() do
        if not person:is_same(ch) and not person:is_same(inst.target) then
            if math.random(1, 100) >= 90 then
                person:meter_mod_int("lifeforce", -dmg)
                act().message({
                    actor  = "@WYour ki blast deflects and smashes into @C$N@W!@n",
                    target = "@WA ki blast deflects and smashes into you!@n",
                    room   = "@W@c$n@W's ki blast deflects and smashes into @C$N@W!@n",
                }, { actor = ch, target = person })
                return
            end
        end
    end
    M.ki_terrain_hit(ch)
end

-- Spawn vnum 80 homing object driven by ki_tsuihidan object script
function M.spawn_homing(inst)
    local ch    = inst.attacker
    local vict  = inst.target
    local proto = db.obj_protos.by_id(80)
    if not proto then return end
    local obj = proto:spawn()
    obj:to_room(ch:room_get())
    obj:script_add("ki_tsuihidan")
    local s = obj:script("ki_tsuihidan")
    s:number_set("damage",    math.floor(inst.base_damage * 0.2))
    s:number_set("target_id", vict:id_get())
    s:number_set("user_id",   ch:id_get())
    obj:event_schedule("script:ki_tsuihidan:tick", 1000, 1000)
end

-- Spawn vnum 82 Spirit Bomb driven by ki_genkidama object script
function M.spawn_huge_ki(inst, kidist)
    local ch    = inst.attacker
    local vict  = inst.target
    local proto = db.obj_protos.by_id(82)
    if not proto then return end
    local obj = proto:spawn()
    obj:to_room(vict:room_get())
    obj:script_add("ki_genkidama")
    local s = obj:script("ki_genkidama")
    s:number_set("damage",    inst.base_damage)
    s:number_set("target_id", vict:id_get())
    s:number_set("user_id",   ch:id_get())
    s:number_set("kidist",    kidist)
    obj:event_schedule("script:ki_genkidama:tick", 1000, 1000)
end

return M
