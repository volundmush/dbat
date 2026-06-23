local dbat = require("dbat")
local act  = require("lua.libs.act")

local SKY_ROOMS = {[50]=true,[51]=true,[52]=true,[53]=true,[54]=true,
                   [55]=true,[56]=true,[57]=true,[58]=true,[59]=true,[198]=true}

return {
    id = "land",
    aliases = {
        {"land", "land", 4},
    },
    execute = function(ctx)
        local ch    = ctx.ch
        local arg   = ctx.argparams and ctx.argparams.tokens and ctx.argparams.tokens[1] or ""
        -- join all tokens for multi-word location names
        if ctx.argparams and ctx.argparams.tokens then
            arg = table.concat(ctx.argparams.tokens, " ")
        end
        arg = arg:match("^%s*(.-)%s*$") -- trim

        local vnum = ch:room_get() and ch:room_get():vnum_get() or 0
        local above_planet = SKY_ROOMS[vnum] == true

        if arg == "" then
            if above_planet then
                ch:send_line("Land where?")
                ch:disp_locations()
            else
                ch:send_line("You are not even in the lower atmosphere of a planet!")
            end
            return
        end

        local landing = ch:land_location(arg)
        if landing ~= -1 then
            local was_in = vnum
            ch:send_line("You descend through the upper atmosphere, and coming down through the clouds you land quickly on the ground below.")
            ch:from_room()
            ch:to_room(landing)
            local location_name = ch:sense_location() or "the surface"
            ch:from_room()
            ch:to_room(was_in)
            local msg = string.format("@C$n@Y flies down through the atmosphere toward @G%s@Y!@n", location_name)
            act.around(ch, msg, {actor=ch})
            ch:from_room()
            ch:to_room(landing)
            ch:fly_zone("can be seen landing from space nearby!@n\r\n")
            ch:send_to_sense(1, "landing on the planet")
            ch:send_to_scouter("A powerlevel signal has been detected landing on the planet", 0, 1)
            act.around(ch, "$n comes down from high above in the sky and quickly lands on the ground.", {actor=ch})
        end
    end,
}
