local dbat     = require("dbat")
local movement = require("lua.libs.movement")

local EX   = dbat.consts.exit_flags
local RF   = dbat.consts.room_flags
local ITEM = dbat.consts.item_types

local function isname(str, namelist)
    if not str or str == "" or not namelist or namelist == "" then return false end
    local lower = str:lower()
    for word in namelist:gmatch("%S+") do
        local wlow = word:lower()
        if wlow:sub(1, #lower) == lower then
            if lower:match("^%d") and lower ~= wlow then return false end
            return true
        end
    end
    return false
end

local function find_obj_vis(ch, name, iter)
    for obj in iter do
        if ch:can_see_obj(obj) and isname(name, obj:name_get() or "") then
            return obj
        end
    end
    return nil
end

return {
    id = "enter",
    aliases = {
        {"enter", 5},
    },
    execute = function(ctx)
        local ch  = ctx.ch
        local arg = ctx.argparams and ctx.argparams.tokens and ctx.argparams.tokens[1] or ""
        local room = ch:room_get()

        if arg ~= "" then
            -- search room objects, then inventory, then equipment
            local obj = room and find_obj_vis(ch, arg, room:contents_get()) or nil
            if not obj then
                obj = find_obj_vis(ch, arg, ch:inventory_get())
            end
            if not obj then
                for i = 0, 19 do
                    local e = ch:equipment_get(i)
                    if e and ch:can_see_obj(e) and isname(arg, e:name_get() or "") then
                        obj = e
                        break
                    end
                end
            end

            if obj then
                movement.perform_enter_obj(ch, obj)
                return
            end

            -- search exits by keyword
            if room then
                for dir = 0, 11 do
                    local exit = room:exit_get(dir)
                    if exit and exit:valid() and exit:destination() then
                        local kw = exit:keyword()
                        if kw and isname(arg, kw) then
                            movement.perform_move(ch, dir)
                            return
                        end
                    end
                end
            end

            ch:send_line("There is no %s here.", arg)
            return
        end

        -- no arg: try to find a door into an INDOORS room
        if room and room:flagged(RF.INDOORS) then
            ch:send_line("You are already indoors.")
            return
        end

        if room then
            for dir = 0, 11 do
                local exit = room:exit_get(dir)
                if exit and exit:valid() and not exit:flagged(EX.CLOSED) then
                    local dest = exit:destination()
                    if dest and dest:flagged(RF.INDOORS) then
                        movement.perform_move(ch, dir)
                        return
                    end
                end
            end
        end

        ch:send_line("You can't seem to find anything to enter.")
    end,
}
