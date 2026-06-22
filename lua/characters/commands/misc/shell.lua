local dbat = require("dbat")

local function execute(ctx)
    local ch = ctx.ch

    if ch:condition_has("arlian_shell") then
        ch:send_line("@mYou quickly absorb the armor carapace covering your body back inside.@n")
        ch:act_around("@M$n's@m armored carapce retreats back to its original size.@n")
        ch:condition_remove("arlian_shell", "retract")
        return
    end

    if ch:meter_current("stamina") < math.floor(ch:meter_max("stamina") * 0.2) then
        ch:send_line("You do not have enough stamina to grow your armored carapace.")
        return
    end

    local con = ch:stat_get("constitution")
    if dbat.axion_dice(0) > con + math.random(1, 10) then
        ch:send_line("@mYou crouch down and begin to focus on your body's carapace cells encouraging them to multiply! However your control is lacking and you ultimately fail to grow your armor very much.@n")
        ch:act_around("@M$n@m crouches down and seems to strain for a moment before giving up and resuming $s normal stance.@n")
        return
    end

    ch:send_line("@mYou crouch down and begin to focus on your body's carapace cells, encouraging them to multiply! Very quickly millions of new carapace cells have been born and your armored carapace extends over all parts of your body!@n")
    ch:act_around("@M$n@m crouches down and after a few moments of straining $s body's carapace armor starts to grow thicker and extends to cover all parts of $s body!@n")
    ch:meter_mod("stamina", -math.floor(ch:meter_max("stamina") * 0.2))
    ch:condition_apply("arlian_shell", "shell", "activate")
end

local function can_execute(ch)
    if ch:race_get() ~= "arlian" then
        return false, "You are not capable of doing that!"
    end
    if ch:sex_get() == "female" then
        return false, "Sorry, you can't do that."
    end
    return true
end

return { id = "shell", aliases = { { "shell", 5 } }, execute = execute, can_execute = can_execute }
