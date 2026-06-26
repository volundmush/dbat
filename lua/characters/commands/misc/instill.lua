local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act
local EF     = dbat.consts.item_extra_flags
local IWEAR  = dbat.consts.item_wear_flags

local NUM_AFFECTS = 6

local BODY_WEAR_FLAGS = {
    IWEAR.FINGER, IWEAR.NECK,   IWEAR.BODY,   IWEAR.HEAD,  IWEAR.LEGS,
    IWEAR.FEET,   IWEAR.HANDS,  IWEAR.ARMS,   IWEAR.SHIELD, IWEAR.ABOUT,
    IWEAR.WAIST,  IWEAR.WRIST,  IWEAR.WIELD,  IWEAR.EYE,   IWEAR.EAR,
}

local function is_wearable(obj)
    for _, flag in ipairs(BODY_WEAR_FLAGS) do
        if obj:wear_flagged(flag) then return true end
    end
    return false
end

local function execute(ctx)
    local ch   = ctx.ch
    local arg1 = ctx.argparams.tokens[1] or ""
    local arg2 = ctx.argparams.tokens[2] or ""

    if arg1 == "" or arg2 == "" then
        ch:send_line("Syntax: instill (token) (target)")
        return
    end

    local token = Search(ch):add_character_inventory(ch):find_one(arg1)
    if not token then
        ch:send_line("Syntax: instill (token) (target)")
        return
    end

    local obj = Search(ch):add_character_inventory(ch):find_one(arg2)
    if not obj then
        ch:send_line("Syntax: instill (token) (target)")
        return
    end

    if not token:extra_flagged(EF.Token) then
        ch:send_line("That is not a token.")
        return
    end
    if token:extra_flagged(EF.FORGED) then
        ch:send_line("That token is a forgery!")
        return
    end
    if not is_wearable(obj) then
        ch:send_line("You can only instill tokens into equipment.")
        return
    end
    if not obj:extra_flagged(EF.SLOT1) and not obj:extra_flagged(EF.SLOT2) then
        ch:send_line("That piece of equipment does not have any slots.")
        return
    end
    if obj:extra_flagged(EF.FILLED) then
        ch:send_line("That piece of equipment has already had its token slots filled. This can not be reversed.")
        return
    end

    -- Check if all 6 affect slots are occupied with different stats
    local stat = token:affect_location_get(0)
    local all_occupied = true
    local has_matching = false
    for i = 0, NUM_AFFECTS - 1 do
        local loc = obj:affect_location_get(i)
        if loc == 0 then
            all_occupied = false
        elseif loc == stat then
            has_matching = true
        end
    end
    if all_occupied and not has_matching then
        ch:send_line("This already has as many different stats as it can hold.")
        return
    end

    act.to_char(ch, "@GYou instill the token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", { object = obj })
    act.around(ch, "@g$n@G instills a token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", { object = obj })

    local raise = token:affect_modifier_get(0)
    token:extract()

    -- Update slot tracking flags
    if obj:extra_flagged(EF.SLOT1) then
        obj:extra_flag_set(EF.FILLED, true)
    elseif obj:extra_flagged(EF.SLOT2) and not obj:extra_flagged(EF.ONEFILL) then
        obj:extra_flag_set(EF.ONEFILL, true)
    elseif obj:extra_flagged(EF.SLOT2) and obj:extra_flagged(EF.ONEFILL) then
        obj:extra_flag_set(EF.FILLED, true)
    end

    -- Find matching slot or empty slot
    for i = 0, NUM_AFFECTS - 1 do
        local loc = obj:affect_location_get(i)
        if loc == stat then
            obj:affect_set(i, loc, 0, obj:affect_modifier_get(i) + raise)
            return
        end
    end
    for i = 0, NUM_AFFECTS - 1 do
        if obj:affect_location_get(i) == 0 then
            obj:affect_set(i, stat, 0, raise)
            return
        end
    end
end

return {
    id      = "instill",
    aliases = { {"instill", 4} },
    execute = execute,
}
