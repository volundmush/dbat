local dbat = require("dbat")
local AFF  = dbat.consts.aff_flags

local function execute(ctx)
    local ch = ctx.ch

    if ch:aff_flagged(AFF.SPIRIT) then
        ch:send_line("You are dead. You can not stake out a room to return to upon revival.")
        return
    end

    local vnum = ch:room_get():vnum_get()
    if vnum >= 0 and vnum <= 14 then
        ch:send_line("You can not stake out an immortal room to be revived in.")
        return
    end

    ch:send_line("You stake out the room you are in and will return to it if you die and are revived.")
    ch:droom_set(vnum)
end

return {
    id      = "beacon",
    aliases = { {"stake", 4} },
    execute = execute,
}
