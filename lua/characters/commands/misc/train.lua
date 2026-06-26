local dbat       = require("dbat")
local BON        = dbat.consts.bonuses
local RF         = dbat.consts.room_flags
local text       = dbat.lib.text
local PULSE_3SEC = 30
local PULSE_5SEC = 50

local stat_defs = {
    str = { key = "strength",     train = "train_strength",     pos = BON.BRAWNY,    neg = BON.WIMP,    res = "stamina" },
    spd = { key = "speed",        train = "train_speed",        pos = BON.QUICK,     neg = BON.SLOW,    res = "stamina" },
    con = { key = "constitution", train = "train_constitution", pos = BON.STURDY,    neg = BON.FRAIL,   res = "stamina" },
    agl = { key = "agility",      train = "train_agility",      pos = BON.AGILE,     neg = BON.CLUMSY,  res = "stamina" },
    ["int"] = { key = "intelligence", train = "train_intelligence", pos = BON.SCHOLARLY, neg = BON.DULL,    res = "ki" },
    wis = { key = "wisdom",       train = "train_wisdom",       pos = BON.SAGE,      neg = BON.FOOLISH, res = "ki" },
}

local function compute_caps(ch)
    local caps = {
        strength     = 5000 + 500 * ch:stat_get("strength"),
        speed        = 5000 + 500 * ch:stat_get("speed"),
        constitution = 5000 + 500 * ch:stat_get("constitution"),
        agility      = 5000 + 500 * ch:stat_get("agility"),
        intelligence = 5000 + 500 * ch:stat_get("intelligence"),
        wisdom       = 5000 + 500 * ch:stat_get("wisdom"),
    }
    local race = ch:race_get()
    if race == "human" then
        caps.intelligence = math.floor(caps.intelligence * 0.75)
        caps.wisdom       = math.floor(caps.wisdom * 0.75)
    elseif race == "kanassan" then
        caps.intelligence = math.floor(caps.intelligence * 0.4)
        caps.wisdom       = math.floor(caps.wisdom * 0.4)
        caps.agility      = math.floor(caps.agility * 0.4)
    elseif race == "halfbreed" then
        caps.intelligence = math.floor(caps.intelligence * 0.75)
        caps.strength     = math.floor(caps.strength * 0.75)
    elseif race == "truffle" then
        caps.strength     = math.floor(caps.strength * 1.5)
        caps.constitution = math.floor(caps.constitution * 1.5)
    end
    return caps
end

local function cap_str(ch, stat_key, cap)
    if ch:stat_get(stat_key) >= 80 then
        return "@rCAPPED"
    end
    return text.add_commas(cap)
end

local function show_status(ch, caps)
    ch:send_line("@D-------------[ @GTraining Status @D]-------------@n")
    ch:send_line("  @mStrength Progress    @D: @R%6s/%6s@n", text.add_commas(ch:stat_get("train_strength")), cap_str(ch, "strength", caps.strength))
    ch:send_line("  @mSpeed Progress       @D: @R%6s/%6s@n", text.add_commas(ch:stat_get("train_speed")), cap_str(ch, "speed", caps.speed))
    ch:send_line("  @mConstitution Progress@D: @R%6s/%6s@n", text.add_commas(ch:stat_get("train_constitution")), cap_str(ch, "constitution", caps.constitution))
    ch:send_line("  @mIntelligence Progress@D: @R%6s/%6s@n", text.add_commas(ch:stat_get("train_intelligence")), cap_str(ch, "intelligence", caps.intelligence))
    ch:send_line("  @mWisdom Progress      @D: @R%6s/%6s@n", text.add_commas(ch:stat_get("train_wisdom")), cap_str(ch, "wisdom", caps.wisdom))
    ch:send_line("  @mAgility Progress     @D: @R%6s/%6s@n", text.add_commas(ch:stat_get("train_agility")), cap_str(ch, "agility", caps.agility))
    ch:send_line("@D  -----------------------------------------  @n")
    ch:send_line("  @CCurrent Weight Held  @D: @c%s@n", text.add_commas(ch:der_total("weight_carried")))
    ch:send_line("@D---------------------------------------------@n")
    ch:send_line("Syntax: train (str | spd | agl | wis | int | con)")
end

local train_msgs = {
    [1] = {
        { "char", "@WYou throw a flurry of punches into the air at an invisible opponent.@n" },
        { "room", "@W$n throws a flurry of punches into the air.@n" },
    },
    [2] = {
        { "char", "@WYou throw a flurry of punches into the air at an invisible opponent.@n" },
        { "room", "@W$n throws a flurry of punches into the air.@n" },
    },
    [3] = {
        { "char", "@WYou throw a flurry of punches into the air at an invisible opponent.@n" },
        { "room", "@W$n throws a flurry of punches into the air.@n" },
    },
}

