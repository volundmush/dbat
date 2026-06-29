local dbat   = require("dbat")
local Search = dbat.lib.search.new
local text   = dbat.lib.text

local APPLY = dbat.consts.applies
local EF    = dbat.consts.item_extra_flags
local ITEM  = dbat.consts.item_types
local POS   = dbat.consts.positions

local MAX_OBJ_AFFECT = 6
local PULSE_2SEC = 20

local PERCENT_APPLIES = {
    [APPLY.REGEN]   = true,
    [APPLY.TRAIN]   = true,
    [APPLY.LIFEMAX] = true,
}

local WEAPON_LEVELS = {
    { flag = EF.WEAPLVL1, level = 1, bonus = 5 },
    { flag = EF.WEAPLVL2, level = 2, bonus = 10 },
    { flag = EF.WEAPLVL3, level = 3, bonus = 20 },
    { flag = EF.WEAPLVL4, level = 4, bonus = 30 },
    { flag = EF.WEAPLVL5, level = 5, bonus = 50 },
}

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.STANDING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    elseif pos == POS.RESTING then
        ch:send_line("Nah... You feel too relaxed to do that..")
    elseif pos == POS.SITTING then
        ch:send_line("Maybe you should get on your feet first?")
    end

    return true
end

local function object_name(obj)
    return obj:short_description_get() or "Something"
end

local function send_token_slots(ch, obj)
    local slot1 = obj:extra_flagged(EF.SLOT1)
    local slot2 = obj:extra_flagged(EF.SLOT2)
    local slot_one = obj:extra_flagged(EF.SLOT_ONE)
    local filled = obj:extra_flagged(EF.SLOTS_FILLED)

    if slot1 and not filled then
        ch:send_line("Token Slots  : @m0/1@n")
    elseif slot1 and filled then
        ch:send_line("Token Slots  : @m1/1@n")
    elseif slot2 and not slot_one and not filled then
        ch:send_line("Token Slots  : @m0/2@n")
    elseif slot2 and slot_one and not filled then
        ch:send_line("Token Slots  : @m1/2@n")
    elseif slot2 and not filled then
        ch:send_line("Token Slots  : @m2/2@n")
    end
end

local function affect_specific_suffix(obj, location, index)
    if location ~= APPLY.FEAT and location ~= APPLY.SKILL then return "" end
    local name = obj:affect_specific_name_get(index)
    if not name or name == "" then return "" end
    return string.format(" (%s)", name)
end

local function bonuses_text(obj)
    local parts = {}
    for i = 0, MAX_OBJ_AFFECT - 1 do
        local modifier = obj:affect_modifier_get(i)
        if modifier ~= 0 then
            local location = obj:affect_location_get(i)
            local apply_name = dbat.consts.apply_names[location] or "UNKNOWN"
            local percent = PERCENT_APPLIES[location] and "%" or ""
            parts[#parts + 1] = string.format("%+d%s to %s%s",
                modifier, percent, apply_name, affect_specific_suffix(obj, location, i))
        end
    end
    if #parts == 0 then return " None" end
    return table.concat(parts, ",")
end

local function special_text(obj)
    local parts = {}
    local names = dbat.consts.aff_flag_names
    for flag = 0, #names do
        local name = names[flag]
        if name ~= "" and obj:aff_flagged(flag) then
            parts[#parts + 1] = name
        end
    end
    return #parts > 0 and table.concat(parts, " ") or "NOBITS"
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if blocked_by_position(ch) then return end
    if ch:is_npc() then return end
    if not ch:know_skill("appraise") then return end

    if arg == "" then
        ch:send_line("Okay, appraise what?")
        return
    end

    local obj = Search(ch):add_character_inventory(ch):find_one(arg)
    if not obj then
        ch:send_line("You want to appraise what?")
        return
    end

    local name = object_name(obj)

    ch:reveal_hiding(0)
    ch:act("@c$n@w looks at $p, turning it over in $s hands.@n", true, obj, nil, "room")
    ch:improve_skill("appraise", 1)

    if ch:skill_get("appraise") < dbat.axion_dice(-10) then
        ch:send_line("You fail to perceive the worth of %s..", name)
        ch:act("@c$n@w looks stumped about $p.@n", true, obj, nil, "room")
        ch:wait_set(PULSE_2SEC)
        return
    end

    if obj:extra_flagged(EF.BROKEN) then
        ch:send_line("%s is broken!", name)
        ch:act("@c$n@w looks at $p and frowns.@n", true, obj, nil, "room")
        ch:wait_set(PULSE_2SEC)
        return
    end

    if obj:extra_flagged(EF.FORGED) then
        ch:send_line("%s is fake and worthless!", name)
        ch:act("@c$n@w looks at $p with an angry face.@n", true, obj, nil, "room")
        ch:wait_set(PULSE_2SEC)
        return
    end

    local display_level = obj:level_get()
    if obj:type_get() == ITEM.WEAPON and obj:extra_flagged(EF.CUSTOM) then
        display_level = 20
    end

    ch:send_line("%s is worth: %s\r\nMin Lvl: %d", name, text.add_commas(obj:cost_get()), display_level)

    if obj:type_get() == ITEM.WEAPON then
        for _, entry in ipairs(WEAPON_LEVELS) do
            if obj:extra_flagged(entry.flag) then
                ch:send_line("Weapon Level: %d\nDamage Bonus: %d%%", entry.level, entry.bonus)
                break
            end
        end
    end

    ch:send_line("Size: %s", dbat.consts.size_names[obj:size_get()] or "Unknown")
    send_token_slots(ch, obj)
    ch:send("Bonuses:")
    ch:act("@c$n@w looks at $p and nods, a satisfied look on $s face.@n", true, obj, nil, "room")
    ch:send(bonuses_text(obj))
    ch:send_line("\nSpecial: %s", special_text(obj))
    ch:wait_set(PULSE_2SEC)
end

return {
    id      = "appraise",
    aliases = { {"appraise", 7} },
    execute = execute,
}
