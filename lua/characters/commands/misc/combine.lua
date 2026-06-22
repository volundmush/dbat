local dbat   = require("dbat")
local Search = require("lua.libs.search").new
local function act() return dbat.lib.act end

local BON  = dbat.consts.bonuses
local WEAR = dbat.consts.wear_positions

-- 14 combine attack variants, 0-indexed externally, stored 1-indexed in this table.
-- name: lowercase match string; display: title-case for messages.
local ATTACKS = {
    -- idx 0
    {
        name    = "kamehameha", display = "Kamehameha", skill = "kamehameha",
        maxki   = 0.15,  attspd = 2,
        bonus_type = "stamina", bonus_factor = 0.02,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! You cup your hands at "..
            "your sides and a ball of @Benergy@W forms there. You chant "..
            "@B'@CKaaaaameeeehaaaameeee@B'@W and then fire a @RKamehameha@W "..
            "wave at @r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W cups $s hands be $s side and chants "..
            "@B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands "..
            "and he quickly brings them forward and fires a @RKamehameha @Wat "..
            "@rYOU@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W cups $s hands be $s side and chants "..
            "@B'@CKaaaameeeehaaaameee@B'@W. A ball of energy forms in $s hands "..
            "and he quickly brings them forward and fires a @RKamehameha @Wat "..
            "@r$N@W while screaming @B'@CHAAAAAAAAAAAAAAAAAAAAA!@B'@n@n",
    },
    -- idx 1
    {
        name    = "galik gun", display = "Galik Gun", skill = "galikgun",
        maxki   = 0.15, attspd = 1,
        bonus_type = "ki", bonus_factor = 0.5,
        msg_actor =
            "@WPositioning yourself in the center of your group you "..
            "call out to your allies to launch a group attack! You "..
            "throw your hands forward and launch a purple beam of "..
            "energy at @r$N@n while shouting @B'@mGalik Gun!@B'@W",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W throws $s arms forward and launches a purple beam "..
            "at @rYOU@W while shouting @B'@mGalik Gun!@B'@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W throws $s arms forward and launches a purple beam "..
            "at @r$N@W while shouting @B'@mGalik Gun!@B'@n",
    },
    -- idx 2
    {
        name    = "masenko", display = "Masenko", skill = "masenko",
        maxki   = 0.15, attspd = 1,
        bonus_type = "ki", bonus_factor = 0.5,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! You raise your hands "..
            "above your head with one resting atop the other and begin to pour "..
            "your charged energy to that point. As soon as the energy is ready "..
            "you shout @B'@RMasenko Ha!@B'@W and bringing your hands down you "..
            "launch a bright reddish orange beam at @r$N@W!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W raises $s hands above $s head and energy quicly pools "..
            "there. Suddenly $e brings $s hands down and shouts @B'@RMasenko "..
            "Ha!@B'@W as a bright reddish orange beam launches toward @rYOU!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W raises $s hands above $s head and energy quicly pools "..
            "there. Suddenly $e brings $s hands down and shouts @B'@RMasenko "..
            "Ha!@B'@W as a bright reddish orange beam launches toward @r$N!@n",
    },
    -- idx 3
    {
        name    = "deathbeam", display = "Deathbeam", skill = "deathbeam",
        maxki   = 0.10, attspd = 4,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! With a quick motion you "..
            "point at @r$N@W and launch a lightning fast @MDeathbeam@W!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! With a quick motion $e points $s finger at @rYOU@W and "..
            "launches a lightning fast @MDeathbeam@W!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack "..
            "against @r$N@W! With a quick motion $e points $s finger at "..
            "@r$N@W and launches a lightning fast @MDeathbeam@W!@n",
    },
    -- idx 4
    {
        name    = "honoo", display = "Honoo", skill = "honoo",
        maxki   = 0.125, attspd = 2,
        burn    = true,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! With your energy ready "..
            "you breath out toward @r$N@W jets of incredibly hot flames in the "..
            "form of a deadly @rHonoo@W!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! Sudden jets of flame burst forth from $s mouth at "..
            "@RYOU@W in the form of a deadly @rHonoo@W!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack "..
            "against @r$N@W! Sudden jets of flame burst forth from $s "..
            "mouth at @R$N@W in the form of a deadly @rHonoo@W!@n",
    },
    -- idx 5
    {
        name      = "twin slash", display = "Twin Slash", skill = "twinslash",
        maxki     = 0.125, attspd = 2,
        sword_req = true, unblockable = true,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out to "..
            "your allies to launch a group attack! With your energy prepared you "..
            "poor it into your blade and accelerate your body to incredible speeds "..
            "toward @r$N! You leave two glowing green marks behind on $S body in a "..
            "single instant as your @GTwin Slash@W hits!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! Raising $s sword @Y$n@W accelerates toward @rYOU@W with "..
            "incredible speed! Two glowing green slashes are left on YOUR body "..
            "from $s successful @GTwin Slash@W!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! Raising $s sword @Y$n@W accelerates toward @r$N@W with "..
            "incredible speed! Two glowing green slashes are left on @R$N's@W "..
            "body from the successful @GTwin Slash@W!@n",
    },
    -- idx 6
    {
        name    = "hell flash", display = "Hell Flash", skill = "hellflash",
        maxki   = 0.20, attspd = 1,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! You stick one of each of "..
            "your hands in your armpits and detach them. With your hands "..
            "detached your point the exposed arm cannons at @r$N@W and launch "..
            "a massive @RHell Flash@W at $M!",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W sticks one of each of $s hands in $s armpits and "..
            "detaches them there. With the hands detached $e aims $s exposed "..
            "arm cannons at @RYOU@W and launches a massive @RHell Flash@W!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W sticks one of each of $s hands in $s armpits and "..
            "detaches them there. With the hands detached $e aims $s exposed "..
            "arm cannons at @R$N@W and launches a massive @RHell Flash@W!@n",
    },
    -- idx 7
    {
        name     = "psychic blast", display = "Psychic Blast", skill = "psyblast",
        maxki    = 0.125, attspd = 1,
        shocked  = true,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! With your energy ready "..
            "you look at @R$N@W as the blue light of your @CPsychic Blast@W "..
            "launches from your head toward $S!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack "..
            "against @rYOU@W! A blue light, identifying a @CPsychic "..
            "Blast@W, launches from @Y$n's@W toward @RYOUR HEAD@W!",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack "..
            "against @r$N@W! A blue light, identifying a @CPsychic "..
            "Blast@W, launches from @Y$n's@W toward @R$N's@W head!",
    },
    -- idx 8
    {
        name    = "crusher ball", display = "Crusher Ball", skill = "crusher",
        maxki   = 0.20, attspd = 0,
        msg_actor =
            "@WPositioning yourself in the center of your group you "..
            "call out to your allies to launch a group attack! Pooling "..
            "your energy you form a large ball of red energy above an "..
            "upraised palm. Slamming your other hand into it you launch "..
            "it toward @r$N@W while shouting @B'@RCrusher Ball@B'@W!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W raises a palm above his head and red energy "..
            "begins to pool there. As the energy completes the formation of a "..
            "ball @Y$n@W slams $s other hand into it and launches it at "..
            "@rYOU@W while shouting @B'@RCrusher Ball@B'@W!",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W raises a palm above his head and red energy begins "..
            "to pool there. As the energy completes the formation of a ball "..
            "@Y$n@W slams $s other hand into it and launches it at @r$N@W "..
            "while shouting @B'@RCrusher Ball@B'@W!",
    },
    -- idx 9
    {
        name    = "water spikes", display = "Water Spikes", skill = "waterspikes",
        maxki   = 0.14, attspd = 0,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! Using your energy to "..
            "form a ball of water between your hands you then raise the ball "..
            "above your head. Several spiked of ice form from the ball of "..
            "water and you hurl them at @r$N@W!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! Forming a ball of water between $s palms with $s energy "..
            "@Y$n@W then raises the ball of water above $s head. Suddenly several "..
            "spikes of ice form from the water and $e launches them at @rYOU@W!",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! Forming a ball of water between $s palms with $s energy "..
            "@Y$n@W then raises the ball of water above $s head. Suddenly several "..
            "spikes of ice form from the water and $e launches them at @r$N@W!",
    },
    -- idx 10
    {
        name    = "tribeam", display = "Tribeam", skill = "tribeam",
        maxki   = 0.20, attspd = 2,
        bonus_type = "powerlevel", bonus_factor = 0.5,
        msg_actor =
            "@WPositioning yourself in the center of your group you call out "..
            "to your allies to launch a group attack! You form a triangle with "..
            "your hands and aim the center of the triangle at @r$N@W. With the "..
            "sudden shout @B'@YTribeam@B'@W you release your prepared energy "..
            "at $M in the form of a beam!",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W forms a triangle with $s hands and aims the "..
            "center at @rYOU@W! With the sudden shout @B'@YTribeam@B'@W a "..
            "large beam of energy flashes toward @rYOU!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W forms a triangle with $s hands and aims the center "..
            "at @r$N@W! With the sudden shout @B'@YTribeam@B'@W a large beam "..
            "of energy flashes toward @r$N!@n",
    },
    -- idx 11
    {
        name    = "star breaker", display = "Star Breaker", skill = "starbreaker",
        maxki   = 0.20, attspd = 0,
        bonus_type = "ki", bonus_factor = 0.6,
        msg_actor =
            "@WPositioning yourself in the center of your group, you call out to "..
            "your allies to launch a group attack! You raise your right hand above "..
            "your head as dark red energy begins to pool in your slightly cupped "..
            "hand, while purple arcs of electricity flow up your left arm. "..
            "Slamming both hands together, you shout @B'@YStarbreaker@B'@W and "..
            "release your prepared energy at $M in the form of a ball!@n",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W raises $s right hand, pooling dark red energy in "..
            "the palm. @Y$n@W slams both their hands together, shouting "..
            "@B'@YStarbreaker@B'@W, a ball of energy flashes toward @rYOU!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W raises their right hand above their head, pooling "..
            "dark red energy. @Y$n@W slams both their hands together, shouting "..
            "@B'@YStarbreaker@B'@W, a ball of energy flashes toward @r$N!@n",
    },
    -- idx 12
    {
        name    = "seishou enko", display = "Seishou Enko", skill = "seishou",
        maxki   = 0.125, attspd = 35,
        msg_actor =
            "@WPositioning yourself in the center of your group you "..
            "call out to your allies to launch a group attack! You open "..
            "your mouth and aim at @r$N@W. You grunt as you release "..
            "your prepared energy at $M in the form of a beam!",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W opens $s mouth and aims at @rYOUW! With the "..
            "sudden grunt, a large beam flashes towards @rYOU@n!",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@r$N@W! @Y$n@W opens $s mouth and aims at @rYOUW! With the sudden "..
            "grunt, a large beam flashes toward @r$N@n!",
    },
    -- idx 13
    {
        name      = "renzokou energy dan", display = "Renzokou Energy Dan", skill = "renzo",
        maxki     = 0.125, attspd = 6,
        nail_only = true,
        msg_actor =
            "@WPositioning yourself in the center of your group you "..
            "call out to your allies to launch a group attack! You slam "..
            "your hands together and aim at @r$n@W. With the sudden "..
            "shout @B'@YRenzoku Energy Dan@B'@W you release your "..
            "prepared energy in the form of hundreds of ki blasts!",
        msg_target =
            "@Y$n@W calls out to $s allies to launch a group attack against "..
            "@rYOU@W! @Y$n@W slams both $s hands together and aims at @rYOUW! "..
            "With the sudden shout @B'@YRenzoku Energy Dan@B'@W hundreds of ki "..
            "blasts flash towards @rYOU!@n",
        msg_room =
            "@Y$n@W calls out to $s allies to launch a group attack "..
            "against @r$N@W! @Y$n@W slams $s hands together and aims at "..
            "@r$N@W! With the sudden shout @B'@YRenzoku Energy Dan@B'@W "..
            "hundreds of ki blasts flash towards @r$N!@n",
    },
}

