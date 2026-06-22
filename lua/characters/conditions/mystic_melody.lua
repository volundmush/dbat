local dbat = require("dbat")
local act  = dbat.lib.act

local SONG_SAFETY        = 1
local SONG_SHIELDING     = 2
local SONG_SHADOW_STITCH = 3

local TELEPORT_DESTS = {
    [4]  = { vnum = 300,   name = "Earth"   },
    [5]  = { vnum = 8003,  name = "Konack"  },
    [6]  = { vnum = 16087, name = "Arlia"   },
    [7]  = { vnum = 10182, name = "Namek"   },
    [8]  = { vnum = 2234,  name = "Vegeta"  },
    [9]  = { vnum = 4047,  name = "Frigid"  },
    [10] = { vnum = 12025, name = "Aether"  },
    [11] = { vnum = 14910, name = "Kanassa" },
}

local function find_instrument(ch)
    for obj in ch:inventory() do
        local v = obj:vnum_get()
        if v == 8802 or v == 8807 then return obj end
    end
end

local function get_master(c)
    local id = c:condition_number_get("following", "target_id")
    return id ~= 0 and dbat.characters.by_id(id) or nil
end

local function is_group_ally(ch, vict)
    if not ch:condition_has("group") or not vict:condition_has("group") then return false end
    local vm = get_master(vict)
    local cm = get_master(ch)
    if vm and vm:id_get() == ch:id_get() then return true end
    if cm and cm:id_get() == vict:id_get() then return true end
    if cm and vm and cm:id_get() == vm:id_get() then return true end
    return false
end

local function song_display_name(id)
    if id == SONG_SAFETY        then return "Song of Safety" end
    if id == SONG_SHIELDING     then return "Song of Shielding" end
    if id == SONG_SHADOW_STITCH then return "Shadow Stitch Minuet" end
    return "Teleportation Melody"
end

local function send_to_others(room, ch, vict, msg, ctx)
    for person in room:people() do
        if person:id_get() ~= ch:id_get() and person:id_get() ~= vict:id_get() then
            act.to_char(person, msg, ctx)
        end
    end
end

