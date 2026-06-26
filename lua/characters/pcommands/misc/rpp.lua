local dbat   = require("dbat")
local consts = dbat.consts
local Search = dbat.lib.search.new

local BONUS  = consts.bonus
local PLR    = consts.plr_flags
local RACES  = consts.races
local SEX    = consts.sexes
local EF     = consts.item_extra_flags

-- Deduct cost, sync descriptor, save, and notify immortals.
local function rp_purchase(ch, cost)
    ch:rp_set(ch:rp_get() - cost)
    ch:rp_save()
    ch:send_line("@R%d@W RPP paid for your selection. Enjoy!@n", cost)
    dbat.send_to_imm(string.format("RPP Purchase: %s %d", ch:name_get(), cost))
end

local function check_rp(ch, cost)
    if ch:rp_get() < cost then
        ch:send_line("You do not have enough RPP for that selection.")
        return false
    end
    return true
end

-- RPP Item Store entries (add new items here to expand the store).
-- Simple single-vnum items have on_purchase generated automatically below.
local STORE_ITEMS = {
    -- 1: Stardust Equipment Set (vnums 1110-1119; 1117-1119 given twice)
    {
        name       = "Stardust Equipment Set",
        cost       = 20,
        min_level  = 50,
        weight_add = 26,
        count_add  = 13,
        on_purchase = function(ch)
            for vnum = 1110, 1119 do
                local copies = (vnum >= 1117) and 2 or 1
                for _ = 1, copies do
                    local obj = dbat.read_object(vnum)
                    obj:to_char(ch)
                    obj:size_set(ch:size_get())
                end
            end
        end,
    },
    -- 2-7: Skill weapons (vnums 1120-1125)
    { name = "Platinum Masamune (Sword)", cost = 5, min_level = 40, vnum = 1120, weight_add = 2, count_add = 1 },
    { name = "Obsidian Dirk (Dagger)",    cost = 5, min_level = 40, vnum = 1121, weight_add = 2, count_add = 1 },
    { name = "Emerald Javelin (Spear)",   cost = 5, min_level = 40, vnum = 1122, weight_add = 2, count_add = 1 },
    { name = "Ivory Cane (Club)",         cost = 5, min_level = 40, vnum = 1123, weight_add = 2, count_add = 1 },
    { name = "Hyper X65 Cannon (Gun)",    cost = 5, min_level = 40, vnum = 1124, weight_add = 2, count_add = 1 },
    { name = "Jagged Rock (Brawl)",       cost = 5, min_level = 40, vnum = 1125, weight_add = 2, count_add = 1 },
    -- 8: Kachin Mountain (exceptionally heavy training weight)
    { name = "Kachin Mountain",           cost = 8, min_level = 0,  vnum = 1126, weight_add = 10000000, count_add = 1 },
    -- 9: Spar Booster
    { name = "Spar Booster",              cost = 15, min_level = 0, vnum = 1127, weight_add = 2, count_add = 1 },
}

-- Auto-generate on_purchase for simple single-vnum items.
for _, item in ipairs(STORE_ITEMS) do
    if not item.on_purchase and item.vnum then
        local v    = item.vnum
        local is_mountain = (v == 1126)
        item.on_purchase = function(ch)
            local obj = dbat.read_object(v)
            if is_mountain then obj:weight_set(10000000) end
            obj:to_char(ch)
            obj:size_set(ch:size_get())
        end
    end
end

local function disp_store(ch)
    ch:send_line("@m                        RPP Item Store@n")
    ch:send_line("@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n")
    ch:send_line("@GItem Name                      @gRPP Cost        @cChoice Number   @yMin Lvl@n")
    for i, item in ipairs(STORE_ITEMS) do
        local min_str = (item.min_level > 0) and string.format("@w%d@n", item.min_level) or ""
        ch:send_line("@W%-31s @D[@Y%2d@D]            @D[@C%d@D]            %s@n",
            item.name, item.cost, i, min_str)
    end
    ch:send_line("@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n")
    ch:send_line("@wSyntax: rpp 12 (choice number)@n")
