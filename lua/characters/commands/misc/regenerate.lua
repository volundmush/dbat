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

local function atoi(str)
    local n = tostring(str or ""):match("^([+-]?%d+)")
    return tonumber(n) or 0
end

local function is_bio(ch)
    return ch:race_get() == "bio"
end

local function restore_limb(ch, limb, regrow_self, regrow_room, mend_self, mend_room)
    local cond = ch:limbcond_get(limb)
    if cond <= 0 then
        ch:act(regrow_self, true, nil, nil, "char")
        ch:act(regrow_room, true, nil, nil, "room")
        ch:limbcond_set(limb, 100)
    elseif cond < 50 then
        ch:act(mend_self, true, nil, nil, "char")
        ch:act(mend_room, true, nil, nil, "room")
        ch:limbcond_set(limb, 100)
    end
end

local function restore_body_parts(ch)
    if ch:is_npc() then return end

    restore_limb(ch, 1,
        "You regrow your right arm!",
        "$n regrows $s right arm!",
        "Your broken right arm mends itself!",
        "$n regenerates $s broken right arm!")
    restore_limb(ch, 2,
        "You regrow your left arm!",
        "$n regrows $s left arm!",
        "Your broken left arm mends itself!",
        "$n regenerates $s broken left arm!")
    restore_limb(ch, 4,
        "You regrow your left leg!",
        "$n regrows $s left leg!",
        "Your broken left leg mends itself!",
        "$n regenerates $s broken left leg!")
    restore_limb(ch, 3,
        "You regrow your right leg!",
        "$n regrows $s right leg!",
        "Your broken right leg mends itself!",
        "$n regenerates $s broken right leg!")

    if not ch:has_tail() and is_bio(ch) then
        ch:gain_tail()
        ch:act("You regrow your tail!", true, nil, nil, "char")
        ch:act("$n regrows $s tail!", true, nil, nil, "room")
    end

    ch:improve_skill("regenerate", 0)
end

local function send_regen_message(ch, amt)
    local cur_pl = ch:meter_current("powerlevel")
    local max_pl = ch:meter_max("powerlevel")

    if cur_pl >= max_pl then
        ch:act("You concentrate your ki and regenerate your body completely.", true, nil, nil, "char")
        ch:act("$n concentrates and regenerates $s body completely.", true, nil, nil, "room")
    elseif amt < math.floor(max_pl / 10) then
        ch:act("You concentrate your ki and regenerate your body a little.", true, nil, nil, "char")
        ch:act("$n concentrates and regenerates $s body a little.", true, nil, nil, "room")
    elseif amt < math.floor(max_pl / 5) then
        ch:act("You concentrate your ki and regenerate your body some.", true, nil, nil, "char")
        ch:act("$n concentrates and regenerates $s body some.", true, nil, nil, "room")
    elseif amt < math.floor(max_pl / 2) then
        ch:act("You concentrate your ki and regenerate your body a great deal.", true, nil, nil, "char")
        ch:act("$n concentrates and regenerates $s body a great deal.", true, nil, nil, "room")
    elseif cur_pl < max_pl then
        ch:act("You concentrate your ki and regenerate you nearly completely.", true, nil, nil, "char")
        ch:act("$n concentrates and regenerates $s body nearly completely.", true, nil, nil, "room")
    end
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if blocked_by_position(ch) then return end

    local skill = ch:init_skill("regenerate")
    if skill < 1 then
        ch:send_line("You are incapable of regenerating.")
        return
    end

    local suppress = ch:stat_get("suppression")
    if suppress > 0 then skill = suppress end

    if arg == "" then
        ch:send("Regenerate how much PL?\r\nMax percent you can regen: %d\r\nSyntax: regenerate (1 - 100)\r\n", skill)
        return
    end

    local max_pl = ch:meter_max("powerlevel")
    local cur_pl = ch:meter_current("powerlevel")

    if cur_pl >= max_pl then
        ch:send_line("You do not need to regenerate, you are at full health.")
        return
    end

    if suppress > 0 and cur_pl >= math.floor((max_pl / 100) * suppress) then
        ch:send_line("You do not need to regenerate, you are at full health.")
        return
    end

    local num = atoi(arg)
    if num <= 0 then
        ch:send("What is the point of that?\r\nSyntax: regenerate (1 - 100)\r\n")
        return
    end

    if num > 100 or num > skill or (suppress > 0 and num > suppress) then
        ch:send_line("You can't regenerate that much!\r\nMax you can regen: %d", skill)
        return
    end

    local amt = math.floor((max_pl * 0.01) * num)
    if amt > 1 then amt = math.floor(amt / 2) end
    if is_bio(ch) then amt = math.floor(amt * 0.9) end

    local lf_cost = math.floor(amt * 0.8)
    local ki_cost = math.floor(amt * 0.2)
    local life = ch:meter_current("lifeforce") - lf_cost
    local energy = ch:meter_current("ki") - ki_cost

    if not ch:is_npc() and (life <= 0 or energy <= 0) then
        ch:send_line("Your life force or ki are too low to regenerate that much.")
        ch:send_line("@YLF Needed@D: @C%s@w, @YKi Needed@D: @C%s@w.@n", dbat.lib.text.add_commas(lf_cost), dbat.lib.text.add_commas(ki_cost))
        return
    elseif ch:is_npc() and energy <= 0 then
        return
    end

    ch:meter_mod_int("powerlevel", amt * 2)
    if not ch:is_npc() then ch:meter_mod_int("lifeforce", -lf_cost) end
    ch:meter_mod_int("ki", -ki_cost)
    ch:reveal_hiding(0)

    send_regen_message(ch, amt)
    ch:improve_skill("regenerate", 0)
    ch:condition_remove_tag("injury", "regenerate")
    restore_body_parts(ch)
end

return {
    id = "regenerate",
    aliases = { {"regenerate", 5} },
    execute = execute,
}
