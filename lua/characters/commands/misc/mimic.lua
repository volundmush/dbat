local dbat = require("dbat")
local partial_match = dbat.lib.utils.partial_match

local function article(word)
    local first = tostring(word or ""):sub(1, 1):lower()
    if first == "a" or first == "e" or first == "i" or first == "o" or first == "u" then
        return "an"
    end
    return "a"
end

local function lower_name(race)
    return string.lower(race.name or race.id or "")
end

local function valid_for_sex(race, sex)
    if not race.valid_sexes then return true end
    for _, valid in ipairs(race.valid_sexes) do
        if valid == sex then return true end
    end
    return false
end

local function mimicable_races(ch)
    local races = {}
    for _, race in pairs(dbat.characters.registry.races or {}) do
        if race.pc_ok and race.mimic_ok ~= false and valid_for_sex(race, ch:sex_get()) then
            races[#races + 1] = race
        end
    end
    table.sort(races, function(left, right)
        return (left.legacy_id or 0) < (right.legacy_id or 0)
    end)
    return races
end

local function show_menu(ch, races)
    ch:send("@CMimic Menu\n@c--------------------@W\r\n")
    for _, race in ipairs(races) do
        ch:send_line("%s", race.name)
    end
    ch:send("Stop@n\r\n")

    local mimic_id = ch:mimic_get()
    if mimic_id ~= 0 then
        for _, race in pairs(dbat.characters.registry.races or {}) do
            if race.legacy_id == mimic_id then
                ch:send("You currently Mimic a %s", race.name)
                return
            end
        end
    end
end

local function find_race(races, arg)
    return partial_match(races, arg, {
        str_func = function(race)
            return race.name
        end,
    })
end

local function execute(ctx)
    local ch = ctx.ch
    if ch:is_npc() then return end

    local arg = (ctx.argparams.tokens[1] or "")
    local skill = ch:skill_get("mimic")

    if skill <= 0 then
        ch:send_line("You do not know how to mimic the appearance of other races.")
        return
    end

    local races = mimicable_races(ch)
    if arg == "" then
        show_menu(ch, races)
        return
    end

    if arg:lower() == "stop" then
        if ch:mimic_get() == 0 then
            ch:send_line("You are not imitating another race.")
            return
        end
        ch:act("@mYou concentrate for a moment and release the illusion that was mimicing another race.@n", true, nil, nil, "char")
        ch:act("@M$n@m concentrates for a moment and SUDDENLY $s appearance changes some what!@n", true, nil, nil, "room")
        ch:mimic_set(0)
        return
    end

    local race = find_race(races, arg)
    if not race then
        ch:send_line("That is not a race you can change into. Enter mimic without arugments for the mimic menu.")
        return
    end

    if race.legacy_id == ch:mimic_get() then
        ch:send_line("You are already mimicing that race. To stop enter 'mimic stop'")
        return
    end

    local cost = skill == 1 and ch:meter_max("ki") or 0
    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to perform the technique.")
        return
    end

    ch:meter_mod_int("ki", -cost)
    if skill < dbat.axion_dice(0) then
        ch:act("@mYou concentrate and attempt to create an illusion to obscure your racial features. However you frown as you realize you have failed.@n", true, nil, nil, "char")
        ch:act("@M$n@m concentrates and the light around them seems to shift and blur. It stops a moment later and $e frowns.@n", true, nil, nil, "room")
        return
    end

    local name = lower_name(race)
    ch:mimic_set(race.legacy_id)
    ch:send_line("@mYou concentrate for a moment and your features start to blur as you use your ki to bend the light around your body. You now appear to be %s %s.@n", article(name), name)
    ch:act(string.format("@M$n@m concentrates for a moment and $s features start to blur as light bends around $m. Now $e appears to be %s @M%s!@n", article(name), name), true, nil, nil, "room")
end

return {
    id = "mimic",
    aliases = { { "mimic", 4 } },
    execute = execute,
}
