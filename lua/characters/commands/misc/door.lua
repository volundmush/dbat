local dbat   = require("dbat")
local act    = require("lua.libs.act")
local Search = dbat.lib.search.new

local ITEM = dbat.consts.item_types
local EX   = dbat.consts.exit_flags
local CONT = dbat.consts.container_flags
local ADM  = dbat.consts.admin_flags
local RF   = dbat.consts.room_flags
local DIR  = dbat.consts.directions

local CMD_DOOR = {"open", "close", "unlock", "lock", "pick"}

local NEED_OPEN     = 1
local NEED_CLOSED   = 2
local NEED_UNLOCKED = 4
local NEED_LOCKED   = 8

local flags_door = {
    [0] = NEED_CLOSED | NEED_UNLOCKED,
    [1] = NEED_OPEN,
    [2] = NEED_CLOSED | NEED_LOCKED,
    [3] = NEED_CLOSED | NEED_UNLOCKED,
    [4] = NEED_CLOSED | NEED_LOCKED,
}

local VAL_CONTAINER_FLAGS = 1
local VAL_KEY_KEYCODE     = 2
local VAL_PORTAL_DEST     = 0
local VAL_DOOR_DCLOCK     = 8

local DIR_NAMES   = dbat.consts.direction_names
local DIR_ABBREVS = dbat.consts.direction_abbrevs

local rev_dir = {
    [0]=2,[1]=3,[2]=0,[3]=1,[4]=5,[5]=4,
    [6]=8,[7]=9,[8]=6,[9]=7,[10]=11,[11]=10,
}

local DIR_BY_NAME = {}
for i, n in ipairs(DIR_NAMES)   do DIR_BY_NAME[n] = i - 1 end
for i, n in ipairs(DIR_ABBREVS) do
    if DIR_BY_NAME[n] == nil then DIR_BY_NAME[n] = i - 1 end
end

