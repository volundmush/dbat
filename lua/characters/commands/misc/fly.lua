local dbat = require("dbat")
local act  = require("lua.libs.act")

local PLR  = dbat.consts.player_flags
local AFF  = dbat.consts.aff_flags
local RF   = dbat.consts.room_flags
local SECT = dbat.consts.sector_types
local RACE = dbat.consts.races
local POS  = dbat.consts.positions

local PLANET_TABLE = {
    {flag=RF.EARTH,   dest=50},
    {flag=RF.CERRIA,  dest=198},
    {flag=RF.VEGETA,  dest=53},
    {flag=RF.FRIGID,  dest=51},
    {flag=RF.KONACK,  dest=52},
    {flag=RF.NAMEK,   dest=54},
    {flag=RF.AETHER,  dest=55},
    {flag=RF.YARDRAT, dest=56},
    {flag=RF.ARLIA,   dest=59},
}

local function is_android(ch)
    return ch:race_get() == RACE.ANDROID
end

local function blast_off(ch, dest_vnum)
    ch:reveal_hiding(0)
    ch:condition_add("flying", "skill", "fly")
    ch:condition_number_set("flying", "altitude", 2)
    if not ch:block_calc() then return end
    ch:condition_remove("flying", "stop_flying")
    ch:fly_zone("can be seen blasting off into space!@n\r\n")
    ch:send_to_sense(1, "leaving the planet")
    ch:send_to_scouter("A powerlevel signal has left the planet", 0, 2)
    act.to_char(ch, "@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", {actor=ch})
    act.around(ch, "@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", {actor=ch})
    ch:from_room()
    ch:to_room(dest_vnum)
    act.around(ch, "@C$n blasts up from the atmosphere below and then comes to a stop.@n", {actor=ch})
    ch:send_line("@mOOC: Use the command 'land' to return to the planet from here.@n")
    if not is_android(ch) then
        ch:meter_mod("ki", -math.floor(ch:meter_max("ki") / 10))
    end
    ch:wait_set(30)
end

return {
    id = "fly",
    aliases = {
        {"fly", "fly", 3},
    },
    execute = function(ctx)
        local ch  = ctx.ch
        local arg = ctx.argparams and ctx.argparams.tokens and ctx.argparams.tokens[1] or ""

        if ch:absorbing_get() or ch:absorbed_by_get() then
            ch:send_line("You can't fly, you are struggling with someone right now!")
            return
        end
        if ch:grappling_get() or ch:grappled_get() then
            ch:send_line("You can't fly, you are struggling with someone right now!")
            return
        end
        if not ch:is_npc() then
            if ch:player_flagged(PLR.HEALT) then
                ch:send_line("You are inside a healing tank!")
                return
            end
            if ch:player_flagged(PLR.PILOTING) then
                ch:send_line("You are busy piloting a ship!")
                return
            end
        end

        if not ch:is_npc() and ch:skill_get("focus") < 30 and not is_android(ch) then
            ch:send_line("You do not have enough focus to hold yourself aloft.")
            ch:send_line("@wOOC@D: @WYou need the skill Focus at @m30@W.@n")
            return
        end

        local room = ch:room_get()
        local sect = room and room:sector_type_get() or 0

        if arg == "" then
            if ch:condition_has("flying") and sect ~= SECT.FLYING and sect ~= SECT.SPACE then
                act.to_char(ch, "@WYou slowly settle down to the ground.@n", {actor=ch})
                act.around(ch, "@W$n slowly settles down to the ground.@n", {actor=ch})
                ch:condition_remove("flying", "stop_flying")
                return
            end
            if ch:condition_has("flying") and sect == SECT.FLYING then
                act.to_char(ch, "@WYou begin to plummet to the ground!@n", {actor=ch})
                act.around(ch, "@W$n starts to pummet to the ground below!@n", {actor=ch})
                ch:condition_remove("flying", "stop_flying")
                ch:handle_fall()
                return
            end
            if ch:condition_has("flying") and sect == SECT.SPACE then
                act.to_char(ch, "@WYou let yourself drift aimlessly through space.@n", {actor=ch})
                act.around(ch, "@W$n starts to drift slowly.!@n", {actor=ch})
                ch:condition_remove("flying", "stop_flying")
                return
            end
            if ch:meter_get("ki") < math.floor(ch:meter_max("ki") / 100) and not is_android(ch) then
                ch:send_line("You do not have the ki to fly.")
                return
            end
            ch:reveal_hiding(0)
            act.to_char(ch, "@WYou slowly take off into the sky.@n", {actor=ch})
            act.around(ch, "@W$n slowly takes off into the sky.@n", {actor=ch})
            local chair = ch:sits_get()
            if chair then
                chair:sitting_set(nil)
                ch:sits_set(nil)
            end
            if ch:position_get() < POS.STANDING then
                ch:position_set(POS.STANDING)
            end
            ch:condition_add("flying", "skill", "fly")
            ch:condition_number_set("flying", "altitude", 1)
            if not is_android(ch) then
                ch:meter_mod("ki", -math.floor(ch:meter_max("ki") / 100))
            end
            return
        end

        if arg == "high" then
            if ch:meter_get("ki") < math.floor(ch:meter_max("ki") / 100) and not is_android(ch) then
                ch:send_line("You do not have the ki to fly.")
                return
            end
            ch:reveal_hiding(0)
            act.to_char(ch, "@WYou rocket high into the sky.@n", {actor=ch})
            act.around(ch, "@W$n rockets high into the sky.@n", {actor=ch})
            local chair = ch:sits_get()
            if chair then
                chair:sitting_set(nil)
                ch:sits_set(nil)
            end
            if ch:position_get() < POS.STANDING then
                ch:position_set(POS.STANDING)
            end
            ch:condition_add("flying", "skill", "fly")
            ch:condition_number_set("flying", "altitude", 2)
            if not is_android(ch) then
                ch:meter_mod("ki", -math.floor(ch:meter_max("ki") / 100))
            end
            return
        end

        if arg == "space" then
            if not ch:is_outside() then
                ch:send_line("You are not outside!")
                return
            end
            if ch:meter_get("ki") < math.floor(ch:meter_max("ki") / 10) and not is_android(ch) then
                ch:send_line("You do not have the ki to fly to space.")
                return
            end
            if ch:fighting_get() then
                ch:send_line("You are too busy fighting!")
                return
            end

            if room and room:flagged(RF.KANASSA) then
                if ch:room_get():vnum_get() ~= 14904 then
                    ch:send_line("You can only fly off the planet from the launchpad of Aquis.")
                    return
                end
                blast_off(ch, 58)
                return
            end

            if room then
                for _, p in ipairs(PLANET_TABLE) do
                    if room:flagged(p.flag) then
                        blast_off(ch, p.dest)
                        return
                    end
                end
            end

            if ch:is_planet_zenith() then
                blast_off(ch, 57)
                return
            end

            ch:send_line("You are not on a planet.")
        end
    end,
}
