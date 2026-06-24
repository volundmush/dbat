local dbat   = require("dbat")
local search = dbat.lib.search
local act    = dbat.lib.act

local PLR = dbat.consts.player_flags
local PRF = dbat.consts.prf_flags
local AFF = dbat.consts.aff_flags
local RF  = dbat.consts.room_flags

local PLANET_VNUMS = {
    ["planet-earth"]  = 300,
    ["planet-namek"]  = 10222,
    ["planet-frigid"] = 4017,
    ["planet-vegeta"] = 2200,
    ["planet-konack"] = 8006,
    ["planet-aether"] = 12024,
}

local function handle_teleport(ch, tar, location)
    local dest
    if location ~= 0 then
        dest = dbat.rooms.by_id(location)
    elseif tar then
        dest = tar:room_get()
    end

    if not dest then
        dbat.log("ERROR: handle_teleport called without a destination.")
        return
    end

    ch:to_room(dest)
    ch:act_around("@w$n@w appears in an instant out of nowhere!@n")

    local dragging = ch:dragging_get()
    local grappling = ch:grappling_get()
    local carrying = ch:carrying_char_get()
    local grappled = ch:grappled_get()

    if dragging and not dragging:is_npc() then
        dragging:to_room(ch:room_get())
        act.message({
            room = "@w$n@w appears in an instant out of nowhere being dragged by $N!@n",
        }, { actor = dragging, target = ch })
    end
    if grappling and not grappling:is_npc() then
        grappling:to_room(ch:room_get())
        act.message({
            room = "@w$n@w appears in an instant out of nowhere being grappled by $N!@n",
        }, { actor = grappling, target = ch })
    end
    if carrying then
        carrying:to_room(ch:room_get())
        act.message({
            room = "@w$n@w appears in an instant out of nowhere being carried by $N!@n",
        }, { actor = carrying, target = ch })
    end
    if grappled and not grappled:is_npc() then
        grappled:to_room(ch:room_get())
        act.message({
            room = "@w$n@w appears in an instant out of nowhere being grappled by $N!@n",
        }, { actor = grappled, target = ch })
    end

    if dragging and dragging:is_npc() then
        act.message({
            actor = "@WYou stop dragging @C$N@W!@n",
            room  = "@C$n@W stops dragging @c$N@W!@n",
        }, { actor = ch, target = dragging })
        dragging:being_dragged_set(nil)
        ch:dragging_set(nil)
    end
    if grappling and grappling:is_npc() then
        ch:grappling_set(nil, 0)
        grappling:grappled_set(nil, 0)
    end
    if grappled and grappled:is_npc() then
        grappled:grappling_set(nil, 0)
        ch:grappled_set(nil, 0)
    end
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if not ch:is_npc() and ch:pref_flagged(PRF.ARENAWATCH) then
        ch:pref_flag_set(PRF.ARENAWATCH, false)
        ch:arena_idnum_set(-1)
        ch:send_line("You stop watching the arena action.")
    end

    if ch:name_get() ~= "Anubis" then
        ch:send_line("You do not even know how to perform that skill!")
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

    if arg == "" then
        ch:send_line("Who or where do you want to shimmer to? [target | planet-(planet name) | afterlife]")
        ch:send_line("Example: shimmer goku\nExample 2: shimmer planet-earth")
        return
    end

    local cost = math.floor(ch:meter_max("ki") / 40)
    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to instantaneously move.")
        return
    end

    local location = 0
    local tar = nil

    if arg == "afterlife" then
        location = 6000
    else
        location = PLANET_VNUMS[arg] or 0
        if location == 0 then
            tar = search.new(ch):add_global_characters():find_one(arg)
            if not tar then
                ch:send_line("@RThat target doesn't exist.@n")
                ch:send_line("Who or where do you want to shimmer to? [target | planet-(planet name) | afterlife]")
                ch:send_line("Example: shimmer goku\nExample 2: shimmer planet-earth")
                return
            end
        end
    end

    local perc = dbat.axion_dice(0)
    local skill = 100
    if skill < perc or (ch:fighting_get() and math.random(1, 2) == 1) then
        if tar and tar == ch then
            ch:send_line("Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.")
            return
        end
        ch:send_line("You prepare to move instantly but mess up the process and waste some of your ki!")
        ch:meter_mod_int("ki", -cost)
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
        local tar_room = tar:room_get()
        if tar_room and tar_room:flagged(RF.NOINSTANT) then
            ch:send_line("You can not go there as it is a protected area!")
            return
        end
        local grappling = ch:grappling_get()
        if grappling and grappling:aff_flagged(AFF.SPIRIT) then
            ch:send_line("You can not take the dead with you!")
            return
        end
        local dragging = ch:dragging_get()
        if dragging and dragging:aff_flagged(AFF.SPIRIT) then
            ch:send_line("You can not take the dead with you!")
            return
        end
        local grappled = ch:grappled_get()
        if grappled and grappled:aff_flagged(AFF.SPIRIT) then
            ch:send_line("You can not take the dead with you!")
            return
        end

        ch:meter_mod_int("ki", -cost)
        act.message({
            actor  = "@wYour body begins to fade away almost appearing ghost like, before a ripple passes through your image and your are gone in an instant!@n",
            target = "@w$n@w appears in an instant out of nowhere right next to you!@n",
            room   = "@w$n@w body begins to fade away almost appearing ghost like, before a ripple passes through $s image and $e is gone in an instant!@n",
        }, { actor = ch, target = tar })
        ch:player_flag_set(PLR.TRANSMISSION, true)
        handle_teleport(ch, tar, 0)
    else
        ch:meter_mod_int("ki", -cost)
        act.message({
            actor = "@wYour body begins to fade away almost appearing ghost like, before a ripple passes through your image and your are gone in an instant!@n",
            room  = "@w$n@w body begins to fade away almost appearing ghost like, before a ripple passes through $s image and $e is gone in an instant!@n",
        }, { actor = ch })
        handle_teleport(ch, nil, location)
    end
end

return { id = "shimmer", aliases = { { "shimmer", 5 } }, execute = execute }
