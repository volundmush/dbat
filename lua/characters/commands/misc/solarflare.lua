local dbat = require("dbat")
local SMH  = dbat.consts.secs_per_mud_hour

local function limb_ok_arm(ch)
    if ch:condition_has("mystic_melody") then
        ch:send_line("You are currently playing a song! Enter the song command in order to stop!")
        return false
    end
    if not ch:is_npc() then
        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then
            ch:send_line("You have no available arms!")
            return false
        end
    end
    return true
end

local function execute(ctx)
    local ch = ctx.ch

    if not ch:know_skill("solar flare") then return end
    if not limb_ok_arm(ch) then return end

    local prob = ch:skill_get("solar flare")
    local cost
    if prob >= 75 then
        cost = math.floor(ch:meter_max("ki") / 50)
    elseif prob >= 50 then
        cost = math.floor(ch:meter_max("ki") / 25)
    elseif prob >= 25 then
        cost = math.floor(ch:meter_max("ki") / 20)
    else
        cost = math.floor(ch:meter_max("ki") / 15)
    end

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki.")
        return
    end

    local bonus = math.floor(ch:stat_get("intelligence") / 3)
    prob = prob + bonus
    local perc = math.random(0, 101)

    if prob < perc then
        ch:send_line("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' but nothing happens!@n")
        ch:act_around("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' but nothing happens!@n")
        ch:meter_mod_int("ki", -cost)
        ch:wait_set(30)
        ch:improve_skill("solar flare", 0)
        return
    end

    ch:send_line("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' as a blinding light fills the area!@n")
    ch:act_around("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' as a blinding light fills the area!@n")

    local room = ch:room_get()
    for vict in room:people() do
        if vict ~= ch
            and not vict:player_flagged(PLR.EYEC)
            and not vict:condition_has("solar_flare")
            and vict:position_get() ~= POS.SLEEPING
        then
            vict:condition_apply_with_duration("solar_flare", "skill", "solar_flare", SMH)
            local actlib = dbat.lib.act
            actlib.message({
                actor  = "@W$N@W is @YBLINDED@W!@n",
                target = "@RYou are @YBLINDED@R!@n",
                room   = "@W$N@W is @YBLINDED@W!@n",
            }, { actor = ch, target = vict })
        end
    end

    ch:improve_skill("solar flare", 0)
    ch:meter_mod_int("ki", -cost)
    ch:wait_set(30)
end

local function can_execute(ch)
    return ch:know_skill("solar flare")
end

return { id = "solarflare", aliases = { { "solarflare", 5 } }, execute = execute, can_execute = can_execute }
