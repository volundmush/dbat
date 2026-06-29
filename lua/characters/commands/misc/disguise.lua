local dbat = require("dbat")
local PLR  = dbat.consts.player_flags
local WEAR = dbat.consts.wear_positions

local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then
        ch:send_line("You forgot your disguise off in mobland.")
        return
    end

    if ch:player_flagged(PLR.DISGUISED) then
        ch:send_line("You stop disguising yourself.")
        ch:player_flag_set(PLR.DISGUISED, false)
        ch:act("@C$n @wpulls off $s disguise and reveals $mself!", true, nil, nil, "room")
        return
    end

    if not ch:know_skill("disguise") then return end

    if not ch:equipment_get(WEAR.HEAD) then
        ch:send_line("You can't disguise your identity without anything on your head.")
        return
    end

    local stamina = ch:meter_current("stamina")
    if stamina < math.floor(stamina / 50) then
        ch:send_line("You are too tired to try that right now.")
        return
    end

    local skill = ch:skill_get("disguise")
    local roll = dbat.axion_dice(-10)

    if skill > roll then
        ch:send_line("You managed to disguise yourself with some skilled manipulation of your headwear.")
        ch:act("@C$n @wmanages to disguise $mself with some skilled manipulation of $s headwear.", true, nil, nil, "room")
        ch:player_flag_set(PLR.DISGUISED, true)
    else
        ch:send_line("You finish attempting to disguise yourself, but realize you failed and need to try again.")
        ch:act("@C$n @wattempts and fails to disguise $mself properly and must try again.", true, nil, nil, "room")
        ch:meter_mod_int("stamina", -math.floor(ch:meter_max("stamina") / 50))
    end
end

return {
    id      = "disguise",
    aliases = { {"disguise", 7} },
    execute = execute,
}
