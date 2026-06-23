local dbat     = require("dbat")
local movement = require("lua.libs.movement")

local PLR  = dbat.consts.player_flags
local EX   = dbat.consts.exit_flags
local RF   = dbat.consts.room_flags
local ITEM = dbat.consts.item_types

return {
    id = "leave",
    aliases = {
        {"leave", "leave", 5},
    },
    execute = function(ctx)
        local ch   = ctx.ch
        local room = ch:room_get()

        if ch:plr_flagged(PLR.HEALT) then
            ch:send_line("You are inside a healing tank!")
            return
        end

        -- look for a hatch or portal in the room
        if room then
            for obj in room:contents_get() do
                if ch:can_see_obj(obj) then
                    local t = obj:type_get()
                    if t == ITEM.HATCH or t == ITEM.PORTAL then
                        movement.perform_leave_obj(ch, obj)
                        return
                    end
                end
            end
        end

        -- check if already outside
        if ch:is_outside() then
            ch:send_line("You are outside.. where do you want to go?")
            return
        end

        -- find an exit leading to a non-INDOORS destination
        if room then
            for dir = 0, 11 do
                local exit = room:exit_get(dir)
                if exit and exit:valid() and not exit:flagged(EX.CLOSED) then
                    local dest = exit:destination()
                    if dest and not dest:flagged(RF.INDOORS) then
                        movement.perform_move(ch, dir)
                        return
                    end
                end
            end
        end

        ch:send_line("I see no obvious exits to the outside.")
    end,
}