local function is_group_ally(ch, person)
    if not person:condition_has("group") then return false end
    local pl = person:following_get()
    local cl = ch:following_get()
    return (pl and pl:is_same(ch))
        or (cl and cl:is_same(person))
        or (pl and cl and pl:is_same(cl))
end

local function group_members_in_room(ch)
    local room   = ch:room_get()
    local result = {}
    if not room then return result end
    for person in room:people() do
        if not person:is_same(ch) and is_group_ally(ch, person) then
            result[#result + 1] = person
        end
    end
    return result
end

-- Match attack name as substring of raw input; return (att, 0-based-idx, target_str).
local function find_attack(raw)
    for i, att in ipairs(ATTACKS) do
        local pos, fin = string.find(raw, att.name, 1, true)
        if pos then
            local rest = string.match(string.sub(raw, fin + 1), "^%s*(.*)")
            return att, i - 1, rest or ""
        end
    end
    return nil, nil, ""
end

local function fire_combine(ch, vict, leader_idx, att, ready_followers)
    local a = act()

    a.message({
        actor  = att.msg_actor,
        target = att.msg_target,
        room   = att.msg_room,
    }, { actor = ch, target = vict })

    -- Drain leader's charge
    local ki_max   = ch:meter_max("ki")
    local drain    = math.min(ch:charge_get(), math.floor(ki_max * att.maxki))
    local totki    = drain
    ch:charge_set(ch:charge_get() - drain)

    -- Accumulate ready followers
    local totalmem = 1
    local attavg   = ch:skill_get(att.skill)
    local same     = true

    for _, fol in ipairs(ready_followers) do
        local fol_cond = fol:condition("combine_ready")
        local fol_idx  = fol_cond and math.floor(fol_cond:number_get("attack_index") or 0) or 0
        local fol_att  = ATTACKS[fol_idx + 1] or att
        if fol_idx ~= leader_idx then same = false end

        local fol_ki_max = fol:meter_max("ki")
        local fol_drain  = math.min(fol:charge_get(), math.floor(fol_ki_max * att.maxki))
        totki  = totki + fol_drain
        fol:charge_set(fol:charge_get() - fol_drain)
        totalmem = totalmem + 1
        attavg   = attavg + fol:skill_get(fol_att.skill)

        a.message({
            actor = "@WYou time and merge your @B'@R" .. fol_att.display ..
                    "@B'@W into the group attack!@n",
            room  = "@Y$n@W times and merges $s @B'@R" .. fol_att.display ..
                    "@B'@W into the group attack!@n",
        }, { actor = fol })

        fol:condition_remove("combine_ready", "fired")
    end

    -- Bonus from leader's stats
    local bonus = 0
    if att.bonus_type == "stamina" then
        bonus = math.floor(ch:meter_max("stamina") * att.bonus_factor)
    elseif att.bonus_type == "ki" then
        bonus = math.floor(ch:meter_max("ki") * att.bonus_factor)
    elseif att.bonus_type == "powerlevel" then
        bonus = math.floor(ch:meter_max("powerlevel") * att.bonus_factor)
    end

    totki = totki + bonus
    if same then totki = totki + bonus end  -- synergy doubles bonus

    local attsk = math.floor(attavg / totalmem)

    -- Defense (skipped entirely for twin slash, idx 5)
    if leader_idx ~= 5 then
        if att.attspd + attsk < vict:skill_get("dodge") + math.floor((ch:stat_get("charisma") or 0) / 10) then
            vict:send_line("@GYou manage to dodge nimbly through the combined attack of your enemies!@n")
            a.message({ room = "@r$n@G manages to dodge nimbly through the combined attack!@n" }, { actor = vict })
            return
        end
        if not att.unblockable and
           att.attspd + attsk < vict:skill_get("block") + math.floor((ch:stat_get("strength") or 0) / 10) then
            vict:send_line("@GYou manage to effectivly block the combined attack of your enemies with the help of your great strength!@n")
            a.message({ room = "@r$n@G manages to dodge nimbly through the combined attack!@n" }, { actor = vict })
            return
        end
    end

    -- Burn effect (honoo)
    if att.burn then
        local fireproof = vict:bonus_flagged(BON.FIREPROOF)
        local is_demon  = vict:race_get() == "demon"
        if fireproof or is_demon then
            ch:send_line("@RThey appear to be fireproof!@n")
        elseif vict:bonus_flagged(BON.FIREPRONE) then
            vict:send_line("@RYou are extremely flammable and are burned by the attack!@n")
            ch:send_line("@RThey are easily burned!@n")
            vict:condition_apply("burned")
        elseif not vict:condition_has("burned") and math.random(1, 4) == 3 then
            vict:send_line("@RYou are burned by the attack!@n")
            ch:send_line("@RThey are burned by the attack!@n")
            vict:condition_apply("burned")
        end
    end

    -- Shock effect (psychic blast)
    if att.shocked then
        if not vict:condition_has("shocked") and math.random(1, 4) == 4
           and not vict:condition_has("barrier") then
            vict:send_line("@MYour mind has been shocked!@n")
            a.message({ room = "@M$n@m's mind has been shocked!@n" }, { actor = vict })
            vict:condition_apply("shocked")
        end
    end

    -- Damage
    vict:damage({ powerlevel = totki }, ch)

    -- Synergy notification
    if same then
        ch:send_line("@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n")
        for _, fol in ipairs(ready_followers) do
            fol:send_line("@YS@yy@Yn@ye@Yr@yg@Yi@ys@Yt@yi@Yc @yB@Yo@yn@Yu@ys@Y!@n")
        end
    end
end

return {
    id      = "combine",
    aliases = { { "combine", 3 } },

    execute = function(ctx)
        local ch  = ctx.actor
        local raw = string.lower(ctx.argparams.raw or "")

        if not ch:has_group() then
            ch:send_line("You need to be in a group!")
            return
        end

        -- "combine stop"
        if raw == "stop" or string.sub(raw, 1, 5) == "stop " then
            if ch:following_get() then
                -- follower cancels their prepared attack
                if ch:condition_has("combine_ready") then
                    ch:send_line("You stop your preparations to combine your attack with a group attack.")
                    ch:condition_remove("combine_ready", "cancelled")
                    for _, member in ipairs(group_members_in_room(ch)) do
                        member:send_line("@BCOMBINE@c: @Y" .. ch:display_name_for(member) ..
                            "@C is no longer prepared to combine an attack with the group!@n")
                    end
                else
                    ch:send_line("You are not trying to combine any attacks...")
                end
            else
                -- leader has no persistent prepared state
                ch:send_line("You do not need to stop as you haven't prepared anything.")
            end
            return
        end

        -- Show syntax if nothing typed
        if raw == "" then
            ch:send_line("Follower Syntax: combine (attack)")
            ch:send_line("Leader Syntax: combine (attack) (target)")
            ch:send_line("Cancel Syntax: combine stop")
            return
        end

        local att, found_idx, target_str = find_attack(raw)

        if not att then
            ch:send_line("Follower Syntax: combine (attack)")
            ch:send_line("Leader Syntax: combine (attack) (target)")
            ch:send_line("Follower Cancel Syntax: combine stop")
            return
        end

        -- Skill check
        if ch:skill_get(att.skill) == 0 then
            ch:send_line("You do not know that skill.")
            return
        end

        -- Nail-only restriction
        if att.nail_only and ch:sensei_get() ~= "nail" then
            ch:send_line("Only students of Nail know how to combine that attack effectively.")
            return
        end

        -- Sword requirement (twin slash)
        if att.sword_req then
            local wield = ch:equipment_get(WEAR.WIELD1)
            if not wield then
                ch:send_line("You need to wield a sword to use this technique.")
                return
            end
            if wield:value_get(3) ~= 3 then  -- VAL_WEAPON_DAMTYPE, DAMTYPE_SLASH=3
                ch:send_line("You are not wielding a sword, you need one to use this technique.")
                return
            end
        end

        local is_leader = not ch:following_get()

        if is_leader then
            -- Leader fires the combined attack
            if target_str == "" then
                ch:send_line("Who will your combined attack be targeting?")
                return
            end

            local room = ch:room_get()
            if not room then return end

            local vict = Search(ch):add_room_people(room):add_filter(function(p)
                return not p:is_same(ch) and ch:can_see(p)
            end):find_one(target_str)

            if not vict then
                ch:send_line("Who will your combined attack be targeting?")
                return
            end
            if vict:is_same(ch) then
                ch:send_line("No targeting yourself...")
                return
            end

            -- Find ready followers (combine_ready condition + enough charge)
            local ready = {}
            for _, member in ipairs(group_members_in_room(ch)) do
                if member:condition_has("combine_ready") and
                   member:charge_get() >= member:meter_max("ki") * 0.05 then
                    ready[#ready + 1] = member
                end
            end

            if #ready == 0 then
                ch:send_line("You do not have any followers who have readied an attack to combine or they do not have enough ki anymore to combine said attack.")
                return
            end

            fire_combine(ch, vict, found_idx, att, ready)
        else
            -- Follower prepares their contribution
            if ch:charge_get() < ch:meter_max("ki") * 0.05 then
                ch:send_line("You do not have the minimum 5% ki charged.")
                return
            end

            ch:condition_apply_number("combine_ready", "attack_index", found_idx,
                                      "command", "combine")

            local a = act()
            a.message({
                room = "@C$n@c appears to be concentrating hard and focusing $s energy!@n",
            }, { actor = ch })

            for _, member in ipairs(group_members_in_room(ch)) do
                member:send_line("@BCOMBINE@c: @Y" .. ch:display_name_for(member) ..
                    "@C has prepared to combine a @c'@G" .. att.name ..
                    "@c'@C with the next group attack!@n")
            end
        end
    end,
}
