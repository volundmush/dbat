local dbat = require("dbat")
local act  = dbat.lib.act
local PRF  = dbat.consts.prf_flags

local ARENA_VNUM_MIN  = 17800
local ARENA_VNUM_MAX  = 17874
local ARENA_LOBBY     = 17875

local function in_arena(ch)
    local vnum = ch:room_get():vnum_get()
    return vnum >= ARENA_VNUM_MIN and vnum <= ARENA_VNUM_MAX
end

local function find_watch_room(watcher)
    local target_idnum = watcher:arena_idnum_get()
    if target_idnum < 0 then return nil end
    for _, fighter in ipairs(dbat.characters.by_subscription("player")) do
        if fighter:player_id_get() == target_idnum and in_arena(fighter) then
            return fighter:room_get()
        end
    end
    return nil
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if in_arena(ch) then
        ch:send_line("You are too busy competing to be a spectator.")
        return
    end

    if arg == "" then
        ch:send_line("Syntax: arena (fighter number of participant)\n        arena look\n        arena scan\n        arena stop")
        return
    end

    if arg == "stop" then
        ch:send_line("You stop viewing what's going on in the arena.")
        ch:pref_flag_set(PRF.ARENAWATCH, false)
        ch:arena_idnum_set(-1)
        return
    end

    if ch:room_get():vnum_get() ~= ARENA_LOBBY then
        ch:send_line("You are not close enough to the arena floor to see it.")
        return
    end

    if arg == "look" then
        if not ch:pref_flagged(PRF.ARENAWATCH) then
            ch:send_line("You are not even watching anyone in the arena.")
            return
        end
        local watch_room = find_watch_room(ch)
        if watch_room then
            ch:look_at_specific_room(watch_room)
        end

    elseif arg == "scan" then
        ch:send_line("@D---@CFighters in the arena@D---@n")
        local found = false
        for _, fighter in ipairs(dbat.characters.by_subscription("player")) do
            if in_arena(fighter) then
                ch:send_line("@YFighter Number@D: @w%d, @W%s@D.@n", fighter:player_id_get(), fighter:name_get())
                found = true
            end
        end
        if not found then
            ch:send_line("@wNone.@n")
        end

    else
        local num = tonumber(arg)
        if not num or num < 0 then
            ch:send_line("That is not a valid fighter number")
            return
        end
        num = math.floor(num)

        local found = false
        for _, fighter in ipairs(dbat.characters.by_subscription("player")) do
            if fighter:player_id_get() == num and in_arena(fighter) then
                found = true
                break
            end
        end

        if found then
            act.to_char(ch, "@wYou start watching the action surrounding that particular fighter in the arena.@n")
            act.around(ch, "@C$n@w starts watching the action in the arena.@n", {})
            ch:pref_flag_set(PRF.ARENAWATCH, true)
            ch:arena_idnum_set(num)
        else
            ch:send_line("A fighter with such a number was not found in the arena.")
        end
    end
end

return {
    id      = "arena",
    aliases = { {"arena", 4} },
    execute = execute,
}
