local dbat   = require("dbat")
local consts = dbat.consts
local PLR    = consts.player_flags
local PRF    = consts.prf_flags
local AFF    = consts.aff_flags
local POS    = consts.positions
local WEAR   = consts.wear_positions
local PREF   = consts.fight_prefs
local DFEA   = consts.distfea
local APPLY  = consts.applies

-- ── Name tables ──────────────────────────────────────────────────────────────

local AURA_NAMES = {
    [0]="white", [1]="blue", [2]="red", [3]="green", [4]="pink",
    [5]="purple", [6]="yellow", [7]="black", [8]="orange",
}

local SONG_NAMES = {
    [1]="Safety", [2]="Shielding", [3]="Shadow Stitch",
    [4]="Teleportation (Earth)", [5]="Teleportation (Konack)",
    [6]="Teleportation (Arlia)", [7]="Teleportation (Namek)",
    [8]="Teleportation (Vegeta)", [9]="Teleportation (Frigid)",
    [10]="Teleportation (Aether)", [11]="Teleportation (Kanassa)",
}

-- Build int → display name for appearance fields using const tables.
local function make_reverse(tbl, overrides)
    local r = {}
    for name, v in pairs(tbl) do r[v] = overrides and overrides[name] or name:lower() end
    return r
end

local HAIRL_NAME = make_reverse(consts.hair_length, {
    BALD="bald", SHORT="short", MEDIUM="medium", LONG="long", RLONG="really long",
})
local HAIRS_NAME = make_reverse(consts.hair_style, {
    NONE="none", PLAIN="plain", MOHAWK="mohawk", SPIKY="spiky", CURLY="curly",
    UNEVEN="uneven", PONYTAIL="pony tail", AFRO="afro", FADE="fade",
    CREW="crew cut", FEATHERED="feathered", DRED="dread locks",
})
local HAIRC_NAME = make_reverse(consts.hair_color, {
    NONE="none", BLACK="black", BROWN="brown", BLONDE="blonde", GREY="grey",
    RED="red", ORANGE="orange", GREEN="green", BLUE="blue", PINK="pink",
    PURPLE="purple", SILVER="silver", CRIMSON="crimson", WHITE="white",
})
local SKIN_NAME = make_reverse(consts.skin_colors, {
    WHITE="white", BLACK="black", GREEN="green", ORANGE="orange", YELLOW="yellow",
    RED="red", GREY="grey", BLUE="blue", AQUA="aqua", PINK="pink", PURPLE="purple",
    TAN="tan",
})
local EYE_NAME = make_reverse(consts.eye_colors, {
    BLUE="blue", BLACK="black", GREEN="green", BROWN="brown", RED="red",
    AQUA="aqua", PINK="pink", PURPLE="purple", CRIMSON="crimson",
    GOLD="gold", AMBER="amber", EMERALD="emerald",
})

-- Build int → race name from the races const table.
local RACE_NAME = {}
for name, v in pairs(consts.races) do RACE_NAME[v] = name:lower() end

-- Non-humanoid races (serpent and animal) — skip appearance section.
local NON_HUMANOID = { [consts.races.ANIMAL]=true, [consts.races.SERPENT]=true }

local function an(word)
    local c = (word or ""):sub(1,1):lower()
    if c=="a" or c=="e" or c=="i" or c=="o" or c=="u" then return "an" end
    return "a"
end

-- ── Section helpers ───────────────────────────────────────────────────────────

local function has_tag(def, tag)
    if not def or not def.tags then return false end
    for _, t in ipairs(def.tags) do
        if t == tag then return true end
    end
    return false
end

-- ── Appearance (bringdesc port) ───────────────────────────────────────────────