local all_msgs = {
    [1] = {  -- str
        {
            { "char", "@WYou throw a flurry of punches into the air at an invisible opponent.@n" },
            { "room", "@W$n throws a flurry of punches into the air.@n" },
        },
        {
            { "char", "@WYou leap into the air and throw a wild kick at an invisible opponent@n" },
            { "room", "@W$n leaps into the air and throws a wild kick at nothing.@n" },
        },
        {
            { "char", "@WYou leap high into the air and unleash a flurry of punches and kicks at an invisible opponent@n" },
            { "room", "@W$n leaps high into the air and unleashes a flurry of punches and kicks at nothing.@n" },
        },
    },
    [2] = {  -- spd
        {
            { "char", "@WYou dash quickly around the surrounding area as fast as you can!@n" },
            { "room", "@W$n dashes quickly around the surrounding area as fast as $e can!@n" },
        },
        {
            { "char", "@WYou dodge to the side as fast as you can!@n" },
            { "room", "@W$n dodges to the side as fast as $e can!@n" },
        },
        {
            { "char", "@WYou dash backwards as fast as you can!@n" },
            { "room", "@W$n dashes backwards as fast as $e can!@n" },
        },
    },
    [3] = {  -- con
        {
            { "char", "@WYou leap into the air and then slam into the ground with your feet outstretched!@n" },
            { "room", "@W$n leaps into the air and then slams into the ground with $s feet outstretched!?@n" },
        },
        {
            { "char", "@WYou leap into the air and then slam into the ground with your fists!@n" },
            { "room", "@W$n leaps into the air and then slams into the ground with $s fists!?@n" },
        },
        {
            { "char", "@WYou leap into the air and then slam into the ground with your body!@n" },
            { "room", "@W$n leaps into the air and then slams into the ground with $s body!?@n" },
        },
    },
    [4] = {  -- agl
        {
            { "char", "@WYou do a series of backflips through the air, landing gracefully on one foot a moment later.@n" },
            { "room", "@W$n does a series of backflips through the air, landing gracefully on one foot a moment later.@n" },
        },
        {
            { "char", "@WYou flip forward and launch off your hands into the air. You land gracefully on one foot a moment later.@n" },
            { "room", "@W$n flips forward and launches off $s hands into the air. Then $e lands gracefully on one foot a moment later.@n" },
        },
        {
            { "char", "@WYou flip to the side off one hand and then land on your feet.@n" },
            { "room", "@W$n flips to the side off one hand and then lands on $s feet.@n" },
        },
    },
    [5] = {  -- int
        {
            { "char", "@WConcentrating you fly high into the air as fast as you can before settling slowly back to the ground.@n" },
            { "room", "@W$n flies high into the air as fast as $e can before settling slowly back to the ground.@n" },
        },
        {
            { "char", "@WYou focus your ki at your outstretched hand and send a mild shockwave in that direction!@n" },
            { "room", "@W$n focuses $s ki at $s outstretched hand and sends a mild shockwave in that direction!@n" },
        },
        {
            { "char", "@WYou concentrate on your ki and force torrents of it to rush out from your body randomly!@n" },
            { "room", "@W$n seems to concentrate before torrents of ki randomly blasts out from $s body!@n" },
        },
    },
    [6] = {  -- wis
        {
            { "char", "@WYou close your eyes and wage a mental battle against an imaginary opponent.@n" },
            { "room", "@W$n closes $s eyes for a moment and an expression of intensity forms on it.@n" },
        },
        {
            { "char", "@WYou look around and contemplate battle tactics for an imaginary scenario.@n" },
            { "room", "@W$n looks around and appears to be imagining things that aren't there.@n" },
        },
        {
            { "char", "@WYou invent a battle plan for a battle that doesn't exist!@n" },
            { "room", "@W$n seems to have thought of something.@n" },
        },
    },
}

local stat_order = { "str", "spd", "con", "agl", "int", "wis" }

