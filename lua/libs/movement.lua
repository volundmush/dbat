local dbat = require("dbat")
local act  = require("lua.libs.act")

local AFF  = dbat.consts.aff_flags
local ADM  = dbat.consts.admin_flags
local RF   = dbat.consts.room_flags
local CF   = dbat.consts.container_flags
local ITEM = dbat.consts.item_types
local EX   = dbat.consts.exit_flags
local POS  = dbat.consts.positions
local ST   = dbat.consts.sector_types
local ZF   = dbat.consts.zone_flags
local MF   = dbat.consts.mob_flags

local ADMLVL_GRGOD  = dbat.consts.adm_levels.GRGOD
local ADMLVL_IMMORT = dbat.consts.adm_levels.IMMORT
local ADMLVL_GOD    = dbat.consts.adm_levels.GOD

local DIR_NAMES = {
    "north", "east", "south", "west", "up", "down",
    "northwest", "northeast", "southeast", "southwest", "inside", "outside"
}

-- Reverse direction mapping (0-based indices)
local REV_DIR = {[0]=2,[1]=3,[2]=0,[3]=1,[4]=5,[5]=4,[6]=8,[7]=9,[8]=6,[9]=7,[10]=11,[11]=10}

local function handle_fall(ch)
    while true do
        local room = ch:room_get()
        if not room then break end
        if room:sector_type_get() ~= ST.FLYING then break end
        local ex = room:exit_get(5)
        if not ex then break end
        local dest = ex:destination()
        if not dest then break end

        ch:to_room(dest)
        local carrying = ch:carrying_char_get()
        if carrying then carrying:to_room(dest) end

        local new_room = ch:room_get()
        if not new_room then break end
        if not new_room:exit_get(5) or new_room:sector_type_get() ~= ST.FLYING then
            act.around(ch, "@r$n slams into the ground!@n", {actor=ch})
            act.to_char(ch, "@rYou slam into the ground!@n", {actor=ch})
            ch:meter_mod_int("powerlevel", -math.floor(ch:meter_max("powerlevel") / 20))
            ch:look_at_room()
            break
        else
            act.around(ch, "@r$n pummets down toward the ground below!@n", {actor=ch})
        end
    end

    local room = ch:room_get()
    if not room then return end
    if room:sector_type_get() == ST.WATER_NOSWIM
       and not ch:carried_by_char_get()
       and ch:race_get() ~= "kanassan" then
        local weight = ch:carry_weight_get()
        if ch:meter_current("stamina") >= weight then
            act.to_char(ch,  "@bYou swim in place.@n", {actor=ch})
            act.around(ch, "@C$n@b swims in place.@n", {actor=ch})
            ch:meter_mod_int("stamina", -weight)
            act.to_char(ch,  "@RYou are drowning!@n", {actor=ch})
            act.around(ch, "@C$n@b gulps water as $e struggles to stay above the water line.@n", {actor=ch})
            local max_pl = ch:meter_max("powerlevel")
            if ch:meter_current("powerlevel") - math.floor(max_pl / 3) <= 0 then
                act.to_char(ch,  "@rYou drown!@n", {actor=ch})
                act.around(ch, "@R$n@r drowns!@n", {actor=ch})
                ch:die(nil)
            else
                ch:meter_mod_int("powerlevel", -math.floor(max_pl * 0.33))
            end
        end
    end
end

-- Returns movement stamina cost based on current room gravity.
local function enter_leave_cost(ch, room)
    local cost = 1
    local g = room:gravity_get()
    if g > 10 or (g == 10 and ch:sensei_get() ~= "bardock" and not ch:is_npc()) then
        cost = (cost + g) * g
    end
    if (ch:stat_get("level") or 0) <= 1 then cost = 0 end
    return cost
end

-- Count non-NPC characters in a room.
local function count_pc_in_room(room)
    local n = 0
    for person in room:people() do
        if not person:is_npc() then n = n + 1 end
    end
    return n
end

-- Replaces IS_HUMANOID macro: all races except serpent and animal.
local function is_humanoid(ch)
    local r = ch:race_get()
    return r ~= "serpent" and r ~= "animal"
end

-- Approximation of roll_skill for sneak/hide movement cost multiplier.
local function roll_skill_approx(ch, skill_name)
    return (ch:skill_get(skill_name) or 0) + math.random(1, 20)
end

