local dbat = require("dbat")
local act  = require("lua.libs.act")

local AFF = dbat.consts.aff_flags
local EX  = dbat.consts.exit_flags
local POS = dbat.consts.positions

local DIR_NAMES = {
    "north", "east", "south", "west", "up", "down",
    "northwest", "northeast", "southeast", "southwest", "inside", "outside"
}

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
    if not ch:try_move(DIR_NAMES[dir_index + 1]) then
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
                perform_move(k, dir_index)
            elseif ch_zan and k_zan and (not ch_grp or not k_grp) then
                act.to_char(k, "$N tries to zanzoken and escape, but your zanzoken matches $S!", ctx)
                act.message({ room = "$N tries to zanzoken and escape, but $n's zanzoken matches $S!" }, ctx)
                act.to_char(ch, "You zanzoken to try and escape, but $n's zanzoken matches yours!", { actor = ch, target = k })
                ch:condition_remove("zanzoken", "zanzoken_over")
                k:condition_remove("zanzoken", "zanzoken_over")
                perform_move(k, dir_index)
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
    if not ch:try_enter(obj) then return false end
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
    if not ch:try_leave(obj) then return false end
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
}