local function on_tick(ch, cond)
    local song = cond:number_get("song")
    if song <= 0 then
        ch:condition_remove("mystic_melody", "no_song")
        return
    end

    if not find_instrument(ch) then
        ch:send_line("You do not have an instrument.")
        ch:act_around("@c$n@C stops playing $s song.@n")
        ch:condition_remove("mystic_melody", "no_instrument")
        return
    end

    local skill    = ch:skill_get("mystic music")
    local diceroll = dbat.axion_dice(0)

    if skill > diceroll then
        ch:send_line("@CYou continue playing your song.@n")
        ch:act_around(string.format("@c$n@C continues playing @y'@Y%s@y'@C.@n", song_display_name(song)))
    else
        ch:send_line("@CYou mess up a portion of the song, but continue playing.@n")
        ch:act_around("@c$n@C messes up a portion of $s song, but continues to play.@n")
        return
    end

    local function ki_exhausted()
        if ch:meter_current("ki") > 0 then return false end
        ch:send_line("You no longer have the ki necessary to play your song.")
        ch:act_around("@c$n@C stops playing $s song.@n")
        ch:condition_remove("mystic_melody", "ki_exhausted")
        return true
    end

    local room = ch:room_get()
    if not room then return end

    local tdest = TELEPORT_DESTS[song]
    if tdest then
        local allies = {}
        for vict in room:people() do
            if vict:id_get() ~= ch:id_get() and is_group_ally(ch, vict) then
                allies[#allies + 1] = vict
            end
        end
        local dest = dbat.rooms.by_id(tdest.vnum)
        if dest then
            for _, vict in ipairs(allies) do
                local ctx = { actor = ch, target = vict }
                act.to_char(ch,
                    string.format("@CYour Teleportation Melody has transported @c$N@C to %s in a flash!@n", tdest.name),
                    ctx)
                act.to_char(vict,
                    string.format("@c$n's@C Teleportation Melody has transported you to %s in a flash!@n", tdest.name),
                    ctx)
                send_to_others(room, ch, vict,
                    "@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", ctx)
                vict:to_room(dest)
            end
            ch:send_line(string.format(
                "@CFinally as the last of your comrades has been teleported you teleport yourself to %s and stop your song.@n",
                tdest.name))
            ch:to_room(dest)
        end
        ch:condition_remove("mystic_melody", "completed")
        return
    end

    if song == SONG_SAFETY then
        for vict in room:people() do
            if vict:id_get() == ch:id_get() or is_group_ally(ch, vict) then
                local restore = math.floor(10 * skill + ch:meter_max("powerlevel") * 0.0004 * skill)
                local ctx = { actor = ch, target = vict }
                if vict:id_get() ~= ch:id_get() then
                    act.to_char(ch,
                        "@CYour skillfully playing of the Song of Safety has an effect on @c$N@C.@n", ctx)
                    act.to_char(vict,
                        "@c$n's@C soothing Song of Safety has recovered some of your powerlevel, stamina, and limb condition.", ctx)
                else
                    ch:send_line("@CYour skillfully playing of the Song of Safety has an effect on your own body@C.@n")
                end
                send_to_others(room, ch, vict, "@c$n@C continues playing $s ocarina!@n", ctx)
                vict:meter_mod_int("powerlevel", restore)
                vict:meter_mod_int("stamina", math.floor(restore * 0.5))
                local function heal_limb(idx, msg)
                    local cur = vict:limbcond_get(idx)
                    if cur < 100 then
                        local nv = math.min(100, cur + 1 + math.floor(skill * 0.1))
                        if nv >= 100 then vict:send_line(msg) end
                        vict:limbcond_set(idx, nv)
                    end
                end
                heal_limb(1, "Your right arm is no longer broken!@n")
                heal_limb(2, "Your left arm is no longer broken!@n")
                heal_limb(3, "Your right leg is no longer broken!@n")
                heal_limb(4, "Your left leg is no longer broken!@n")
                ch:improve_skill("mystic music", 2)
                ch:meter_mod_int("ki", -math.floor(ch:meter_max("ki") * 0.0003 + skill))
                if ki_exhausted() then return end
            end
        end

    elseif song == SONG_SHADOW_STITCH then
        for vict in room:people() do
            local cm = get_master(ch)
            local vm = get_master(vict)
            if cm and vm and is_group_ally(ch, vict) then goto shadow_continue end
            if skill > diceroll + 10 then
                local ctx = { actor = ch, target = vict }
                act.to_char(ch,
                    "@CYour forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n", ctx)
                act.to_char(vict,
                    "@c$n's@C forboding music has caused YOUR shadows to stitch into YOUR body, slow YOUR actions down!@n", ctx)
                send_to_others(room, ch, vict,
                    "@c$n's@C forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n", ctx)
                ch:meter_mod_int("ki", -math.floor(ch:meter_max("ki") * 0.001 + skill))
                vict:condition_apply_with_duration("shadow_stitch", "song", "shadow_stitch", 60)
            end
            if ki_exhausted() then return end
            ::shadow_continue::
        end

    elseif song == SONG_SHIELDING then
        for vict in room:people() do
            if vict:id_get() ~= ch:id_get() and not is_group_ally(ch, vict) then goto shield_continue end
            local ctx = { actor = ch, target = vict }
            if vict:id_get() ~= ch:id_get() then
                act.to_char(ch,
                    "@CYour triumphant and soaring music has powered a barrier around @c$N@C!@n", ctx)
                act.to_char(vict,
                    "@c$n's@C triumphant and soaring music has powered a barrier around you!@n", ctx)
            else
                ch:send_line("@CYour triumphant and soaring music has powered a barrier around yourself@C!@n")
            end
            send_to_others(room, ch, vict,
                "@c$n's@C triumphant and soaring music has powered a barrier around @c$N@C!@n", ctx)
            local max_barrier = math.floor(vict:meter_max("powerlevel") * 0.75)
            local add = math.floor(ch:meter_max("powerlevel") * 0.005 * (skill * 0.25) + skill)
            vict:barrier_set(math.min(max_barrier, vict:barrier_get() + add))
            if not vict:condition_has("barrier") then
                vict:condition_apply("barrier", "mystic_melody", "shield")
            end
            ch:meter_mod_int("ki", -math.floor(ch:meter_max("ki") * 0.02 + skill))
            if ki_exhausted() then return end
            ::shield_continue::
        end
    end
end

return {
    id         = "mystic_melody",
    name       = "Mystic Melody",
    tags       = { "song" },
    persistent = false,

    on_apply = function(ch, cond)
        cond:schedule_event("tick", 15000, 15000)
    end,

    on_game_activate = function(ch, cond)
        if not cond:event_pending("tick") then
            cond:schedule_event("tick", 15000, 15000)
        end
    end,

    on_remove = function(ch, cond, reason)
        cond:cancel_event("tick")
    end,

    on_event = function(ch, cond, event)
        if event == "tick" then on_tick(ch, cond) end
    end,

    status_line = function(ch, cond)
        local id = cond:number_get("song")
        return id > 0 and ("You are playing the " .. song_display_name(id) .. ".") or nil
    end,
}