-- Replaces static has_flight(ch) in act.movement.cpp.
local function has_flight(ch)
    if ch:admin_flagged(ADM.WALKANYWHERE) then return true end
    if ch:condition_has("flying") then
        if ch:is_npc() then return true end
        if ch:race_get() == "android" then return true end
        local level = math.max(1, ch:stat_get("level") or 1)
        local threshold = level + math.floor(ch:meter_max("ki") / (level * 30))
        if ch:meter_current("ki") >= threshold then
            return true
        end
        -- Out of KI: crash to ground
        ch:send_line("@WYou crash to the ground, too tired to fly anymore!@n")
        ch:act_around("@W$n@W crashes to the ground!@n")
        ch:condition_remove("flying", "stop_flying")
        handle_fall(ch)
        return false
    end
    -- TODO: non-wearable flying items in inventory (needs obj AFF flag scan)
    return false
end

-- Replaces static check_swim(ch) in act.movement.cpp.
-- Always deducts the cost; returns true if the character had enough.
local function check_swim(ch)
    local room = ch:room_get()
    if room and room:flagged(RF.SPACE) then
        local space_cost = math.floor(ch:meter_max("ki") / 1000) + math.floor(ch:carry_weight_get() / 2)
        local can = ch:meter_current("ki") >= space_cost
        ch:meter_mod_int("ki", -space_cost)
        if not can then
            ch:send_line("You do not have enough ki to fly through space. You are drifting helplessly.")
        end
        return can
    else
        local swim_cost = ch:carry_weight_get() - 1
        local can = ch:meter_current("stamina") >= swim_cost
        ch:meter_mod_int("stamina", -swim_cost)
        if not can then
            ch:send_line("You are too tired to swim!")
        end
        return can
    end
end

-- Replaces has_o2(ch) in act.movement.cpp (ch:has_o2() binding will be removed).
local function has_o2(ch)
    if ch:admin_flagged(ADM.WALKANYWHERE) then return true end
    if ch:condition_has("rune_laguz") then return true end
    local r = ch:race_get()
    return r == "kanassan" or r == "android" or r == "icer" or r == "majin"
end

-- Replaces roll_pursue(fighter, vict) in act.movement.cpp.
-- fighter is the character who was fighting the fleeing vict.
local function roll_pursue(fighter, vict)
    if not fighter or not vict then return end
    local skill = 0
    local perc  = dbat.axion_dice(0)
    if fighter:is_npc() then
        if fighter:mob_flagged(MF.SENTINEL) then return end
        skill = fighter:stat_get("level") or 0
        local vr = vict:room_get()
        if vr and vr:flagged(RF.NOMOB) then skill = -1 end
    else
        skill = fighter:skill_get("pursuit") or 0
    end
    if not vict:is_npc() and fighter:is_npc() and not vict:has_connection() then
        skill = -1
    end
    if skill > perc then
        act.message({ room = "@C$n@R pursues after the fleeing @c$N@R!@n" },
            { actor = fighter, target = vict })
        fighter:to_room(vict:room_get())
        act.message({
            actor  = "@GYou pursue right after @c$N@G!@n",
            target = "@C$n@R pursues after you!@n",
            room   = "@C$n@R pursues after the fleeing @c$N@R!@n",
        }, { actor = fighter, target = vict })
        fighter:followers_each(function(fol)
            if fol:room_get() == vict:room_get() and fol:position_get() >= POS.STANDING then
                if not fighter:condition_has("zanzoken") or
                   (fighter:condition_has("group") and fol:condition_has("group")) then
                    act.message(
                        { actor = "You follow $N.", room = "$n follows after $N." },
                        { actor = fol, target = fighter })
                    fol:to_room(fighter:room_get())
                end
            end
        end)
        vict:aff_flag_set(AFF.PURSUIT, false)
    else
        fighter:send_line("@RYou fail to pursue after them!@n")
        if fighter:fighting_get() then fighter:stop_fighting() end
        if vict:fighting_get() then vict:stop_fighting() end
    end
end