end

local function handle_store(ch, choice)
    local item = STORE_ITEMS[choice]
    if not item then
        ch:send_line("That is not a selection option!")
        return
    end
    if not check_rp(ch, item.cost) then return end

    local carry_w = ch:der_total("weight_carried")
    local carry_n = ch:inventory_count(false)

    if carry_w + item.weight_add > ch:carry_weight_max() then
        ch:send_line("You can not carry that much weight at this moment.")
        return
    end
    if carry_n + item.count_add > 50 then
        ch:send_line("You have too many items on you to carry anymore at this moment.")
        return
    end
    if item.min_level > 0 and ch:stat_get("level") < item.min_level then
        ch:send_line("You are below the minimum level to equip it.")
        return
    end

    item.on_purchase(ch)
    rp_purchase(ch, item.cost)
end

-- Selection 13: distinguishing feature purchase / change.
local function handle_feature(ch, arg)
    if not arg or arg == "" then
        ch:send_line("Syntax: rpp 13 (description)\nExample: rpp 13 a large red scar on his face\nDisplayed to others: He has a large red scar on his face.")
        return
    end
    if #arg > 60 then
        ch:send_line("Please limit it to 60 characters.")
        return
    end
    if not check_rp(ch, 2) then return end

    local sex_word
    if ch:sex_get() == SEX.FEMALE then
        sex_word = "She"
    elseif ch:sex_get() == SEX.MALE then
        sex_word = "He"
    else
        sex_word = "It"
    end

    local changing = ch:feature_get() ~= nil
    local buf = string.format("...%s has %s.", sex_word, arg)

    ch:feature_set(buf)
    rp_purchase(ch, 2)
    ch:send_line("You now have the following line underneath you when someone sees you in a room as:\n@C%s@n", buf)
    if changing then
        dbat.send_to_imm(string.format(
            "%s has altered their extra description. Make sure the reason is legit! If it is then reimb them 2 RPP.",
            ch:user_get()))
        ch:send_line("The immortals have been notified about this change. It had better have been for a good reason.")
    end
    dbat.log(string.format("%s RPP Feature: '%s' Check for rule compliance.", ch:user_get(), buf))
end