local function render_appearance(ch, t)
    local race = ch:race_get()
    -- Skip non-humanoid races.
    local race_id = 0
    for name, v in pairs(consts.races) do
        if name:lower() == race then race_id = v; break end
    end
    if NON_HUMANOID[race_id] then return end

    -- Hair: only races with hair (not demon/icer/namek for the label, but they
    -- still have the fields so we show them with generic label).
    local hairl = ch:hairl_get()
    local hairs = ch:hairs_get()
    local hairc = ch:hairc_get()
    local skin  = ch:skin_get()
    local eye   = ch:eye_get()

    -- Races without conventional hair use a different label.
    local hair_label = "Hair"
    if race == "demon" or race == "icer" then
        hair_label = "Horns"
    elseif race == "majin" then
        hair_label = "Forelock"
    elseif race == "namek" then
        hair_label = "Antennae"
    end

    if hairl and HAIRL_NAME[hairl] and HAIRL_NAME[hairl] ~= "undefined" then
        t[#t+1] = string.format(
            "            @D[@c%-10s @D: @W%-14s@D]@n\r\n",
            hair_label .. " Len", HAIRL_NAME[hairl]:sub(1,1):upper() .. HAIRL_NAME[hairl]:sub(2) .. ".")
    end
    if hairs and HAIRS_NAME[hairs] and HAIRS_NAME[hairs] ~= "undefined" and HAIRS_NAME[hairs] ~= "none" then
        t[#t+1] = string.format(
            "            @D[@c%-10s @D: @W%-14s@D]@n\r\n",
            hair_label .. " Style", HAIRS_NAME[hairs]:sub(1,1):upper() .. HAIRS_NAME[hairs]:sub(2) .. ".")
    end
    if hairc and HAIRC_NAME[hairc] and HAIRC_NAME[hairc] ~= "undefined" and HAIRC_NAME[hairc] ~= "headed" then
        t[#t+1] = string.format(
            "            @D[@c%-10s @D: @W%-14s@D]@n\r\n",
            hair_label .. " Color", HAIRC_NAME[hairc]:sub(1,1):upper() .. HAIRC_NAME[hairc]:sub(2) .. ".")
    end
    if eye and EYE_NAME[eye] and EYE_NAME[eye] ~= "undefined" then
        t[#t+1] = string.format(
            "            @D[@c%-10s @D: @W%-14s@D]@n\r\n",
            "Eye Color", EYE_NAME[eye]:sub(1,1):upper() .. EYE_NAME[eye]:sub(2) .. ".")
    end
    if skin and SKIN_NAME[skin] and SKIN_NAME[skin] ~= "undefined" then
        t[#t+1] = string.format(
            "            @D[@c%-10s @D: @W%-14s@D]@n\r\n",
            "Skin Color", SKIN_NAME[skin]:sub(1,1):upper() .. SKIN_NAME[skin]:sub(2) .. ".")
    end
end

-- ── Appendages ────────────────────────────────────────────────────────────────

local LIMB_LABELS = { "Right Arm", "Left Arm ", "Right Leg", "Left Leg " }
local LIMB_CYBER  = { PLR.CRARM, PLR.CLARM, PLR.CRLEG, PLR.CLLEG }

