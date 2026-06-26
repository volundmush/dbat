local dbat = require("dbat")
local act  = dbat.lib.act
local EF   = dbat.consts.item_extra_flags

local RUBY_VNUM = 6600
local PULSE_1SEC = 10

local function execute(ctx)
    local ch = ctx.ch

    if ch:race_get() ~= "demon" then
        ch:send_line("You are not a Demon!")
        return
    end
    if ch:skill_base_get("style") < 40 then
        ch:send_line("This requires a fighting style at level 40 or more!!")
        return
    end

    local cost = math.floor(ch:meter_max_get("ki") * 0.15)
    if ch:meter_get("ki") < cost then
        ch:send_line("You do not have enough ki to channel with!")
        return
    end

    local ruby = nil
    for obj in ch:inventory() do
        if obj:vnum_get() == RUBY_VNUM and not obj:extra_flagged(EF.HOT) then
            ruby = obj
            break
        end
    end

    if not ruby then
        ch:send_line("You do not have any uncharged blood rubies.")
        return
    end

    if ch:room_get():geffect_get() <= 0 then
        ch:send_line("There is no lava here!")
        return
    end

    local skill = ch:skill_get("style")
    local chance = dbat.axion_dice(0)

    if skill < chance then
        act.to_char(ch, "@RAs you move your ki through the lava you begin to draw heat away from it into the ruby. You screw up the rate of heating though and cause the ruby to crumble to dust!@n")
        act.around(ch, "@RAs $n@R moves $s ki through the lava $e begins to draw heat away from it into a blood ruby. However $e screws up the rate of heating and causes the ruby to crumble to dust!@n", {})
        ruby:extract()
    else
        act.to_char(ch, "@RAs you move your ki through the lava you begin to draw heat away from it into the ruby. You do so at an even rate and end up with a glowing red hot blood ruby!@n")
        act.around(ch, "@RAs $n@R moves $s ki through the lava $e begins to draw heat away from it into a blood ruby. The ruby glows red hot as $e finishes the process of channeling the heat!@n", {})
        ch:room_get():geffect_set(0)
        ruby:extra_flag_set(EF.HOT, true)
    end

    ch:meter_mod_int("ki", -cost)
    ch:wait_set(PULSE_1SEC)
end

local function can_execute(ch)
    if ch:is_npc() then return false end
    return ch:race_get() == "demon"
end

return {
    id          = "channel",
    aliases     = { {"channel", 4} },
    execute     = execute,
    can_execute = can_execute,
}