local function execute(ctx)
    local ch   = ctx.ch
    local args = ctx.argparams.tokens
    local arg  = args[1] or ""
    local arg2 = args[2] or ""

    if ch:is_npc() then return end

    local level   = ch:stat_get("level")
    local tnlcost = math.max(1, 1 + math.floor(level / 40))

    -- Display menu when called with no argument.
    if arg == "" then
        ch:send_line("@C                             Rewards Menu")
        ch:send_line("@b  ------------------------------------------------------------------")
        ch:send_line("  @C1@D)@R Disabled            @D[@G -- RPP @D]  @C2@D)@R Disabled              @D[@G -- RPP @D]@n")
        ch:send_line("  @C3@D)@c Custom Equipment    @D[@G 20 RPP @D]  @C4@D)@c Alignment Change      @D[@G 20 RPP @D]")
        ch:send_line("  @C5@D)@c 7,500 zenni         @D[@G  1 RPP @D]  @C6@D)@c +2 To A Stat          @D[@G  2 RPP @D]")
        ch:send_line("  @C7@D)@c +750 PS             @D[@G  4 RPP @D]  @C8@D)@R Disabled               @D[@G -- RPP @D]")
        ch:send_line("  @C9@D)@c 50%% TNL Exp         @D[@G%3d RPP @D] @C10@D)@c Aura Change           @D[@G  2 RPP @D]", tnlcost)
        ch:send_line(" @C11@D)@c Reach Softcap       @D[@G  5 RPP @D] @C12@D)@c RPP Store             @D[@G ?? RPP @D]")
        ch:send_line(" @C13@D)@c Extra Feature       @D[@G  1 RPP @D] @C14@D)@c Restring Equipment    @D[@G  1 RPP @D]")
        ch:send_line(" @C15@D)@c Extra Skillslot     @D[@G  3 RPP @D] @C16@D)@R Disabled              @D[@G -- RPP @D]@n")
        ch:send_line("@b  -----------------------------------------------------------------@n")
        ch:send_line("@D                           [@YYour RPP@D:@G %3d@D]@n", ch:rp_get())
        ch:send_line("\nSyntax: rpp (num)")
        return
    end

    local selection = tonumber(arg) or 0
    if selection <= 0 or selection > 15 then
        ch:send_line("You must choose a number from the menu. Enter the command again with no arguments for the menu.")
        return
    end

    -- Selection 1 and 2: disabled
    if selection == 1 or selection == 2 then
        ch:send_line("That option is currently disabled.")
        return
    end

    -- Selection 3: Custom Equipment
    if selection == 3 then
        if not check_rp(ch, 20) then return end
        ch:rpp_custom_equip_launch()
        return
    end

    -- Selection 4: Alignment Change
    if selection == 4 then
        if not check_rp(ch, 20) then return end
        if arg2 == "" then
            ch:send_line("What do you want to change your alignment to? (evil, sorta-evil, neutral, sorta-good, good)")
            return
        end
        local aligns = {
            ["evil"]       = -750,
            ["sorta-evil"] = -50,
            ["neutral"]    = 0,
            ["sorta-good"] = 51,
            ["good"]       = 300,
        }
        local val = aligns[arg2:lower()]
        if not val then
            ch:send_line("That is not an acceptable option for changing alignment.")
            return
        end
        ch:send_line("You change your alignment to %s.", arg2)
        ch:stat_set("alignment", val)
        rp_purchase(ch, 20)
        return
    end

    -- Selection 5: 7,500 zenni
    if selection == 5 then
        if not check_rp(ch, 1) then return end
        ch:stat_mod("money_bank", 7500)
        ch:send_line("Your bank zenni has been increased by 7,500")
        rp_purchase(ch, 1)
        return
    end

    -- Selection 6: +2 to a stat
    if selection == 6 then
        if not check_rp(ch, 2) then return end
        if arg2 == "" then
            ch:send_line("What stat? (str, con, int, wis, spd, agl)")
            return
        end
        local stat_map = {
            str = { stat = "strength",     bonus = BONUS.WIMP,    cap_field = "strength" },
            con = { stat = "constitution", bonus = BONUS.FRAIL,   cap_field = "constitution" },
            int = { stat = "intelligence", bonus = BONUS.DULL,    cap_field = "intelligence" },
            wis = { stat = "wisdom",       bonus = BONUS.FOOLISH, cap_field = "wisdom" },
            spd = { stat = "speed",        bonus = BONUS.SLOW,    cap_field = "speed" },
            agl = { stat = "agility",      bonus = BONUS.CLUMSY,  cap_field = "agility" },
        }
        local entry = stat_map[arg2:lower()]
        if not entry then
            ch:send_line("That is not an acceptable option for changing alignment.")
            return
        end
        if ch:bonus_flagged(entry.bonus) and ch:stat_get(entry.stat) >= 70 then
            ch:send_line("You can't because that stat maxes at 70 due to a trait negative.")
            return
        end
        if ch:stat_get(entry.stat) >= 100 then
            ch:send_line("100 is the maximum for any stat.")
            return
        end
        ch:send_line("You increase your %s by 2.", entry.stat)
        ch:stat_mod(entry.stat, 2)
        rp_purchase(ch, 2)
        return
    end

    -- Selection 7: +750 practices
    if selection == 7 then
        if not check_rp(ch, 4) then return end
        ch:stat_mod("practices", 750)
        ch:send_line("Your practices have been increased by 750")
        rp_purchase(ch, 4)
        return
    end

    -- Selection 8: disabled
    if selection == 8 then
        ch:send_line("That option is currently disabled.")
        return
    end

    -- Selection 9: 50% TNL experience
    if selection == 9 then
        if not check_rp(ch, tnlcost) then return end
        if level >= 100 then
            ch:send_line("You can not buy experience anymore at your level. I think you know why.")
            return
        end
        local exp_to_next = ch:level_exp(level + 1) - ch:stat_get("experience")
        if exp_to_next < 0 then
            ch:send_line("You can not buy experience anymore UNTIL you level.")
            return
        end
        ch:stat_mod("experience", math.floor(ch:level_exp(level + 1) * 0.52))
        ch:send_line("You gained 50%% of the entire experience needed for your next level.")
        rp_purchase(ch, tnlcost)
        return
    end

    -- Selection 10: Aura Change
    if selection == 10 then
        if not check_rp(ch, 2) then return end
        if arg2 == "" then
            ch:send_line("Change your aura to what? (white, blue, red, green, pink, purple, yellow, black, orange)")
            return
        end
        local aura_map = {
            white  = 0, blue   = 1, red    = 2, green  = 3, pink   = 4,
            purple = 5, yellow = 6, black  = 7, orange = 8,
        }
        local val = aura_map[arg2:lower()]
        if val == nil then
            ch:send_line("That is not an acceptable option for changing aura.")
            return
        end
        ch:aura_set(val)
        ch:send_line("You change your aura to %s.", arg2:lower())
        rp_purchase(ch, 2)
        return
    end

    -- Selection 11: Reach Softcap
    if selection == 11 then
        if not check_rp(ch, 5) then return end
        if level >= 100 then
            ch:send_line("You can't use this at level 100.")
            return
        end
        if ch:race_get() == RACES.ARLIAN then
            ch:send_line("This is not available to bugs.")
            return
        end
        if ch:is_soft_cap(0) and ch:is_soft_cap(1) and ch:is_soft_cap(2) then
            ch:send_line("You are already above your softcap for this level.")
            return
        end
        ch:bring_to_cap()
        rp_purchase(ch, 5)
        return
    end

    -- Selection 12: RPP Store sub-menu
    if selection == 12 then
        if arg2 == "" then
            disp_store(ch)
        else
            local choice = tonumber(arg2) or 0
            if choice <= 0 then
                ch:send_line("That is not a choice in the RPP store!")
                return
            end
            handle_store(ch, choice)
        end
        return
    end

    -- Selection 13: Extra Feature
    if selection == 13 then
        -- Reconstruct full arg2 (may contain spaces) from raw argument string
        local raw_args = ctx.argparams.raw or ""
        local feature_text = raw_args:match("^%S+%s+(.+)$") or arg2
        handle_feature(ch, feature_text)
        return
    end

    -- Selection 14: Restring Equipment
    if selection == 14 then
        if not check_rp(ch, 1) then return end
        if arg2 == "" then
            ch:send_line("You don't have a that equipment to restring in your inventory.")
            ch:send_line("Syntax: rpp 14 (obj name)")
            return
        end
        local obj = Search(ch):add_character_inventory(ch):find_one(arg2)
        if not obj then
            ch:send_line("You don't have a that equipment to restring in your inventory.")
            ch:send_line("Syntax: rpp 14 (obj name)")
            return
        end
        if obj:extra_flagged(EF.CUSTOM) then
            ch:send_line("You can not restring a custom piece. Why? Cause I say so. :P")
            return
        end
        ch:rp_set(ch:rp_get() - 1)
        ch:rp_save()
        ch:rpp_restring_launch(obj)
        return
    end

    -- Selection 15: Extra Skillslot
    if selection == 15 then
        if not check_rp(ch, 3) then return end
        local cap = ch:bonus_flagged(BONUS.GMEMORY) and 65 or 60
        if ch:stat_get("skill_slots") >= cap then
            ch:send_line("You are already at your skillslot cap.")
            return
        end
        ch:stat_mod("skill_slots", 1)
        rp_purchase(ch, 3)
        return
    end
end

return {
    id      = "rpp",
    aliases = { {"rpp", 3} },
    execute = execute,
}
