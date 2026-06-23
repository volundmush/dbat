local dbat     = require("dbat")
local act      = require("lua.libs.act")
local movement = require("lua.libs.movement")

local RF  = dbat.consts.room_flags
local EX  = dbat.consts.exit_flags
local POS = dbat.consts.positions

local DIR_NAME_TO_IDX = {
    north = 0,  n  = 0,
    east  = 1,  e  = 1,
    south = 2,  s  = 2,
    west  = 3,  w  = 3,
    up    = 4,  u  = 4,
    down  = 5,  d  = 5,
    northwest = 6,  nw = 6,
    northeast = 7,  ne = 7,
    southeast = 8,  se = 8,
    southwest = 9,  sw = 9,
    inside    = 10, ["in"] = 10,
    outside   = 11, out   = 11,
}

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams and ctx.argparams.tokens and ctx.argparams.tokens[1] or ""
    arg = arg:lower()

    if ch:position_get() < POS.RESTING then
        ch:send_line("You are in pretty bad shape, unable to flee!")
        return
    end
    if ch:grappling_get() or ch:grappled_get() then
        ch:send_line("You are grappling with someone!")
        return
    end
    if ch:absorbing_get() then
        ch:send_line("You are absorbing from someone!")
        return
    end
    if ch:absorbed_by_get() then
        ch:send_line("You are being absorbed from by someone!")
        return
    end
    if not ch:is_npc() then
        local room = ch:room_get()
        for obj in room:contents_get() do
            if obj:kicharge_get() > 0 and obj:user_get() == ch then
                ch:send_line("You are too busy controlling your attack!")
                return
            end
        end
    end

    local parsed_dir = arg ~= "" and DIR_NAME_TO_IDX[arg] or nil
    local room = ch:room_get()

    for _ = 1, 12 do
        local attempt = parsed_dir or math.random(0, 11)
        local exit = room:exit_get(attempt)
        if exit and not exit:flagged(EX.CLOSED) then
            local dest = exit:destination()
            if dest and not dest:flagged(RF.DEATH) then
                act.around(ch, "$n panics, and attempts to flee!", {actor = ch})

                if ch:is_npc() and dest:flagged(RF.NOMOB) then return end

                -- Glacial wall check (object vnum 79, cost == direction)
                local hit_wall = false
                for obj in room:contents_get() do
                    if obj:vnum_get() == 79 and obj:cost_get() == attempt then
                        hit_wall = true
                        break
                    end
                end
                if hit_wall then return end

                if not ch:block_calc() then return end

                -- Absorb checks inside the loop (can change after block_calc)
                local absorbing = ch:absorbing_get()
                if absorbing then
                    ch:send_line("You are busy absorbing from %s!", absorbing:name_get())
                    return
                end
                local absorber = ch:absorbed_by_get()
                if absorber then
                    if dbat.axion_dice(0) < absorber:skill_get("absorb") then
                        ch:send_line("You are being held by %s, they are absorbing you!", absorber:name_get())
                        absorber:send_line("%s struggles in your grasp!", ch:name_get())
                        ch:wait_set(20)
                        return
                    else
                        act.around(absorber, "@c$N@W manages to break loose of @C$n's@W hold!@n",
                            {actor = absorber, target = ch})
                        act.to_char(ch, "@WYou manage to break loose of @C$n's@W hold!@n",
                            {actor = absorber, target = ch})
                        act.to_char(absorber, "@c$N@W manages to break loose of your hold!@n",
                            {actor = absorber, target = ch})
                        ch:absorbed_by_set(nil)
                        absorber:absorbing_set(nil)
                    end
                end

                if ch:try_move(movement.DIR_NAMES[attempt + 1]) then
                    ch:send_line("You flee head over heels.")
                    ch:wait_set(20)
                else
                    act.around(ch, "$n tries to flee, but can't!", {actor = ch})
                    ch:wait_set(20)
                end
                return
            end
        end
    end

    ch:send_line("PANIC!  You couldn't escape!")
end

return {
    id      = "flee",
    aliases = { { "flee", "fl", 2 } },
    execute = execute,
}
