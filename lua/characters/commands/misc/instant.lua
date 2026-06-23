local dbat = require("dbat")
local search = dbat.lib.search
local RF     = dbat.consts.room_flags

local PLANET_VNUMS = {
    ["planet-earth"]  = 300,
    ["planet-namek"]  = 10222,
    ["planet-frigid"] = 4017,
    ["planet-vegeta"] = 2200,
    ["planet-konack"] = 8006,
    ["planet-aether"] = 12024,
}

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if not ch:is_npc() and ch:pref_flagged(PRF.ARENAWATCH) then
        ch:pref_flag_set(PRF.ARENAWATCH, false)
        ch:arena_idnum_set(-1)
        ch:send_line("You stop watching the arena action.")
    end

    if not ch:know_skill("instant transmission") then return end

    if ch:skill_get("sense") == 0 and not ch:player_flagged(PLR.SENSEM) then
        ch:send_line("You can't sense them to go to there!")
        return
    end
    if ch:player_flagged(PLR.PILOTING) then
        ch:send_line("You are busy piloting a ship!")
        return
    end
    if ch:player_flagged(PLR.HEALT) then
        ch:send_line("You are inside a healing tank!")
        return
    end

    local vnum = ch:room_get():vnum_get()
    if vnum >= 19800 and vnum <= 19899 then
        ch:send_line("@rYou are in a pocket dimension!@n")
        return
    end

    local room = ch:room_get()
    if room:room_flagged(RF.RHELL) or room:room_flagged(RF.AL) or room:room_flagged(RF.HELL) then
        ch:send_line("You can not leave where you are at!")
        return
    end

    if arg == "" then
        ch:send_line("Who or where do you want to instant transmission to? [target | planet-(planet name)]")
        ch:send_line("Example: instant goku\nExample 2: instant planet-earth")
        return
    end

    local skill = ch:skill_get("instant transmission")
    local cost
    if skill > 75 then
        cost = math.floor(ch:meter_max("ki") / 40)
    elseif skill > 50 then
        cost = math.floor(ch:meter_max("ki") / 20)
    elseif skill > 25 then
        cost = math.floor(ch:meter_max("ki") / 15)
    else
        cost = math.floor(ch:meter_max("ki") / 10)
    end

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to instantaneously move.")
        return
    end

    local planet_vnum = PLANET_VNUMS[arg]
    local tar = nil
    if not planet_vnum then
        tar = search.new(ch):add_global_characters():find_one(arg)
        if not tar then
            ch:send_line("@RThat target was not found.@n")
            ch:send_line("Who or where do you want to instant transmission to? [target | planet-(planet name)]")
            ch:send_line("Example: instant goku\nExample 2: instant planet-earth")
            return
        end
    end

    local perc = dbat.axion_dice(0)
    local fighting = ch:fighting_get()
    if skill < perc or (fighting and math.random(1, 2) == 1) then
        if tar and tar == ch then
            ch:send_line("Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.")
            return
        end
        ch:send_line("You prepare to move instantly but mess up the process and waste some of your ki!")
        ch:meter_mod_int("ki", -cost)
        ch:improve_skill("instant transmission", 1)
        ch:wait_set(20)
        return
    end

    ch:reveal_hiding(0)
    ch:wait_set(20)

    if tar then
        if tar == ch then
            ch:send_line("Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.")
            return
        end
        if ch:grappling_get() == tar then
            ch:send_line("You are already in the same room with them and are grappling with them!")
            return
        end
        if tar:admin_level_get() > 0 and ch:admin_level_get() < 1 then
            ch:send_line("That immortal prevents you from reaching them.")
            return
        end
        if tar:race_get() == "android" or tar:meter_current("powerlevel") < math.floor(ch:meter_current("powerlevel") * 0.001) + 1 then
            ch:send_line("You can't sense them well enough.")
            return
        end
        local tar_room = tar:room_get()
        if not room:room_flagged(RF.AL) and tar_room:room_flagged(RF.AL) then
            ch:send_line("They are dead and can't be reached.")
            return
        end
        if not room:room_flagged(RF.RHELL) and tar_room:room_flagged(RF.RHELL) then
            ch:send_line("They are dead and can't be reached.")
            return
        end
        if tar_room:room_flagged(RF.NOINSTANT) then
            ch:send_line("You can not go there as it is a protected area!")
            return
        end

        ch:meter_mod_int("ki", -cost)
        local actlib = dbat.lib.act
        actlib.message({
            actor  = "@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move through the molecules of the universe faster than the speed of light. You stop as you arrive at $N@w!@n",
            target = "@w$n@w appears in an instant out of nowhere right next to you!@n",
            room   = "@w$n@w places two fingers on $s forehead and disappears in an instant!@n",
        }, { actor = ch, target = tar })
        ch:player_flag_set(PLR.TRANSMISSION, true)
        ch:to_room(tar_room)
        ch:improve_skill("instant transmission", 1)
    else
        ch:meter_mod_int("ki", -cost)
        ch:send_line("@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move faster than light and arrive almost instantly at your destination. Having located the planet by its collective population's ki.@n")
        ch:act_around("@w$n@w places two fingers on $s forehead and disappears in an instant!@n")
        ch:to_room(dbat.rooms.by_id(planet_vnum))
        ch:improve_skill("instant transmission", 1)
    end
end

local function can_execute(ch)
    return ch:know_skill("instant transmission")
end

return { id = "instant", aliases = { { "instant", 5 } }, execute = execute, can_execute = can_execute }