local function is_name(name, namelist)
    if not namelist then return false end
    local n = name:lower()
    for word in namelist:lower():gmatch("%S+") do
        if word:sub(1, #n) == n then return true end
    end
    return false
end

local function first_word(s)
    return s and s:match("^%S+") or nil
end

local function an(s)
    return ("aeiouAEIOU"):find(s:sub(1,1), 1, true) and "an" or "a"
end

local function objval_flagged(obj, flag)
    return (obj:value_get(VAL_CONTAINER_FLAGS) & flag) ~= 0
end

local function door_is_openable(ch, obj, door)
    if obj then
        local ot = obj:type_get()
        return (ot == ITEM.CONTAINER or ot == ITEM.VEHICLE or
                ot == ITEM.HATCH or ot == ITEM.WINDOW or ot == ITEM.PORTAL)
               and objval_flagged(obj, CONT.CLOSEABLE)
    end
    local ex = ch:room_get():dir_option_get(door)
    return ex ~= nil and ex:flagged(EX.ISDOOR)
end

local function door_is_open(ch, obj, door)
    if obj then return not objval_flagged(obj, CONT.CLOSED) end
    local ex = ch:room_get():dir_option_get(door)
    return ex == nil or not ex:flagged(EX.CLOSED)
end

local function door_is_unlocked(ch, obj, door)
    if obj then return not objval_flagged(obj, CONT.LOCKED) end
    local ex = ch:room_get():dir_option_get(door)
    return ex == nil or not ex:flagged(EX.LOCKED)
end

local function door_is_pickproof(ch, obj, door)
    if obj then return objval_flagged(obj, CONT.PICKPROOF) end
    local ex = ch:room_get():dir_option_get(door)
    return ex ~= nil and ex:flagged(EX.PICKPROOF)
end

local function door_key(ch, obj, door)
    if obj then return obj:value_get(VAL_KEY_KEYCODE) end
    local ex = ch:room_get():dir_option_get(door)
    return ex and ex:key() or -1
end

local function door_dclock(ch, obj, door)
    if obj then return obj:value_get(VAL_DOOR_DCLOCK) end
    local ex = ch:room_get():dir_option_get(door)
    return ex and ex:dclock_get() or 0
end

local function do_open_door(room, obj, door)
    if obj then
        obj:value_set(VAL_CONTAINER_FLAGS, obj:value_get(VAL_CONTAINER_FLAGS) & ~CONT.CLOSED)
    else
        local ex = room:dir_option_get(door)
        if ex then ex:flag_set(EX.CLOSED, false) end
    end
end

local function do_close_door(room, obj, door)
    if obj then
        obj:value_set(VAL_CONTAINER_FLAGS, obj:value_get(VAL_CONTAINER_FLAGS) | CONT.CLOSED)
    else
        local ex = room:dir_option_get(door)
        if ex then ex:flag_set(EX.CLOSED, true) end
    end
end

local function do_lock_door(room, obj, door)
    if obj then
        obj:value_set(VAL_CONTAINER_FLAGS, obj:value_get(VAL_CONTAINER_FLAGS) | CONT.LOCKED)
    else
        local ex = room:dir_option_get(door)
        if ex then ex:flag_set(EX.LOCKED, true) end
    end
end

local function do_unlock_door(room, obj, door)
    if obj then
        obj:value_set(VAL_CONTAINER_FLAGS, obj:value_get(VAL_CONTAINER_FLAGS) & ~CONT.LOCKED)
    else
        local ex = room:dir_option_get(door)
        if ex then ex:flag_set(EX.LOCKED, false) end
    end
end

local function do_toggle_lock(room, obj, door)
    if obj then
        obj:value_set(VAL_CONTAINER_FLAGS, obj:value_get(VAL_CONTAINER_FLAGS) ~ CONT.LOCKED)
    else
        local ex = room:dir_option_get(door)
        if ex then ex:flag_toggle(EX.LOCKED) end
    end
end

local function find_door(ch, type_str, dir_str, cmdname)
    if dir_str ~= "" then
        local door = DIR_BY_NAME[dir_str:lower()]
        if door == nil then
            ch:send_line("That's not a direction.")
            return -1
        end
        local ex = ch:room_get():dir_option_get(door)
        if not ex then
            ch:send_line("I really don't see how you can %s anything there.", cmdname)
            return -1
        end
        local kw = ex:keyword()
        if kw and not is_name(type_str, kw) then
            ch:send_line("I see no %s there.", type_str)
            return -1
        end
        return door
    else
        if type_str == "" then
            ch:send_line("What is it you want to %s?", cmdname)
            return -1
        end
        local num_dirs = dbat.consts.NUM_OF_DIRS
        for door = 0, num_dirs - 1 do
            local ex = ch:room_get():dir_option_get(door)
            if ex then
                local kw = ex:keyword()
                if kw and is_name(type_str, kw) then
                    return door
                end
            end
        end
        ch:send_line("There doesn't seem to be %s %s that could be manipulated in that way here.", an(type_str), type_str)
        return -1
    end
end

local function has_key(ch, key)
    if key == 1 then return true end
    for obj in ch:inventory() do
        if obj:vnum_get() == key then return true end
    end
    for obj in ch:equipment_get() do
        if obj:vnum_get() == key then return true end
    end
    return false
end

local function ok_pick(ch, keynum, pickproof, dclock, subcmd, hatch)
    if subcmd ~= 4 then return true end

    if ch:skill_get("open_lock") == 0 then
        ch:send_line("You have no idea how!")
        return false
    end
    if not ch:inventory_find_vnum(18) then
        ch:send_line("You need a lock picking kit.")
        return false
    end
    if hatch ~= nil and (hatch:type_get() == ITEM.HATCH or hatch:type_get() == ITEM.VEHICLE) then
        ch:send_line("No picking ship hatches.")
        return false
    end

    local skill_lvl = ch:roll_skill("open_lock")
    if dclock == 0 then
        dclock = math.random(1, 101)
    end

    if keynum == -1 then
        ch:send_line("Odd - you can't seem to find a keyhole.")
    elseif pickproof then
        ch:send_line("It resists your attempts to pick it.")
        act.around(ch, "@c$n@w puts a set of lockpick tools away.@n", {actor=ch})
    elseif ch:meter_current("stamina") < ch:meter_max("stamina") / 30 then
        ch:send_line("You don't have the stamina to try, it takes percision to pick locks. Not shaking tired hands.")
    elseif dclock > (skill_lvl - 2) then
        ch:send_line("You failed to pick the lock...")
        act.around(ch, "@c$n@w puts a set of lockpick tools away.@n", {actor=ch})
        ch:meter_mod_int("stamina", -(ch:meter_current("stamina") // 30))
    else
        ch:meter_mod_int("stamina", -(ch:meter_current("stamina") // 30))
        return true
    end
    return false
end

local function do_doorcmd(ch, obj, door, subcmd)
    local orig_room = ch:room_get()
    local other_room = nil
    local back = nil
    local hatch = nil
    local vehicle = nil

    if obj and obj:type_get() == ITEM.HATCH then
        vehicle = obj:hatch_vehicle_get()
    elseif obj and obj:type_get() == ITEM.VEHICLE then
        local interior = dbat.rooms.by_id(obj:value_get(VAL_PORTAL_DEST))
        if interior then
            ch:from_room()
            ch:to_room(interior)
        end
        for o in ch:room_get():contents_get() do
            if o:type_get() == ITEM.HATCH then
                hatch = o
            end
        end
    end

    if not dbat.dgscripts.door_mtrigger(ch, subcmd, door) then
        if obj and obj:type_get() == ITEM.VEHICLE then
            ch:from_room()
            ch:to_room(orig_room)
        end
        return
    end
    if not dbat.dgscripts.door_wtrigger(ch, subcmd, door) then
        if obj and obj:type_get() == ITEM.VEHICLE then
            ch:from_room()
            ch:to_room(orig_room)
        end
        return
    end

    if not obj then
        local ex = orig_room:dir_option_get(door)
        if ex then
            other_room = ex:destination()
        end
        if other_room then
            local rev = rev_dir[door]
            if rev ~= nil then
                local bx = other_room:dir_option_get(rev)
                if bx and bx:destination() == orig_room then
                    back = bx
                end
            end
        end
    end

    local cmd = CMD_DOOR[subcmd + 1]
    local dir_name = DIR_NAMES[door + 1] or "somewhere"

    if subcmd == 0 then -- OPEN
        if obj then
            if obj:type_get() == ITEM.HATCH and vehicle then
                do_open_door(ch:room_get(), vehicle, door)
                if obj:vnum_get() > 19199 then
                    ch:room_get():send_line("@wThe ship hatch opens slowly and settles onto the ground outside.")
                    local veh_room = vehicle:room_get()
                    if veh_room then
                        veh_room:send_line("@wThe ship hatch opens slowly and settles onto the ground.")
                        if veh_room:flagged(RF.SPACE) then
                            ch:room_get():send_line("@wA great vortex forms as air begins to get sucked out into the void!")
                        end
                    end
                else
                    act.to_char(ch, "@wYou open @c$p@w.", {actor=ch, object=obj})
                    act.around(ch, "@C$n@w opens @c$p@w.", {actor=ch, object=obj})
                    local veh_room = vehicle:room_get()
                    if veh_room then
                        veh_room:send_line(string.format("@wThe door to %s@w is opened from the other side.", vehicle:short_description_get()))
                    end
                end
                vehicle = nil
            end
            if obj:type_get() == ITEM.VEHICLE and hatch then
                do_open_door(ch:room_get(), hatch, door)
                ch:from_room()
                ch:to_room(orig_room)
                if obj:vnum_get() > 19199 then
                    ch:room_get():send_line("@wThe ship hatch opens slowly and settles onto the ground.")
                    local hatch_room = hatch:room_get()
                    if hatch_room then
                        hatch_room:send_line("@wThe ship hatch opens slowly.")
                        if obj:room_get() and obj:room_get():flagged(RF.SPACE) then
                            ch:room_get():send_line("@wThe air starts getting sucked out into space as the hatch opens!")
                        end
                    end
                else
                    act.to_char(ch, "@wYou open @c$p@w.", {actor=ch, object=obj})
                    act.around(ch, "@C$n@w opens @c$p@w.", {actor=ch, object=obj})
                    local hatch_room = hatch:room_get()
                    if hatch_room then
                        hatch_room:send_line("@wThe door is opened from the other side.")
                    end
                end
                hatch = nil
            end
        end
        do_open_door(ch:room_get(), obj, door)
        if back then do_open_door(other_room, obj, rev_dir[door]) end
        if not obj then
            local ex = orig_room:dir_option_get(door)
            local kw = ex and ex:keyword()
            ch:send_line("You open the %s that leads %s.", kw or "door", dir_name)
        elseif obj:type_get() ~= ITEM.VEHICLE and obj:type_get() ~= ITEM.HATCH then
            ch:send_line("You open %s.", obj:short_description_get())
        end

    elseif subcmd == 1 then -- CLOSE
        if obj then
            if obj:type_get() == ITEM.HATCH and vehicle then
                do_close_door(ch:room_get(), vehicle, door)
                if obj:vnum_get() > 19199 then
                    ch:room_get():send_line("@wThe ship hatch slowly closes, sealing the ship from the outside.")
                    local veh_room = vehicle:room_get()
                    if veh_room then
                        veh_room:send_line("@wThe ship hatch slowly closes, sealing the ship.")
                        if veh_room:flagged(RF.SPACE) then
                            ch:room_get():send_line("@wThe air stops getting sucked out into space as the hatch seals!")
                        end
                    end
                else
                    act.to_char(ch, "@wYou close @c$p@w.", {actor=ch, object=obj})
                    act.around(ch, "@C$n@w closes @c$p@w.", {actor=ch, object=obj})
                    local veh_room = vehicle:room_get()
                    if veh_room then
                        veh_room:send_line(string.format("@wThe door to %s@w is closed from the other side.", vehicle:short_description_get()))
                    end
                end
                vehicle = nil
            end
            if obj:type_get() == ITEM.VEHICLE and hatch then
                do_close_door(ch:room_get(), hatch, door)
                ch:from_room()
                ch:to_room(orig_room)
                if obj:vnum_get() > 19199 then
                    ch:room_get():send_line("@wThe ship hatch slowly closes, sealing the ship.")
                    local hatch_room = hatch:room_get()
                    if hatch_room then
                        hatch_room:send_line("@wThe ship hatch slowly closes, sealing the ship from the outside.")
                        if obj:room_get() and obj:room_get():flagged(RF.SPACE) then
                            ch:room_get():send_line("@wAir stops getting sucked out into space as the hatch seals!")
                        end
                    end
                else
                    act.to_char(ch, "@wYou close @c$p@w.", {actor=ch, object=obj})
                    act.around(ch, "@C$n@w closes @c$p@w.", {actor=ch, object=obj})
                    local hatch_room = hatch:room_get()
                    if hatch_room then
                        hatch_room:send_line(string.format("@wThe door to %s@w is closed from the other side.", hatch:short_description_get()))
                    end
                end
                hatch = nil
            end
        end
        do_close_door(ch:room_get(), obj, door)
        if back then do_close_door(other_room, obj, rev_dir[door]) end
        if not obj then
            local ex = orig_room:dir_option_get(door)
            local kw = ex and ex:keyword()
            ch:send_line("You close the %s that leads %s.", kw or "door", dir_name)
        else
            ch:send_line("You close %s.", obj:short_description_get())
        end

    elseif subcmd == 2 then -- UNLOCK
        if obj then
            if obj:type_get() == ITEM.HATCH and vehicle then
                do_unlock_door(ch:room_get(), vehicle, door)
                vehicle = nil
            end
            if obj:type_get() == ITEM.VEHICLE and hatch then
                do_unlock_door(ch:room_get(), hatch, door)
                ch:from_room()
                ch:to_room(orig_room)
                hatch = nil
            end
        end
        do_unlock_door(ch:room_get(), obj, door)
        if back then do_unlock_door(other_room, obj, rev_dir[door]) end
        if not obj then
            local ex = orig_room:dir_option_get(door)
            local kw = ex and ex:keyword()
            ch:send_line("You unlock the %s that leads %s.", kw or "door", dir_name)
        else
            ch:send_line("You unlock %s.", obj:short_description_get())
        end

    elseif subcmd == 3 then -- LOCK
        if obj then
            if obj:type_get() == ITEM.HATCH and vehicle then
                do_lock_door(ch:room_get(), vehicle, door)
                vehicle = nil
            end
            if obj:type_get() == ITEM.VEHICLE and hatch then
                do_lock_door(ch:room_get(), hatch, door)
                ch:from_room()
                ch:to_room(orig_room)
                hatch = nil
            end
        end
        do_lock_door(ch:room_get(), obj, door)
        if back then do_lock_door(other_room, obj, rev_dir[door]) end
        if not obj then
            local ex = orig_room:dir_option_get(door)
            local kw = ex and ex:keyword()
            ch:send_line("You lock the %s that leads %s.", kw or "door", dir_name)
        else
            ch:send_line("You lock %s.", obj:short_description_get())
        end

    elseif subcmd == 4 then -- PICK
        do_toggle_lock(ch:room_get(), obj, door)
        if back then do_toggle_lock(other_room, obj, rev_dir[door]) end
        ch:send_line("The lock quickly yields to your skills.")
    end

    -- Notify the room
    if subcmd ~= 4 then
        if not obj then
            local ex = orig_room:dir_option_get(door)
            local kw = ex and ex:keyword()
            local label = first_word(kw) or "door"
            act.around(ch, string.format("$n %ss the %s that leads %s.", cmd, label, dir_name), {actor=ch})
        elseif obj:room_get() ~= nil then
            act.around(ch, string.format("$n %ss $p.", cmd), {actor=ch, object=obj})
        end
    else
        if not obj then
            local ex = orig_room:dir_option_get(door)
            local kw = ex and ex:keyword()
            local label = first_word(kw) or "door"
            act.around(ch, string.format("$n skillfully picks the lock on the %s that leads %s.", label, dir_name), {actor=ch})
        elseif obj:room_get() ~= nil then
            act.around(ch, "$n skillfully picks the lock on $p.", {actor=ch, object=obj})
        end
    end

    -- Notify the other room (for exits only, on open/close/lock/unlock)
    if back and not obj then
        local back_kw = back:keyword()
        local back_label = first_word(back_kw) or "door"
        if subcmd == 0 then
            other_room:send_line(string.format("The %s that leads %s is opened from the other side.", back_label, dir_name))
        elseif subcmd == 1 then
            other_room:send_line(string.format("The %s that leads %s is closed from the other side.", back_label, dir_name))
        elseif subcmd == 2 then
            other_room:send_line(string.format("The %s that leads %s is unlocked from the other side.", back_label, dir_name))
        elseif subcmd == 3 then
            other_room:send_line(string.format("The %s that leads %s is locked from the other side.", back_label, dir_name))
        end
    end
end

local ALIAS_SUBCMD = {open=0, close=1, unlock=2, lock=3, pick=4}

return {
    id = "door",
    aliases = {
        {"open",   4},
        {"close",  5},
        {"unlock", 6},
        {"lock",   4},
        {"pick",   4},
    },
    execute = function(ctx)
        local ch     = ctx.ch
        local subcmd = ALIAS_SUBCMD[ctx.alias] or 0
        local arg    = ctx.argparams and ctx.argparams.raw or ""
        arg = arg:match("^%s*(.-)%s*$")

        if arg == "" then
            local cmd = CMD_DOOR[subcmd + 1]
            ch:send_line("%s%s what?", cmd:sub(1,1):upper(), cmd:sub(2))
            return
        end

        local type_str, dir_str = arg:match("^(%S+)%s*(.-)%s*$")
        type_str = type_str or ""
        dir_str  = dir_str  or ""

        local room = ch:room_get()
        local obj = Search(ch):add_character_inventory(ch):add_room_objects(room):find_one(type_str)

        -- if found obj is not a container/vehicle/hatch, treat as no object found
        if obj then
            local ot = obj:type_get()
            if ot ~= ITEM.CONTAINER and ot ~= ITEM.VEHICLE and ot ~= ITEM.HATCH then
                obj = nil
            end
        end

        local door = -1
        if not obj then
            door = find_door(ch, type_str, dir_str, CMD_DOOR[subcmd + 1])
        end

        if not obj and door < 0 then return end

        local keynum   = door_key(ch, obj, door)
        local dc       = door_dclock(ch, obj, door)

        -- initialize dclock if not set
        if dc == 0 then
            if obj then
                obj:value_set(VAL_DOOR_DCLOCK, 20)
            else
                local ex = ch:room_get():dir_option_get(door)
                if ex then ex:dclock_set(20) end
            end
            dc = 20
        end

        if not door_is_openable(ch, obj, door) then
            act.to_char(ch, string.format("You can't %s that!", CMD_DOOR[subcmd + 1]), {actor=ch})
        elseif not door_is_open(ch, obj, door) and (flags_door[subcmd] & NEED_OPEN) ~= 0 then
            ch:send_line("But it's already closed!")
        elseif door_is_open(ch, obj, door) and (flags_door[subcmd] & NEED_CLOSED) ~= 0 then
            ch:send_line("But it's currently open!")
        elseif door_is_unlocked(ch, obj, door) and (flags_door[subcmd] & NEED_LOCKED) ~= 0 then
            ch:send_line("Oh.. it wasn't locked, after all..")
        elseif not door_is_unlocked(ch, obj, door) and (flags_door[subcmd] & NEED_UNLOCKED) ~= 0 then
            ch:send_line("It seems to be locked.")
        elseif not has_key(ch, keynum) and not ch:adm_flagged(ADM.NOKEYS)
               and (subcmd == 3 or subcmd == 2) then
            ch:send_line("You don't seem to have the proper key.")
        elseif not obj and ok_pick(ch, keynum, door_is_pickproof(ch, obj, door), dc, subcmd, nil) then
            do_doorcmd(ch, obj, door, subcmd)
        elseif obj and ok_pick(ch, keynum, door_is_pickproof(ch, obj, door), dc, subcmd, obj) then
            do_doorcmd(ch, obj, door, subcmd)
        end
    end,
}