local function render_appendages(ch, t)
    -- Head
    if ch:player_flagged(PLR.HEAD) then
        t[#t+1] = "            @D[@cHead        @D: @GHave.          @D]@n\r\n"
    else
        t[#t+1] = "            @D[@cHead        @D: @rMissing.         @D]@n\r\n"
    end

    -- Limbs
    for i = 1, 4 do
        local cond  = ch:limbcond_get(i)
        local cyber = ch:player_flagged(LIMB_CYBER[i])
        local lbl   = LIMB_LABELS[i]
        if cond >= 50 and not cyber then
            t[#t+1] = string.format(
                "            @D[@c%s @D: @G%2d%%@D/@g100%%        @D]@n\r\n", lbl, cond)
        elseif cond > 0 and not cyber then
            t[#t+1] = string.format(
                "            @D[@c%s @D: @rBroken @y%2d%%@D/@g100%% @D]@n\r\n", lbl, cond)
        elseif cond > 0 and cyber then
            t[#t+1] = string.format(
                "            @D[@c%s @D: @cCybernetic @G%2d%%@D/@G100%%@D]@n\r\n", lbl, cond)
        else
            t[#t+1] = string.format(
                "            @D[@c%s @D: @rMissing.         @D]@n\r\n", lbl)
        end
    end

    -- Tail
    local race = ch:race_get()
    if race == "saiyan" or race == "halfbreed" then
        if ch:player_flagged(PLR.STAIL) then
            t[#t+1] = "            @D[@cTail        @D: @GHave.            @D]@n\r\n"
        else
            t[#t+1] = "            @D[@cTail        @D: @rMissing.         @D]@n\r\n"
        end
    elseif race == "icer" or race == "bio" then
        if ch:player_flagged(PLR.TAIL) then
            t[#t+1] = "            @D[@cTail        @D: @GHave.            @D]@n\r\n"
        else
            t[#t+1] = "            @D[@cTail        @D: @rMissing.         @D]@n\r\n"
        end
    end
end

-- ── Hunger / Thirst ───────────────────────────────────────────────────────────

local HUNGER_MSGS = {
    {48, "You are full."},
    {40, "You are nearly full."},
    {30, "You are not hungry."},
    {21, "You wouldn't mind a snack."},
    {15, "You are slightly hungry."},
    {10, "You are partially hungry."},
    {5,  "You are really hungry."},
    {2,  "You are extremely hungry."},
    {0,  "You are starving!"},
}
local THIRST_MSGS = {
    {48, "You are not thirsty."},
    {40, "You are nearly quenched."},
    {30, "You are not thirsty."},
    {21, "You wouldn't mind a drink."},
    {15, "You are slightly thirsty."},
    {10, "You are partially thirsty."},
    {5,  "You are really thirsty."},
    {2,  "You are extremely thirsty."},
    {0,  "You are dehydrated!"},
}

local function level_msg(value, msgs, below_zero_msg)
    if value < 0 then return below_zero_msg end
    for _, entry in ipairs(msgs) do
        if value >= entry[1] then return entry[2] end
    end
    return msgs[#msgs][2]
end

-- ── Traits subcommand ─────────────────────────────────────────────────────────

local function show_traits(ch)
    local t = {"@CYour Traits@n\n@D-----------------------------@w\n"}
    local bonuses, flaws = {}, {}
    for _, cid in ipairs(ch:conditions()) do
        local def = dbat.characters.registry.conditions[cid]
        if def then
            if has_tag(def, "bonus") then
                bonuses[#bonuses+1] = { name=def.name, desc=def.description or "(no description)" }
            elseif has_tag(def, "flaw") then
                flaws[#flaws+1] = { name=def.name, desc=def.description or "(no description)" }
            end
        end
    end
    if #bonuses == 0 and #flaws == 0 then
        t[#t+1] = "@wNone.\r\n"
    else
        for _, b in ipairs(bonuses) do
            t[#t+1] = string.format("@c%s@n\n", b.desc)
        end
        if #bonuses > 0 and #flaws > 0 then t[#t+1] = "\r\n" end
        for _, f in ipairs(flaws) do
            t[#t+1] = string.format("@r%s@n\n", f.desc)
        end
    end
    t[#t+1] = "@D-----------------------------@n\r\n"
    ch:send(table.concat(t))
end

-- ── Main execute ─────────────────────────────────────────────────────────────

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if arg == "traits" then
        show_traits(ch)
        return
    elseif arg ~= "" then
        ch:send_line("The only argument status takes is 'traits'. If you just want your status do not use an argument.")
        return
    end

    local t = {}

    -- Header
    t[#t+1] = "@D<@b------------------------@D[@YYour Status@D]@b-------------------------@D>@n\r\n\r\n"

    -- Appearance
    t[#t+1] = "            @D---------------@CAppearance@D---------------\n"
    render_appearance(ch, t)

    -- Appendages
    t[#t+1] = "            @D---------------@RAppendages@D---------------\n"
    render_appendages(ch, t)
    t[#t+1] = "\r\n"

    -- Hunger / Thirst
    t[#t+1] = "         @D-----------------@YHunger@D/@yThirst@D-----------------@n\r\n"
    local hunger = ch:stat_get("hunger")
    t[#t+1] = "         " .. level_msg(hunger, HUNGER_MSGS, "You need not eat.") .. "\r\n"
    local thirst = ch:stat_get("thirst")
    t[#t+1] = "         " .. level_msg(thirst, THIRST_MSGS, "You need not drink.") .. "\r\n"

    -- Info block
    t[#t+1] = "         @D--------------------@D[@GInfo@D]---------------------@n\r\n"

    -- Transformations (iterate conditions by tag "transformation")
    local conds_registry = dbat.characters.registry.conditions
    for _, cid in ipairs(ch:conditions()) do
        local def = conds_registry[cid]
        if def and has_tag(def, "transformation") then
            t[#t+1] = string.format("         You are in %s form.\r\n", def.name or cid)
        end
    end

    t[#t+1] = string.format("         You have died %d times.\r\n", ch:stat_get("death_count"))

    if ch:player_flagged(PLR.NOSHOUT) then
        t[#t+1] = "         You have been @rmuted@n on public channels.\r\n"
    end
    if not ch:pref_flagged(PRF.HINTS) then
        t[#t+1] = "         You have hints turned off.\r\n"
    end
    if ch:news_pending() then
        t[#t+1] = "         Check the 'news', it has been updated recently.\r\n"
    end
    if ch:has_mail() then
        t[#t+1] = "         Check your mail at the nearest postmaster.\r\n"
    end
    if ch:pref_flagged(PRF.HIDE) then
        t[#t+1] = "         You are hidden from who and ooc.\r\n"
    end

    local voice = ch:voice_get()
    if voice then
        t[#t+1] = string.format("         Your voice desc: '%s'\r\n", voice)
    end

    -- Distinguishing feature
    local df = ch:distfea_get()
    if df == DFEA.EYE then
        t[#t+1] = "         Your eyes are your most distinctive feature.\r\n"
    elseif df == DFEA.HAIR then
        local race = ch:race_get()
        if race == "demon" or race == "icer" then
            t[#t+1] = "         Your horns are your most distinctive feature.\r\n"
        elseif race == "majin" then
            t[#t+1] = "         Your forelock is your most distinctive feature.\r\n"
        elseif race == "namek" then
            t[#t+1] = "         Your antennae are your most distinctive feature.\r\n"
        else
            t[#t+1] = "         Your hair is your most distinctive feature.\r\n"
        end
    elseif df == DFEA.SKIN then
        t[#t+1] = "         Your skin is your most distinctive feature.\r\n"
    elseif df == DFEA.HEIGHT then
        t[#t+1] = "         Your height is your most distinctive feature.\r\n"
    elseif df == DFEA.WEIGHT then
        t[#t+1] = "         Your weight is your most distinctive feature.\r\n"
    end

    -- Fight preference
    local pref = ch:preference_get()
    if pref == 0 then
        t[#t+1] = "         You preferred a balanced form of fighting.\r\n"
    elseif pref == PREF.KI then
        t[#t+1] = "         You preferred a ki dominate form of fighting.\r\n"
    elseif pref == PREF.WEAPON then
        t[#t+1] = "         You preferred a weapon dominate form of fighting.\r\n"
    elseif pref == PREF.H2H then
        t[#t+1] = "         You preferred a body dominate form of fighting.\r\n"
    elseif pref == PREF.THROWING then
        t[#t+1] = "         You preferred a throwing dominate form of fighting.\r\n"
    end

    -- Scouter
    local scouter = ch:equipment(WEAR.EYE)
    if scouter then
        local freq = scouter:scoutfreq_get()
        if freq == 0 then freq = 1 end
        t[#t+1] = string.format("         Your scouter is on frequency @G%d@n\r\n", freq)
    end

    -- Ki charge
    local charge = ch:charge_get()
    if charge > 0 then
        t[#t+1] = string.format("         You have @C%s@n ki charged.\r\n",
            dbat.lib.text.add_commas(charge))
    end

    -- Kaioken
    local kaioken = ch:stat_get("kaioken")
    if kaioken > 0 then
        t[#t+1] = string.format("         You are focusing Kaioken x %d.\r\n", kaioken)
    end

    -- Barrier (AFF_SANCTUARY = ki barrier in this codebase)
    if ch:aff_flagged(AFF.SANCTUARY) then
        t[#t+1] = string.format("         You are surrounded by a barrier @D(@Y%s@D)@n\r\n",
            dbat.lib.text.add_commas(ch:barrier_get()))
    end

    -- Fire shield
    if ch:aff_flagged(AFF.FIRESHIELD) then
        t[#t+1] = "         You are surrounded by flames!@n\r\n"
    end

    -- Suppression
    local suppress = ch:stat_get("suppression")
    if suppress > 0 then
        t[#t+1] = string.format("         You are suppressing current PL to %s.\r\n",
            dbat.lib.text.add_commas(suppress))
    end

    -- Absorbs (race-specific)
    local race = ch:race_get()
    if race == "majin" then
        t[#t+1] = string.format("         You have ingested %d people.\r\n", ch:absorbs_get())
    elseif race == "bio" then
        t[#t+1] = string.format("         You have %d absorbs left.\r\n", ch:absorbs_get())
    end

    -- Aura
    local aura = ch:aura_get()
    t[#t+1] = string.format("         You have %s colored aura.\r\n",
        AURA_NAMES[aura] or "unknown")

    -- Soft cap
    local level = ch:stat_get("level")
    if level < 100 then
        if (race == "android" and ch:player_flagged(PLR.ABSORB)) or
           (race ~= "android" and race ~= "bio" and race ~= "majin") then
            t[#t+1] = string.format("         @R%s@n to SC a stat this level.\r\n",
                dbat.lib.text.add_commas(ch:soft_cap()))
        else
            t[#t+1] = string.format("         @R%s@n in PL/KI/ST combined to SC this level.\r\n",
                dbat.lib.text.add_commas(ch:soft_cap()))
        end
    else
        t[#t+1] = "         Your strengths are potentially limitless.\r\n"
    end

    -- Backstab
    if ch:skill_get("dagger") > 0 then
        if ch:backstab_cooldown() > 0 then
            t[#t+1] = "         @yYou can't preform a backstab yet.@n\r\n"
        else
            t[#t+1] = "         @YYou can backstab.@n\r\n"
        end
    end

    -- Extra feature / room display
    local feature = ch:feature_get()
    if feature then
        t[#t+1] = string.format("         Extra Feature: @C%s@n\r\n", feature)
    end
    local rdisplay = ch:rdisplay_get()
    if rdisplay then
        t[#t+1] = string.format("         Room Display: @C...%s@n\r\n", rdisplay)
    end

    -- Mimic
    local mimic_id = ch:mimic_get()
    if mimic_id > 0 then
        local rname = RACE_NAME[mimic_id] or "unknown"
        t[#t+1] = string.format("         You are mimicing the general appearance of %s %s\r\n",
            an(rname), rname)
    end

    -- Mutations (iterate conditions by tag "mutation")
    local mut_lines = {}
    for _, cid in ipairs(ch:conditions()) do
        local def = conds_registry[cid]
        if def and has_tag(def, "mutation") then
            mut_lines[#mut_lines+1] = "  " .. (def.name or cid) .. ".\r\n"
        end
    end
    if #mut_lines > 0 then
        t[#t+1] = "         Your Mutations:\r\n"
        for _, ml in ipairs(mut_lines) do t[#t+1] = ml end
    end

    -- Bio genomes (iterate conditions by tag "bio_genome")
    local genome_lines = {}
    for _, cid in ipairs(ch:conditions()) do
        local def = conds_registry[cid]
        if def and has_tag(def, "bio_genome") then
            genome_lines[#genome_lines+1] = "  " .. (def.name or cid) .. ".\r\n"
        end
    end
    if #genome_lines > 0 then
        t[#t+1] = "         Your genes carry:\r\n"
        for _, gl in ipairs(genome_lines) do t[#t+1] = gl end
    end

    -- Group kills
    if ch:has_group() then
        local kills = 0
        if ch:condition_has("group") then
            kills = ch:condition("group"):number_get("kills")
        end
        t[#t+1] = string.format("         @GGroup Victories@D: @w%s@n\r\n",
            dbat.lib.text.add_commas(kills))
    end

    -- ── Condition block ───────────────────────────────────────────────────────
    t[#t+1] = "\r\n@D<@b-------------------------@D[@BCondition@D]@b----------------------@D>@n\r\n"

    -- Conditions with status_line callbacks
    for _, cid in ipairs(ch:conditions()) do
        local def = conds_registry[cid]
        if def and def.status_line then
            local line = def.status_line(ch, ch:condition(cid))
            if line then t[#t+1] = line .. "\r\n" end
        end
    end

    -- Position
    local pos = ch:position_get()
    if pos == POS.DEAD then
        t[#t+1] = "You are DEAD!\r\n"
    elseif pos == POS.MORTALLYW then
        t[#t+1] = "You are mortally wounded! You should seek help!\r\n"
    elseif pos == POS.INCAP then
        t[#t+1] = "You are incapacitated, slowly fading away...\r\n"
    elseif pos == POS.STUNNED then
        t[#t+1] = "You are stunned! You can't move!\r\n"
    elseif pos == POS.SLEEPING then
        t[#t+1] = "You are sleeping.\r\n"
    elseif pos == POS.RESTING then
        t[#t+1] = "You are resting.\r\n"
    elseif pos == POS.SITTING then
        t[#t+1] = "You are sitting.\r\n"
    elseif pos == POS.STANDING then
        t[#t+1] = "You are standing.\r\n"
    end

    -- Sleep / rest quality (insomniac check via condition)
    if not ch:condition_has("bonus_insomniac") then
        local sleeptime = ch:sleeptime_get()
        if sleeptime > 6 and pos ~= POS.SLEEPING then
            t[#t+1] = "You are well rested.\r\n"
        elseif sleeptime > 6 then
            t[#t+1] = "You are getting the rest you need.\r\n"
        elseif sleeptime > 4 then
            t[#t+1] = "You are rested.\r\n"
        elseif sleeptime > 2 then
            t[#t+1] = "You are not sleepy.\r\n"
        elseif sleeptime >= 1 then
            t[#t+1] = "You are getting a little sleepy.\r\n"
        else
            t[#t+1] = "You could sleep at any time.\r\n"
        end
    end

    -- Relaxation

    -- Eyes closed
    if ch:player_flagged(PLR.EYEC) then
        t[#t+1] = "Your eyes are closed.\r\n"
    end

    -- Sneak
    if ch:aff_flagged(AFF.SNEAK) then
        t[#t+1] = "You are prepared to sneak where ever you go.\r\n"
    end

    -- Disguised
    if ch:player_flagged(PLR.DISGUISED) then
        t[#t+1] = "You have disguised your facial features.\r\n"
    end

    -- Piloting
    if ch:player_flagged(PLR.PILOTING) then
        t[#t+1] = "You are busy piloting a ship.\r\n"
    end

    -- Infuse (legacy AFF flag — not yet a condition)
    if ch:aff_flagged(AFF.INFUSE) then
        t[#t+1] = "Your ki will be infused in your next physical attack.\r\n"
    end

    -- Tail flags
    if ch:player_flagged(PLR.TAILHIDE) then
        t[#t+1] = "Your tail is hidden!\r\n"
    end
    if ch:player_flagged(PLR.NOGROW) then
        t[#t+1] = "Your tail is no longer regrowing!\r\n"
    end

    -- No parry preference
    if ch:pref_flagged(PRF.NOPARRY) then
        t[#t+1] = "You have decided not to parry attacks.\r\n"
    end

    -- Drunk
    local drunk = ch:stat_get("drunk")
    if drunk > 15 then
        t[#t+1] = "You are extremely drunk.\r\n"
    elseif drunk > 10 then
        t[#t+1] = "You are pretty drunk.\r\n"
    elseif drunk > 4 then
        t[#t+1] = "You are drunk.\r\n"
    elseif drunk > 0 then
        t[#t+1] = "You have an alcoholic buzz.\r\n"
    end

    -- Knocked out
    if ch:aff_flagged(AFF.KNOCKED) then
        t[#t+1] = "You have been knocked unconcious!\r\n"
    end

    -- Invisible
    if ch:aff_flagged(AFF.INVISIBLE) then
        t[#t+1] = "You are invisible.\r\n"
    end

    -- Detect invis
    if ch:aff_flagged(AFF.DETECT_INVIS) then
        t[#t+1] = "You are sensitive to the presence of invisible things.\r\n"
    end

    -- Mind break
    if ch:aff_flagged(AFF.MBREAK) then
        t[#t+1] = "Your mind has been broken!\r\n"
    end

    -- Shocked
    if ch:aff_flagged(AFF.SHOCKED) then
        t[#t+1] = "Your mind has been shocked!\r\n"
    end

    -- Charm
    if ch:aff_flagged(AFF.CHARM) then
        t[#t+1] = "You have been charmed!\r\n"
    end

    -- Infravision
    if ch:aff_flagged(AFF.INFRAVISION) then
        t[#t+1] = "You can see in darkness with infravision.\r\n"
    end

    -- Summonable
    if ch:pref_flagged(PRF.SUMMONABLE) then
        t[#t+1] = "You are summonable by other players.\r\n"
    end

    -- Detect align/magic
    if ch:aff_flagged(AFF.DETECT_ALIGN) then
        t[#t+1] = "You see into the hearts of others.\r\n"
    end
    if ch:aff_flagged(AFF.DETECT_MAGIC) then
        t[#t+1] = "You are sensitive to the magical nature of things.\r\n"
    end

    -- Spirit world
    if ch:aff_flagged(AFF.SPIRIT) then
        t[#t+1] = "You have died and are part of the SPIRIT world!\r\n"
    end

    -- No give
    if ch:pref_flagged(PRF.NOGIVE) then
        t[#t+1] = "You are not accepting items being handed to you right now.\r\n"
    end

    -- Ethereal
    if ch:aff_flagged(AFF.ETHEREAL) then
        t[#t+1] = "You are ethereal and cannot interact with normal space!\r\n"
    end

    -- Legacy modifiers
    local regen = ch:legacy_modifier(APPLY.REGEN, -1)
    if regen ~= 0 then
        t[#t+1] = string.format("Something is augmenting your regen rate by %s%d%%!\r\n",
            regen > 0 and "+" or "", regen)
    end
    local train = ch:legacy_modifier(APPLY.TRAIN, -1)
    if train ~= 0 then
        t[#t+1] = string.format("Something is augmenting your auto-skill training rate by %s%d%%!\r\n",
            train > 0 and "+" or "", train)
    end
    local lifemax = ch:legacy_modifier(APPLY.LIFEMAX, -1)
    if lifemax ~= 0 then
        t[#t+1] = string.format("Something is augmenting your Life Force Max by %s%d%%!\r\n",
            lifemax > 0 and "+" or "", lifemax)
    end

    -- Aura light
    if ch:player_flagged(PLR.AURALIGHT) then
        t[#t+1] = "Aura Light is active.\r\n"
    end

    -- Footer
    t[#t+1] = "@D<@b------------------------------------------------------@D>@n\r\n"
    t[#t+1] = "To view your bonus/negative traits enter: status traits\r\n"

    ch:send(table.concat(t))
end

return {
    id      = "status",
    aliases = { { "status", 5 } },
    execute = execute,
}
