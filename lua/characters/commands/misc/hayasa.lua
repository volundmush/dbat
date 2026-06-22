local dbat = require("dbat")
local P    = dbat.consts.pulses

local function execute(ctx)
    local ch    = ctx.ch
    local skill = ch:skill_get("hayasa")

    if skill == 0 then
        ch:send_line("You do not know how to perform this technique!")
        return
    end

    if ch:condition_has("hayasa") then
        ch:send_line("You are already focusing ki to continually speed up your movements.")
        return
    end

    local cost = math.floor(ch:meter_max("ki") / (skill / 2))

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki.")
        return
    end

    ch:meter_mod("ki", -cost)

    if skill < dbat.axion_dice(0) then
        ch:send_line("@CYou close your eyes for a brief moment and focus your ki around your body as a soft blue glow. The glow disappears though as you fail to maintain the effect...@n")
        ch:act_around("@c$n@C closes $s eyes for a brief moment and a soft blue glow begins to form around $s body. The glow disappears a second later though and $e frowns.@n")
        ch:improve_skill("hayasa", 1)
        ch:wait_set(P.two_sec)
    else
        ch:condition_apply("hayasa", "hayasa", "activate")
        ch:reveal_hiding(0)
        ch:send_line("@CYou close your eyes for a brief moment and focus your ki around your body as a soft blue glow. All your movements are faster now!@n")
        ch:act_around("@c$n@C closes $s eyes for a brief moment and a soft blue glow begins to form around $s body. The glow pulsates gently as $e opens his eyes and smiles.@n")
        ch:improve_skill("hayasa", 1)
        ch:wait_set(P.two_sec)
    end
end

return { id = "hayasa", aliases = { { "hayasa", 4 } }, execute = execute }