local function execute(ctx)
    local ch  = ctx.ch
    local arg = string.lower(ctx.argparams.tokens[1] or "")

    if ch:is_npc() then return end

    if ch:der_total("weight_carried") > ch:der_total("weight_carry_capacity") then
        ch:send_line("You are weighted down too much!")
        return
    end

    local caps = compute_caps(ch)

    if arg == "" then
        show_status(ch, caps)
        return
    end

    local room    = ch:room_get()
    local weight  = ch:der_total("weight_carried")
    local gravity = room:gravity_get()
    local total   = weight * (gravity + 1)
    total = total + (gravity + 1) * (gravity + 1)  -- (gravity+1)^2; C++ used ^ which is XOR there, but intent was squaring
    local rv = room:vnum_get()
    if rv >= 6100 and rv <= 6135 then
        total = math.floor(total + total * 0.15)
    end

    local sensei_def  = dbat.registry.senseis[ch:sensei_get()]
    local in_sensei   = sensei_def and rv == sensei_def.location or false
    if in_sensei then
        if ch:stat_get("money") < 8 or ch:stat_get("practices") < 1 then
            ch:send_line("It costs 8 Zenni and 1 PS to train with your sensei.")
            return
        end
        total = math.floor(total + total * 0.85)
        local level = ch:stat_get("level")
        if     level >= 100 then total = total * 15000
        elseif level >= 80  then total = total * 1500
        elseif level >= 40  then total = total * 600
        elseif level >= 20  then total = total * 300
        elseif level >= 10  then total = total * 150
        end
        ch:send_line("@G%s begins to instruct you in training technique.@n", sensei_def.name)
    end

    local pl_max = ch:meter_max("powerlevel")
    local bonus  = 0
    if     total > pl_max * 2       then bonus = 5
    elseif total > pl_max           then bonus = 4
    elseif total > math.floor(pl_max / 2) then bonus = 3
    elseif total > math.floor(pl_max / 4) then bonus = 2
    elseif total > math.floor(pl_max / 8) then bonus = 1
    end

    local def = stat_defs[arg]
    if not def then
        ch:send_line("Syntax: train (str | spd | agl | wis | int | con)")
        return
    end

    if ch:stat_get(def.key) == 80 then
        ch:send_line("Your base %s is maxed!", def.key)
        return
    end
    if ch:stat_get(def.key) >= 70 and ch:bonus_flagged(def.neg) then
        ch:send_line("You're not able to withstand increasing your %s beyond 70.", def.key)
        return
    end

    local level    = ch:stat_get("level")
    local stat_cap = 20
    if     level >= 61 then stat_cap = 80
    elseif level > 40  then stat_cap = 60
    elseif level > 20  then stat_cap = 40
    end
    if ch:stat_get(def.key) >= stat_cap then
        ch:send_line("You have reached the stat cap for your level.")
        return
    end

    local res_max = ch:meter_max(def.res)
    local cost    = math.floor(total / (in_sensei and 25 or 20))
                  + math.floor(res_max / (in_sensei and 60 or 50))
    if ch:bonus_flagged(BON.HARDWORKER) then
        cost = math.floor(cost - cost * 0.25)
    end
    if ch:meter_current(def.res) < cost then
        ch:send_line("You do not have enough %s with the current weight worn and gravity!", def.res)
        return
    end

    -- Compute plus before spending (uses same non-sensei divisors as C++)
    local plus = math.floor(
        (math.floor(total / 20) + math.floor(res_max / 50)) * 100 / res_max
    )

    ch:meter_mod_int(def.res, -cost)
    ch:reveal_hiding(0)

    -- Training message
    local stat_idx  = ({ str=1, spd=2, con=3, agl=4, ["int"]=5, wis=6 })[arg]
    local msg_group = all_msgs[stat_idx][math.random(1, 3)]
    for _, m in ipairs(msg_group) do
        ch:act(m[2], true, nil, nil, m[1])
    end

    plus = plus + 75
    plus = plus * 2
    if rv >= 19800 and rv <= 19899 then plus = plus * 4 end
    if room:flagged(RF.HBTC)       then plus = plus * 3 end
    if ch:bonus_flagged(BON.HARDWORKER) then plus = math.floor(plus + plus * 0.25) end
    if ch:bonus_flagged(def.pos)        then plus = math.floor(plus + plus * 0.75) end
    if ch:bonus_flagged(BON.LONER)      then plus = math.floor(plus + plus * 0.05) end
    if in_sensei                        then plus = math.floor(plus + plus * 0.2)  end

    local gain_base = ({ 5, 10, 25, 50, 100 })[bonus]
    if gain_base then
        ch:stat_mod(def.train, gain_base + plus)
        local msgs = {
            "You feel slight improvement. @D[@G+%d@D]@n",
            "You feel some improvement. @D[@G+%d@D]@n",
            "You feel good improvement. @D[@G+%d@D]@n",
            "You feel great improvement! @D[@G+%d@D]@n",
            "You feel awesome improvement! @D[@G+%d@D]@n",
        }
        ch:send_line(msgs[bonus], gain_base + plus)
        ch:wait_set(bonus >= 4 and PULSE_5SEC or PULSE_3SEC)
    else
        ch:stat_mod(def.train, 1)
        ch:send_line("You barely feel any improvement. @D[@G+1@D]@n")
        ch:wait_set(PULSE_3SEC)
    end

    if in_sensei then
        ch:stat_mod("money", -8)
        ch:stat_mod("practices", -1)
    end

    local needed    = caps[def.key]
    local train_val = ch:stat_get(def.train)
    if train_val >= needed then
        ch:stat_set(def.train, train_val - needed)
        ch:send_line("You feel your %s improve!", def.key)
        ch:stat_mod(def.key, 1)
        if ch:sensei_get() == "piccolo" and ch:race_get() == "namek" then
            local next_level = level + 1
            if ch:level_exp(next_level) - ch:stat_get("experience") > 0 then
                ch:stat_mod("experience", math.floor(ch:level_exp(next_level) * 0.25))
                ch:send_line("You gained quite a bit of experience from that!")
            end
        end
        ch:rp_save()
    end
end

return {
    id      = "train",
    aliases = { {"train", 5} },
    execute = execute,
}
