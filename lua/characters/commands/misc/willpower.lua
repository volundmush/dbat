local dbat = require("dbat")

local POS = dbat.consts.positions

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.RESTING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    end

    return true
end

local function spend_attempt_cost(ch)
    ch:stat_set("experience", 0)
    ch:stat_mod("practices", -100)
    if ch:stat_get("level") >= 100 then
        ch:stat_mod("practices", -100)
    end
end

local function execute(ctx)
    local ch = ctx.ch

    if blocked_by_position(ch) then return end
    if ch:is_npc() then return end

    if not ch:condition_has("majinized") then
        ch:send_line("You are not majinized and have no need to reclaim full control of your own will.")
        return
    end

    local level = ch:stat_get("level")
    local practices = ch:stat_get("practices")

    if practices < 100 and level < 100 then
        ch:send_line("You do not have enough PS to focus your attempt to break free.")
        return
    end
    if practices < 200 and level >= 100 then
        ch:send_line("You do not have enough PS to focus your attempt to break free.")
        return
    end
    if ch:stat_get("experience") < ch:level_exp(level + 1) and level < 100 then
        ch:send_line("You need a full level's worth of experience stored up to try and break free.")
        return
    end

    spend_attempt_cost(ch)
    if math.random(10, 100) - ch:stat_get("intelligence") > 60 then
        ch:reveal_hiding(0)
        ch:act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments you give up, having failed to overcome the majinization!@n",
            true, nil, nil, "char")
        ch:act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments $n seems to give up and the commotion dies down.@n",
            true, nil, nil, "room")
        return
    end

    spend_attempt_cost(ch)
    ch:reveal_hiding(0)
    ch:act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments the ground splits beneath you and while letting out a piercing scream the M disappears from your forehead! You are free while still keeping the boost you had recieved from the majinization!@n",
        true, nil, nil, "char")
    ch:act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments the ground beneath $n splits and $e lets out a piercing scream. The M on $s forehead disappears!@n",
        true, nil, nil, "room")
    ch:condition_number_set("majinized", "lord", 3)
end

return {
    id = "willpower",
    aliases = { {"will", 3} },
    execute = execute,
}
