local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:name_get():lower() ~= "galeos" then
        ch:send_line("You do not know how to perform that technique.")
        return
    end
    if arg == "" then
        ch:send_line("Syntax: scry (target)")
        return
    end

    local room = ch:room_get()
    local vict = Search(ch):add_room_people(room):find_one(arg)
    if not vict then
        ch:send_line("Who are you using Oracle Scry on?")
        return
    end
    if vict:is_same(ch) then
        ch:send_line("You can't do that to yourself!")
        return
    end
    if vict:is_npc() then
        ch:send_line("No using this on mobs!")
        return
    end

    local cost = 2000
    if ch:stat_get("practices") < cost then
        ch:send_line("You do not have enough PS to Oracle Scry!")
        return
    end

    ch:reveal_hiding(0)
    act.message({
        actor  = "@GYou focus your mind and begin to allow the flood of images and energy to roar through your mind. You then allow those thoughts to make their way into the mind of @c$N@G. You can hardly comprehend the vastness of the information flooding in, yet still glimpse bits and pieces of your own destiny.@n",
        target = "@GYou see @C$n@G begin to focus, and then without warning, your mind is flooded painfully with images, energy and information. The data streams in a mad torrent through your psyche, and just when you think snapping is possible, the voice of @C$n@G comes to you and eases and guides you. You see images of potential futures, information not yet known, knowledge yet undiscovered. Though you could not fully  grasp what is to come, you feel more prepared at facing the unknown.@n",
        room   = "@C$n@W appears to be performing some sort of ritual or something with @c$N@W.@n",
    }, { actor = ch, target = vict })

    local boost = ch:stat_get("intelligence") * 0.5

    vict:stat_mod("powerlevel", math.floor(vict:stat_get("powerlevel") * 0.01 * boost))
    vict:stat_mod("ki",         math.floor(vict:stat_get("ki")         * 0.01 * boost))
    vict:stat_mod("stamina",    math.floor(vict:stat_get("stamina")    * 0.01 * boost))

    vict:send_line("Your Powerlevel, Ki, and Stamina have improved drastically! On top of that your Intelligence and Wisdom have improved permanantly!")
    vict:stat_mod("intelligence", 2)
    vict:stat_mod("wisdom", 2)
    ch:stat_mod("practices", -2000)

    if ch:stat_get("level") < 100 then
        ch:send_line("@D[@mPractice Sessions@D:@R -2000@D]@n")
        local exp_needed = ch:level_exp(ch:stat_get("level") + 1) - ch:stat_get("experience")
        if exp_needed > 0 then
            ch:stat_mod("experience", exp_needed)
            ch:send_line("The remaining experience needed for your next level up has been gained!")
        else
            ch:send_line("Due to already having enough experience to level up you gain no expereince.")
        end
    else
        local boost_pct = 0.025
        ch:stat_mod("powerlevel", math.floor(ch:stat_get("powerlevel") * boost_pct))
        ch:stat_mod("ki",         math.floor(ch:stat_get("ki")         * boost_pct))
        ch:stat_mod("stamina",    math.floor(ch:stat_get("stamina")    * boost_pct))
        ch:send_line("Your Powerlevel, Ki, and Stamina have improved!")
    end
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    return ch:name_get():lower() == "galeos"
end

return {
    id          = "scry",
    aliases     = { {"scry", 4} },
    execute     = execute,
    can_execute = can_execute,
}