-- Lua port of do_simple_move. Returns true on success.
-- dir is a 0-based direction index (0=north … 11=outside).
-- is_follower=true skips the special-proc check (followers don't retrigger procs).
local function simple_move(ch, dir, is_follower)
    local was_in = ch:room_get()
    if not was_in then return false end
    local ex   = was_in:exit_get(dir)
    local dest = ex and ex:destination()
    if not dest then return false end

    -- Special proc dispatch (non-followers only, matching need_specials_check=0 for followers)
    if not is_follower and ch:check_special(dir) then return false end

    -- DG leave triggers (with teleport-crash guard each time)
    if not dbat.dgscripts.leave_mtrigger(ch, dir) or ch:room_get() ~= was_in then return false end
    if not dbat.dgscripts.leave_wtrigger(was_in, ch, dir) or ch:room_get() ~= was_in then return false end
    if not dbat.dgscripts.leave_otrigger(was_in, ch, dir) or ch:room_get() ~= was_in then return false end

    -- Charm: refuse to leave master
    if ch:aff_flagged(AFF.CHARM) then
        local master = ch:following_get()
        if master and master:room_get() == was_in then
            ch:send_line("The thought of leaving your master makes you weep.")
            ch:act_around("$n bursts into tears.")
            return false
        end
    end

    -- Flying-sector check: will we fall on arrival?
    local willfall = false
    local sect      = was_in:sector_type_get()
    local dest_sect = dest:sector_type_get()
    if sect == ST.FLYING or dest_sect == ST.FLYING then
        if not has_flight(ch) then
            if dir ~= 4 then
                willfall = true
            else
                ch:send_line("You need to fly to go there!")
                return false
            end
        end
    end

    -- Water / no-swim check
    if (sect == ST.WATER_NOSWIM or dest_sect == ST.WATER_NOSWIM) and is_humanoid(ch) then
        local race = ch:race_get()
        if (race == "kanassan" or race == "icer") and not has_flight(ch) then
            ch:send_line("@CYou swim swiftly.@n")
            ch:act_around("@c$n@C swims swiftly.@n")
        elseif not has_flight(ch) then
            if not check_swim(ch) then return false end
            ch:send_line("@CYou swim through the cold water.@n")
            ch:act_around("@c$n@C swim through the cold water.@n")
            ch:wait_set(10) -- PULSE_1SEC
        end
    end

    -- Space room: non-android must spend KI to move
    if was_in:flagged(RF.SPACE) and ch:race_get() ~= "android" then
        if not check_swim(ch) then return false end
    end

    -- NPC: block entry into lava rooms for non-humanoid NPCs
    if dest:geffect_get() == 6 and not is_humanoid(ch) and ch:is_npc() then return false end

    -- NPC: block entry into NOMOB rooms unless charmed/following
    if ch:is_npc() and dest:flagged(RF.NOMOB) and not ch:following_get() then return false end

    -- Sunken room breathing
    if was_in:is_sunken() or dest:is_sunken() then
        if not has_o2(ch) then
            local gb           = ch:group_bonus(2)
            local ki_threshold = math.floor(ch:meter_max("ki") / (gb == 10 and 800 or 200))
            if ch:meter_current("ki") < ki_threshold then
                local max_pl = ch:meter_max("powerlevel")
                if ch:meter_current("powerlevel") >= math.floor(max_pl / 20) then
                    ch:send_line("@RYou struggle to breath!@n")
                    ch:meter_mod_int("powerlevel", -math.floor(max_pl / 20))
                else
                    ch:send_line("@rYou drown!@n")
                    ch:die(nil)
                    return false
                end
            else
                ch:send_line("@CYou hold your breath!@n")
                ch:meter_mod_int("ki", -ki_threshold)
            end
        end
    end

    -- Gravity movement cost
    local cost = enter_leave_cost(ch, was_in)
    -- Stealth multipliers
    if ch:aff_flagged(AFF.HIDE) then
        cost = cost * (roll_skill_approx(ch, "hide") > 15 and 2 or 4)
    end
    if ch:aff_flagged(AFF.SNEAK) then
        cost = math.floor(cost * (roll_skill_approx(ch, "move_silently") > 15 and 1.2 or 2))
    end

    -- Flying KI cost (and fall if out of KI)
    if ch:condition_has("flying") and ch:race_get() ~= "android" then
        local conc  = ch:skill_get("concentration") or 0
        local focus = ch:skill_get("focus") or 0
        local ki_max = ch:meter_max("ki")
        local flight_cost
        if conc == 0 and focus == 0 then
            flight_cost = math.floor(ki_max / 100)
        elseif conc > 0 and focus == 0 then
            flight_cost = math.floor(ki_max / (conc * 2))
        elseif conc == 0 and focus > 0 then
            flight_cost = math.floor(ki_max / (focus * 3))
        else
            flight_cost = math.floor(ki_max / (conc * 2 + focus * 3))
        end
        if ch:meter_current("ki") < flight_cost then
            ch:meter_mod_int("ki", -flight_cost)
            ch:send_line("@WYou crash to the ground, too tired to fly anymore!@n")
            ch:act_around("@W$n@W crashes to the ground!@n")
            ch:condition_remove("flying", "stop_flying")
        else
            ch:meter_mod_int("ki", -flight_cost)
        end
    end

    -- Stamina exhaustion check
    if not ch:condition_has("flying") and not ch:is_npc() and ch:meter_current("stamina") < cost then
        ch:send_line(is_follower and "You are too exhausted to follow." or "You are too exhausted.")
        return false
    end

    -- TODO: exit DC skill check (needs exit:dcskill_get() / exit:dcmove_get() bindings)

    -- House atrium check
    if was_in:flagged(RF.ATRIUM) and not dest:house_can_enter(ch) then
        ch:send_line("That's private property -- no trespassing!")
        return false
    end

    -- Tunnel capacity check
    if dest:flagged(RF.TUNNEL) and count_pc_in_room(dest) >= 1 then
        ch:send_line("There isn't enough room there for more than one person!")
        return false
    end

    -- God room check
    if dest:flagged(RF.GODROOM) and ch:admin_level_get() < ADMLVL_GRGOD then
        ch:send_line("You aren't godly enough to use that room!")
        return false
    end

    -- Zone level/flag checks
    local zone = dest:zone_get()
    if zone then
        local zmin   = zone:min_level_get()
        local zmax   = zone:max_level_get()
        local lvl    = ch:stat_get("level") or 0
        local admlvl = ch:admin_level_get()
        if not ch:is_npc() and admlvl < ADMLVL_IMMORT and zmin > 0 and lvl < zmin then
            ch:send_line("Sorry, you are too low a level to enter this zone.")
            return false
        end
        if admlvl < ADMLVL_IMMORT and zmax > 0 and lvl > zmax then
            ch:send_line("Sorry, you are too high a level to enter this zone.")
            return false
        end
        if admlvl < ADMLVL_IMMORT and zone:flagged(ZF.CLOSED) then
            ch:send_line("This zone is currently closed to mortals.")
            return false
        end
        if admlvl >= ADMLVL_IMMORT and admlvl < ADMLVL_GRGOD and zone:flagged(ZF.NOIMMORT) then
            ch:send_line("This zone is closed to all.")
            return false
        end
        -- TODO: use can_edit_zone() for the ZONE_QUEST check; for now block all low immortals
        if admlvl >= ADMLVL_IMMORT and admlvl < ADMLVL_GOD and zone:flagged(ZF.QUEST) then
            ch:send_line("This is a Quest zone.")
            return false
        end
    end

    -- Deduct stamina (not for NPCs, admins with walkanywhere, or flying characters)
    if not ch:is_npc() and not ch:admin_flagged(ADM.WALKANYWHERE) and not ch:condition_has("flying") then
        ch:meter_mod_int("stamina", -cost)
    end

    -- Leave messages
    -- TODO: act.lua doesn't support TO_SNEAKRESIST; messages go to all in room
    local is_sneak  = ch:aff_flagged(AFF.SNEAK) and not ch:is_npc()
    local is_flying = ch:condition_has("flying")
    if is_sneak then
        local ms = ch:skill_get("move_silently") or 0
        if ms > 0 then
            ch:improve_skill("move_silently", 0)
        end
        -- TODO: teach move_silently from scratch (needs ch:skill_set() binding)
        act.message({ room = "$n sneaks " .. DIR_NAMES[dir + 1] .. "." }, { actor = ch })
    elseif is_flying then
        act.message({ room = "$n flies " .. DIR_NAMES[dir + 1] .. "." }, { actor = ch })
    else
        act.message({ room = "$n leaves " .. DIR_NAMES[dir + 1] .. "." }, { actor = ch })
    end

    -- Pre-move companion announcements
    local dragging = ch:dragging_get()
    local carrying = ch:carrying_char_get()
    if dragging then
        act.message({ room = "@C$n@w drags @c$N@w with $m.@n" }, { actor = ch, target = dragging })
    end
    if carrying then
        act.message({ room = "@C$n@w carries @c$N@w with $m.@n" }, { actor = ch, target = carrying })
    end

    -- Move character to destination
    ch:aff_flag_set(AFF.PURSUIT, true)
    ch:to_room(dest)

    -- Zone-change scouter blip
    if not ch:is_npc() and ch:race_get() ~= "android" then
        if dest:zone_vnum_get() ~= was_in:zone_vnum_get() then
            ch:send_to_sense(0, "You sense someone")
            ch:send_to_scouter(
                string.format("@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range@n.",
                    dbat.add_commas(ch:meter_current("powerlevel"))),
                0, 0)
        end
    end

    -- Entry triggers (roll back on denial)
    if not dbat.dgscripts.entry_mtrigger(ch) or not dbat.dgscripts.enter_wtrigger(dest, ch, dir) then
        ch:to_room(was_in)
        ch:aff_flag_set(AFF.PURSUIT, false)
        return false
    end

    -- Arrival message
    local from_str
    if dir == 4 then
        from_str = "below"
    elseif dir == 5 then
        from_str = "above"
    else
        from_str = "the " .. DIR_NAMES[REV_DIR[dir] + 1]
    end
    act.message({ room = "$n arrives from " .. from_str .. "." }, { actor = ch })

    -- Combat pursuit
    local fighting = ch:fighting_get()
    if fighting then
        local dsect = dest:sector_type_get()
        if dsect ~= ST.FLYING and dsect ~= ST.WATER_NOSWIM and dest:geffect_get() == 0 then
            roll_pursue(fighting, ch)
        end
        ch:aff_flag_set(AFF.PURSUIT, false)
    end

    -- Move dragging companion to new room
    if dragging then
        act.message({
            actor = "@wYou drag @C$N@w with you.@n",
            room  = "@C$n@w drags @c$N@w with $m.@n",
        }, { actor = ch, target = dragging })
        dragging:to_room(ch:room_get())
        -- TODO: SITS(dragging) object movement
        if not dragging:aff_flagged(AFF.KNOCKED) and not dragging:aff_flagged(AFF.SLEEP) then
            if math.random(1, 3) ~= 1 then
                dragging:send_line("You feel your sleeping body being moved.")
                if dragging:is_npc() and not dragging:fighting_get() then
                    dragging:start_fighting(ch)
                end
            end
        end
    end

    -- Move carrying companion to new room
    if carrying then
        act.message({
            actor = "@wYou carry @C$N@w with you.@n",
            room  = "@C$n@w carries @c$N@w with $m.@n",
        }, { actor = ch, target = carrying })
        carrying:to_room(ch:room_get())
        -- TODO: SITS(carrying) object movement
        if not carrying:aff_flagged(AFF.KNOCKED) and not carrying:aff_flagged(AFF.SLEEP) then
            if math.random(1, 3) ~= 1 then
                carrying:send_line("You feel your sleeping body being moved.")
            end
        end
    end

    -- Look at room + sneak noise reveal on arrival
    if not ch:is_npc() then
        ch:look_at_room()
        local ms = ch:skill_get("move_silently") or 0
        if ch:aff_flagged(AFF.SNEAK) and ms > 0 and ms < math.random(1, 101) then
            ch:send_line("@wYou make a noise as you arrive and are no longer sneaking!@n")
            act.message({ room = "@c$n@w makes a noise revealing $s sneaking!@n" }, { actor = ch })
            ch:reveal_hiding(0)
            ch:aff_flag_set(AFF.SNEAK, false)
        end
    end

    -- Lava damage (geffect == 6) for current or source room
    local cur_room = ch:room_get()
    if cur_room:geffect_get() == 6 or was_in:geffect_get() == 6 then
        if ch:race_get() ~= "demon" and not ch:condition_has("flying") and ch:group_bonus(2) ~= 14 then
            ch:send_line("@rYour legs are burned by the lava!@n")
            act.message({ room = "@R$n@r's legs are burned by the lava!@n" }, { actor = ch })
            -- TODO: NPC auto-fly call when lava damages them (do_fly equivalent)
            local max_pl = ch:meter_max("powerlevel")
            ch:meter_mod_int("powerlevel", -math.floor(max_pl / 20))
            if ch:meter_current("powerlevel") <= 0 then
                ch:send_line("@rYou have burned to death!@n")
                act.message({ room = "@R$n@r has burned to death!@n" }, { actor = ch })
                ch:die(nil)
                return true
            end
        end
        -- Lava damage for dragged companion
        if dragging and dragging:race_get() ~= "demon" then
            act.message(
                { actor = "@R$N@r gets burned!@n", room = "@R$N@r gets burned!@n" },
                { actor = ch, target = dragging })
            dragging:meter_mod_int("powerlevel", -math.floor(dragging:meter_max("powerlevel") / 20))
            if dragging:meter_current("powerlevel") < 0 then
                act.message(
                    { actor = "@rYou have burned to death!@n", room = "@R$n@r has burned to death!@n" },
                    { actor = dragging })
                dragging:die(nil)
            end
        end
    end

    -- Entry memory trigger and greet triggers (greet can roll back)
    dbat.dgscripts.entry_memory_mtrigger(ch)
    if not dbat.dgscripts.greet_mtrigger(ch, dir) then
        ch:to_room(was_in)
        ch:look_at_room()
    else
        dbat.dgscripts.greet_memory_mtrigger(ch)
    end

    -- Fall if we moved into/from a flying-sector without flight
    if willfall then
        handle_fall(ch)
        local still_dragging = ch:dragging_get()
        if still_dragging then handle_fall(still_dragging) end
    end

    return true
end

-- Lua port of do_simple_enter. Returns true on success.
local function simple_enter(ch, obj, is_follower)
    local dest = dbat.rooms.by_id(obj:value_get(0))
    if not dest then
        ch:send_line("That doesn't appear to lead anywhere.")
        return false
    end

    local room = ch:room_get()

    -- Charmed: refuses to leave master
    if ch:aff_flagged(AFF.CHARM) then
        local master = ch:following_get()
        if master and master:room_get() == room then
            ch:send_line("The thought of leaving your master makes you weep.")
            ch:act_around("$n bursts into tears.")
            return false
        end
    end

    -- Stamina cost
    local cost = enter_leave_cost(ch, room)
    if ch:meter_current("stamina") < cost and not ch:condition_has("flying") and not ch:is_npc() then
        if is_follower and ch:following_get() then
            ch:send_line("You are too exhausted to follow.")
        else
            ch:send_line("You are too exhausted.")
        end
        return false
    end

    -- House atrium check
    if room and room:flagged(RF.ATRIUM) then
        if not dest:house_can_enter(ch) then
            ch:send_line("That's private property -- no trespassing!")
            return false
        end
    end

    -- Tunnel capacity check (CONFIG_TUNNEL_SIZE == 1)
    if dest:flagged(RF.TUNNEL) and count_pc_in_room(dest) >= 1 then
        ch:send_line("There isn't enough room there for more than one person!")
        return false
    end

    -- God room check
    if dest:flagged(RF.GODROOM) and ch:admin_level_get() < ADMLVL_GRGOD then
        ch:send_line("You aren't godly enough to use that room!")
        return false
    end

    -- Deduct stamina
    if not ch:is_npc() and not ch:admin_flagged(ADM.WALKANYWHERE) and not ch:condition_has("flying") then
        ch:meter_mod_int("stamina", -cost)
    end

    -- Pre-move messages
    act.message({ room = "$n enters $p." }, { actor = ch, tool = obj })

    local dragging = ch:dragging_get()
    if dragging then
        act.message({ room = "@C$n@w drags @c$N@w with $m.@n" }, { actor = ch, target = dragging })
    end
    local carrying = ch:carrying_char_get()
    if carrying then
        act.message({ room = "@C$n@w carries @c$N@w with $m.@n" }, { actor = ch, target = carrying })
    end

    -- Move character
    local was_in = room
    ch:to_room(dest)

    -- Entry trigger: roll back if denied
    if not dbat.dgscripts.entry_mtrigger(ch) then
        ch:to_room(was_in)
        return false
    end

    -- Arrival messages
    if obj:type_get() == ITEM.PORTAL then
        act.message({ room = "$n arrives from $p." }, { actor = ch, tool = obj })
    else
        act.message({ room = "$n arrives from outside." }, { actor = ch })
    end

    -- Move dragged companion
    if dragging then
        act.message({
            actor = "@wYou drag @C$N@w with you.@n",
            room  = "@C$n@w drags @c$N@w with $m.@n",
        }, { actor = ch, target = dragging })
        if not dragging:aff_flagged(AFF.KNOCKED) and not dragging:aff_flagged(AFF.SLEEP) then
            dragging:send_line("You feel your sleeping body being moved.")
            if dragging:is_npc() and not dragging:fighting_get() then
                dragging:start_fighting(ch)
            end
        end
        dragging:to_room(ch:room_get())
        -- TODO: SITS(dragging) object movement
    end

    -- Move carried companion
    if carrying then
        act.message({
            actor = "@wYou carry @C$N@w with you.@n",
            room  = "@C$n@w carries @c$N@w with $m.@n",
        }, { actor = ch, target = carrying })
        if not carrying:aff_flagged(AFF.KNOCKED) and not carrying:aff_flagged(AFF.SLEEP) then
            carrying:send_line("You feel your sleeping body being moved.")
        end
        carrying:to_room(ch:room_get())
        -- TODO: SITS(carrying) object movement
    end

    -- Look at new room
    if not ch:is_npc() then ch:look_at_room() end

    -- Death trap
    local cur_room = ch:room_get()
    if cur_room and cur_room:flagged(RF.DEATH) and not ch:admin_flagged(ADM.WALKANYWHERE) then
        ch:die(nil)
        return false
    end

    dbat.dgscripts.entry_memory_mtrigger(ch)
    dbat.dgscripts.greet_memory_mtrigger(ch)
    return true
end

-- Lua port of do_simple_leave. Returns true on success.
local function simple_leave(ch, obj, is_follower)
    local vehicle = nil
    if obj:type_get() ~= ITEM.PORTAL then
        vehicle = obj:hatch_vehicle_get()
    end

    if vehicle == nil and obj:type_get() ~= ITEM.PORTAL then
        ch:send_line("That doesn't appear to lead anywhere.")
        return false
    end

    if obj:type_get() == ITEM.PORTAL and (obj:value_get(1) & CF.CLOSED) ~= 0 then
        ch:send_line("But it's closed!")
        return false
    end

    local dest
    if vehicle then
        dest = vehicle:room_get()
        if not dest then
            ch:send_line("That doesn't appear to lead anywhere.")
            return false
        end
    else
        dest = dbat.rooms.by_id(obj:value_get(0))
        if not dest then
            ch:send_line("That doesn't appear to lead anywhere.")
            return false
        end
    end

    local room = ch:room_get()

    -- Charmed: refuses to leave master
    if ch:aff_flagged(AFF.CHARM) then
        local master = ch:following_get()
        if master and master:room_get() == room then
            ch:send_line("The thought of leaving your master makes you weep.")
            ch:act_around("$n bursts into tears.")
            return false
        end
    end

    -- Stamina cost
    local cost = enter_leave_cost(ch, room)
    if ch:meter_current("stamina") < cost and not ch:condition_has("flying") and not ch:is_npc() then
        if is_follower and ch:following_get() then
            ch:send_line("You are too exhausted to follow.")
        else
            ch:send_line("You are too exhausted.")
        end
        return false
    end

    -- House atrium check
    if room and room:flagged(RF.ATRIUM) then
        if not dest:house_can_enter(ch) then
            ch:send_line("That's private property -- no trespassing!")
            return false
        end
    end

    -- Tunnel capacity check (CONFIG_TUNNEL_SIZE == 1)
    if dest:flagged(RF.TUNNEL) and count_pc_in_room(dest) >= 1 then
        ch:send_line("There isn't enough room there for more than one person!")
        return false
    end

    -- Deduct stamina
    if not ch:is_npc() and not ch:admin_flagged(ADM.WALKANYWHERE) and not ch:condition_has("flying") then
        ch:meter_mod_int("stamina", -cost)
    end

    -- Pre-move messages
    act.message({ room = "$n leaves $p." }, { actor = ch, tool = vehicle })

    local dragging = ch:dragging_get()
    if dragging then
        act.message({ room = "@C$n@w drags @c$N@w with $m.@n" }, { actor = ch, target = dragging })
    end
    local carrying = ch:carrying_char_get()
    if carrying then
        act.message({ room = "@C$n@w carries @c$N@w with $m.@n" }, { actor = ch, target = carrying })
    end

    -- Move character
    local was_in = room
    ch:to_room(dest)

    -- Entry trigger: roll back if denied
    if not dbat.dgscripts.entry_mtrigger(ch) then
        ch:to_room(was_in)
        return false
    end

    -- Arrival messages
    if vehicle then
        act.message({ room = "$n arrives from inside $p." }, { actor = ch, tool = vehicle })
    else
        act.message({ room = "$n arrives from inside." }, { actor = ch })
    end

    -- Move dragged companion
    if dragging then
        act.message({
            actor = "@wYou drag @C$N@w with you.@n",
            room  = "@C$n@w drags @c$N@w with $m.@n",
        }, { actor = ch, target = dragging })
        dragging:to_room(ch:room_get())
        -- TODO: SITS(dragging) object movement
        if not dragging:aff_flagged(AFF.KNOCKED) and not dragging:aff_flagged(AFF.SLEEP) then
            dragging:send_line("You feel your sleeping body being moved.")
            if dragging:is_npc() and not dragging:fighting_get() then
                dragging:start_fighting(ch)
            end
        end
    end

    -- Move carried companion
    if carrying then
        act.message({
            actor = "@wYou carry @C$N@w with you.@n",
            room  = "@C$n@w carries @c$N@w with $m.@n",
        }, { actor = ch, target = carrying })
        carrying:to_room(ch:room_get())
        -- TODO: SITS(carrying) object movement
        if not carrying:aff_flagged(AFF.KNOCKED) and not carrying:aff_flagged(AFF.SLEEP) then
            carrying:send_line("You feel your sleeping body being moved.")
        end
    end

    -- TODO: send_to_sense / send_to_scouter (scouter detection)

    -- Look at new room (show action description if any)
    if not ch:is_npc() then
        local ad = obj:action_description_get()
        if ad and ad ~= "" then ch:send_line(ad) end
        ch:look_at_room()
    end

    -- Death trap
    local cur_room = ch:room_get()
    if cur_room and cur_room:flagged(RF.DEATH) and not ch:admin_flagged(ADM.WALKANYWHERE) then
        ch:die(nil)
        return false
    end

    dbat.dgscripts.entry_memory_mtrigger(ch)
    dbat.dgscripts.greet_memory_mtrigger(ch)
    return true
end

-- Lua port of C++ perform_move(ch, dir, need_specials=1).
-- Returns true if the character actually moved.
local function perform_move(ch, dir_index)
    if ch:grappling_get() or ch:grappled_get() then
        ch:send_line("You are grappling with someone!")
        return false
    end
    if ch:absorbing_get() or ch:absorbed_by_get() then
        ch:send_line("You are struggling with someone!")
        return false
    end

    if not ch:aff_flagged(AFF.SNEAK) or
       (ch:aff_flagged(AFF.SNEAK) and ch:skill_get("move_silently") < dbat.axion_dice(0)) then
        ch:reveal_hiding(0)
    end

    local room = ch:room_get()
    local exit = room and room:exit_get(dir_index)

    if not exit or (exit:flagged(EX.SECRET) and exit:flagged(EX.CLOSED)) then
        ch:send_line("Alas, you cannot go that way...")
        return false
    end
    if exit:flagged(EX.CLOSED) then
        local kw = exit:keyword()
        if kw and kw ~= "" then
            local first_word = kw:match("^(%S+)") or kw
            ch:send_line("The %s seems to be closed.", first_word)
        else
            ch:send_line("It seems to be closed.")
        end
        return false
    end

    -- Glacial wall scan: room objects with VNUM 79, cost == dir_index block movement
    if room then
        for obj in room:contents_get() do
            if obj:vnum_get() == 79 and obj:cost_get() == dir_index then
                ch:send_line("That direction has a glacial wall blocking it.")
                return false
            end
        end
    end

    local was_in_room = room
    if not simple_move(ch, dir_index, false) then
        return false
    end

    ch:followers_each(function(k)
        if k:room_get() == was_in_room and k:position_get() >= POS.STANDING then
            local ch_zan = ch:condition_has("zanzoken")
            local k_zan  = k:condition_has("zanzoken")
            local ch_grp = ch:condition_has("group")
            local k_grp  = k:condition_has("group")
            local ctx = { actor = k, target = ch }

            if not ch_zan or (ch_grp and k_grp) then
                act.to_char(k, "You follow $N.", ctx)
                -- Followers skip special proc check (is_follower=true) and their own pre-checks
                if not k:grappling_get() and not k:grappled_get() and
                   not k:absorbing_get() and not k:absorbed_by_get() then
                    simple_move(k, dir_index, true)
                end
            elseif ch_zan and k_zan and (not ch_grp or not k_grp) then
                act.to_char(k, "$N tries to zanzoken and escape, but your zanzoken matches $S!", ctx)
                act.message({ room = "$N tries to zanzoken and escape, but $n's zanzoken matches $S!" }, ctx)
                act.to_char(ch, "You zanzoken to try and escape, but $n's zanzoken matches yours!", { actor = ch, target = k })
                ch:condition_remove("zanzoken", "zanzoken_over")
                k:condition_remove("zanzoken", "zanzoken_over")
                if not k:grappling_get() and not k:grappled_get() and
                   not k:absorbing_get() and not k:absorbed_by_get() then
                    simple_move(k, dir_index, true)
                end
            elseif ch_zan and not k_zan then
                act.to_char(k, "You try to follow $N, but $E disappears in a flash of movement!", ctx)
                act.message({ room = "$n tries to follow $N, but $E disappears in a flash of movement!" }, ctx)
                act.to_char(ch, "$n tries to follow you, but you manage to zanzoken away!", { actor = ch, target = k })
                ch:condition_remove("zanzoken", "zanzoken_over")
            end
        end
    end)

    return true
end

-- Lua port of C++ perform_enter_obj (follower wrapper around do_simple_enter).
local function perform_enter_obj(ch, obj)
    if ch:grappling_get() or ch:grappled_get() then
        ch:send_line("You are grappling with someone!")
        return false
    end
    local was_in_room = ch:room_get()
    if not simple_enter(ch, obj, false) then return false end
    ch:followers_each(function(k)
        if k:room_get() == was_in_room and k:position_get() >= POS.STANDING then
            act.to_char(k, "You follow $N.", { actor = k, target = ch })
            perform_enter_obj(k, obj)
        end
    end)
    return true
end

-- Lua port of C++ perform_leave_obj (follower wrapper around do_simple_leave).
local function perform_leave_obj(ch, obj)
    if ch:grappling_get() or ch:grappled_get() then
        ch:send_line("You are grappling with someone!")
        return false
    end
    local was_in_room = ch:room_get()
    if not simple_leave(ch, obj, false) then return false end
    ch:followers_each(function(k)
        if k:room_get() == was_in_room and k:position_get() >= POS.STANDING then
            act.to_char(k, "You follow $N.", { actor = k, target = ch })
            perform_leave_obj(k, obj)
        end
    end)
    return true
end

return {
    perform_move      = perform_move,
    perform_enter_obj = perform_enter_obj,
    perform_leave_obj = perform_leave_obj,
    DIR_NAMES         = DIR_NAMES,
    has_o2            = has_o2,
    handle_fall       = handle_fall,
}
