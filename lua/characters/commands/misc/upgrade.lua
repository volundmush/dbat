local dbat = require("dbat")

local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions
local Search = dbat.lib.search.new
local text = dbat.lib.text

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

local function fmt(n)
    return text.add_commas(math.floor(n))
end

local function stat_name(arg)
    arg = string.lower(arg or "")
    if arg == "powerlevel" or arg == "ki" or arg == "stamina" then return arg end
    return nil
end

local function find_augmentation_kit(ch)
    return Search(ch):add_character_inventory(ch):find_one("Augmentation")
end

local function augment_gain(ch)
    local gain = math.floor(ch:meter_max("powerlevel") * 0.005)
    if ch:stat_get("level") == 100 and gain > 10000000 then gain = 10000000 end
    return gain
end

local function do_augment(ch, arg2)
    if ch:stat_get("level") < 80 then
        ch:send_line("You need to be at least level 80 to use these kits.")
        return
    end

    local obj = find_augmentation_kit(ch)
    if not obj then
        ch:send_line("You don't have a Circuit Augmentation Kit.")
        return
    end

    local stat = stat_name(arg2)
    if not stat then
        ch:send_line("What do you want to augment? Powerlevel, ki, or stamina?")
        return
    end

    local gain = augment_gain(ch)
    obj:extract()

    if stat == "powerlevel" then
        ch:act("@WYou install the circuits and upgrade your maximum powerlevel.@n", true, nil, nil, "char")
        ch:stat_mod("powerlevel", gain)
    elseif stat == "ki" then
        ch:act("@WYou install the circuits and upgrade your maximum ki.@n", true, nil, nil, "char")
        ch:stat_mod("ki", gain)
    else
        ch:act("@WYou install the circuits and upgrade your maximum stamina.@n", true, nil, nil, "char")
        ch:stat_mod("stamina", gain)
    end
    ch:act("@C$n@W installs some circuits and upgrades $s systems.@n", true, nil, nil, "room")
    ch:send_line("@gGain @D[@G+%s@D]", fmt(gain))
end

local function upgrade_bonus(level, stat)
    if stat == "powerlevel" then
        if level >= 90 then return level * 5000 end
        if level >= 80 then return level * 2500 end
        if level >= 70 then return level * 2000 end
        if level >= 60 then return level * 1300 end
        if level >= 50 then return level * 500 end
        if level >= 25 then return level * 250 end
        return level * 150
    end

    if level >= 90 then return level * 3650 end
    if level >= 80 then return level * 2450 end
    if level >= 70 then return level * 1800 end
    if level >= 60 then return level * 1250 end
    if level >= 50 then
        return stat == "stamina" and level * 500 or level * 400
    end
    if level >= 25 then return level * 200 end
    return level * 120
end

local function upgrade_cost(stat)
    if stat == "powerlevel" then return 75 end
    if stat == "ki" then return 40 end
    return 50
end

local function do_upgrade_stat(ch, stat, amount)
    local count = amount
    local bonus = 0
    local cost = 0
    local level = ch:stat_get("level")

    while count > 0 do
        bonus = bonus + upgrade_bonus(level, stat)
        cost = cost + upgrade_cost(stat)
        count = count - 1
    end

    local upgrades = ch:stat_get("upgrades")
    if cost > upgrades then
        ch:send_line("You need %s upgrade points, and only have %s.", fmt(cost), fmt(upgrades))
        return
    end
    if ch:is_soft_cap(bonus) then
        ch:send_line("@mYou can't spend that much UGP on it as it will go over your softcap.@n")
        return
    end

    ch:stat_mod("upgrades", -cost)
    ch:send("You upgrade your system and gain %s %s!", fmt(bonus), stat)
    ch:stat_mod(stat, bonus)
end

local function send_menu(ch)
    if not ch:player_flagged(PLR.ABSORB) then
        ch:send("@c--------@D[@rUpgrade Menu@D]@c--------\r\n@cUpgrade @RPowerlevel@D: @Y75 @WPoints\r\n@cUpgrade @CKi        @D: @Y40 @WPoints\r\n@cUpgrade @GStamina   @D: @Y50 @WPoints\r\n@D            -----------\r\n")
    end
    ch:send("@cAugment @RPowerlevel\r\n@cAugment @CKi\r\n@cAugment @GStamina\r\n@WCurrent Upgrade Points @D[@y%s@D]@n\r\n", fmt(ch:stat_get("upgrades")))
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = string.lower(ctx.argparams.tokens[1] or "")
    local arg2 = string.lower(ctx.argparams.tokens[2] or "")

    if blocked_by_position(ch) then return end

    if ch:is_npc() or ch:race_get() ~= "android" then
        ch:send_line("You are not an android!")
        return
    end

    if arg == "" then
        send_menu(ch)
        return
    end

    if arg == "augment" then
        do_augment(ch, arg2)
        return
    end

    if ch:player_flagged(PLR.ABSORB) then
        ch:send_line("You are an absorb model and can only upgrade with augmentation kits.")
        return
    end

    if ch:is_soft_cap(0) then
        ch:send_line("@mYou are unable to spend anymore UGP right now (Softcap)@n")
        return
    end

    local stat = stat_name(arg)
    if stat then
        if arg2 == "" then
            ch:send("How many times do you want to increase %s?", arg)
            return
        end
        local amount = atoi(arg2)
        if amount <= 0 or amount > 1000 then
            ch:send_line("It needs to be between 1-1000")
            return
        end
        do_upgrade_stat(ch, stat, amount)
        return
    end

    ch:send_line("That is not a valid upgrade option.")
end

return {
    id = "upgrade",
    aliases = { {"upgrade", 6} },
    execute = execute,
}
