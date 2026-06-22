local dbat   = require("dbat")
local Search = require("lua.libs.search").new
local function act() return dbat.lib.act end

local ITEM = dbat.consts.item_flags
local MAT  = dbat.consts.materials
local PREF = dbat.consts.fight_prefs
local PLR  = dbat.consts.player_flags
local MOB  = dbat.consts.mob_flags
local WEAR = dbat.consts.wear_positions
local BON  = dbat.consts.bonuses

-- Weapon damage type offsets from TYPE_HIT=300:
-- TYPE_SLASH=303, TYPE_CRUSH=306, TYPE_BLAST=312, TYPE_PIERCE=311, TYPE_STAB=314
local DAMTYPE_PIERCE = 11
local DAMTYPE_SLASH  = 3
local DAMTYPE_CRUSH  = 6
local DAMTYPE_STAB   = 14
local DAMTYPE_BLAST  = 12

-- Object value array indices
local VAL_HEALTH   = 4
local VAL_MATERIAL = 7
local VAL_DAMTYPE  = 3

local WEAPON_TYPE = dbat.consts.item_types.WEAPON

local function is_sparring(ch)
    if ch:is_npc() then return ch:mob_flagged(MOB.SPAR) end
    return ch:player_flagged(PLR.SPAR)
end

local function find_in_inventory(ch, name)
    return Search(ch):add_character_inventory(ch)
        :add_filter(function(searcher, e) return searcher:can_see(e) end)
        :find_one(name)
end

local function find_in_room(ch, name)
    local room = ch:room_get()
    if not room then return nil end
    return Search(ch):add_room_people(room)
        :add_filter(function(searcher, e)
            return searcher:can_see(e) and not e:is_same(searcher)
        end)
        :find_one(name)
end

local function odam_for(obj)
    local m = obj:value_get(VAL_MATERIAL)
    if     m == MAT.STEEL                   then return math.random(5, 30)
    elseif m == MAT.IRON                    then return math.random(18, 50)
    elseif m == MAT.MITHRIL or m == MAT.KACHIN then return math.random(5, 15)
    elseif m == MAT.STONE                   then return math.random(20, 50)
    elseif m == MAT.DIAMOND                 then return math.random(5, 20)
    elseif m == MAT.ENERGY                  then
        return math.random(1, 2) == 2 and 0 or math.random(1, 3)
    else                                         return math.random(90, 100)
    end
end

local function wtype_for(obj)
    if obj:type_get() ~= WEAPON_TYPE then return 0 end
    local dt = obj:value_get(VAL_DAMTYPE)
    if     dt == DAMTYPE_PIERCE then return 1
    elseif dt == DAMTYPE_SLASH  then return 2
    elseif dt == DAMTYPE_CRUSH  then return 3
    elseif dt == DAMTYPE_STAB   then return 4
    elseif dt == DAMTYPE_BLAST  then return 5
    else                             return 6
    end
end

local function engage_combat(ch, vict)
    if not ch:fighting_get()   then ch:start_fighting(vict) end
    if not vict:fighting_get() then vict:start_fighting(ch) end
end

local HIT_MSGS = {
    [0] = {
        actor  = "You throw $p at $N@n full speed, and watch it smash into $M!",
        room   = "$n@n throws $p at $N@n full speed, and watches it smash into $M!",
        target = "$n@n throws $p at you full speed. You reel as it smashes into your body!",
    },
    [1] = {
        actor  = "You pull out and throw $p at $N@n full speed, and watch it sink into $M!",
        room   = "$n@n pulls out and throws $p at $N@n full speed, and watches it sink into $M!",
        target = "$n@n pulls out and throws $p at you full speed. You reel as it sink into your body!",
    },
    [3] = {
        actor  = "You swing $p overhead and throw it at $N@n full speed, and watch it slam into $M!",
        room   = "$n@n swings $p overhead and throws it at $N@n full speed, and watches it slam into $M!",
        target = "$n@n swings $p overhead and throws it at you full speed. You reel as it slam into your body!",
    },
    [4] = {
        actor  = "You bring $p over your shoulder and throw it at $N@n full speed, and watch it sink into $M!",
        room   = "$n@n brings $p over $s shoulder and throws it at $N@n full speed, and watches it sink into $M!",
        target = "$n@n brings $p over $s shoulder and throws $p at you full speed. You reel as it sink into your body!",
    },
    [5] = {
        actor  = "You pull out and throw $p at $N@n full speed, and watch it hit $M!",
        room   = "$n@n pulls out and throws $p at $N@n full speed, and watches it hit $M!",
        target = "$n@n pulls out and throws $p at you full speed. You reel as it hits your body!",
    },
}
HIT_MSGS[2] = HIT_MSGS[1]  -- slash same as pierce
HIT_MSGS[6] = HIT_MSGS[0]  -- other same as generic

local function throw_object(ch, obj, vict, arg3)
    local room    = ch:room_get()
    local gravity = room and room:gravity_get() or 0
    local max_hp  = ch:meter_max("powerlevel")
    local str     = ch:stat_get("strength") or 1
    local cha     = ch:stat_get("charisma") or 1
    local dex     = ch:stat_get("dexterity") or 1
    local weight  = obj:weight_get()

    if obj:extra_flagged(ITEM.BROKEN) then
        ch:send_line("That is broken and useless to throw!")
        return
    end
    local stamcost = math.floor(max_hp / 200) + weight
    if (ch:meter_current("stamina") or 0) < math.floor(max_hp / 200) + weight then
        ch:send_line("You do not have enough stamina to do it...")
        return
    end
    if weight + gravity > ch:der_total("weight_carry_capacity") then
        ch:send_line("The gravity has made that too heavy for you to throw!")
        return
    end

    local wtype     = wtype_for(obj)
    local odam      = odam_for(obj)
    local multithrow = true
    local penalty   = 0
    local is_throw  = obj:extra_flagged(ITEM.THROW)

    -- Weapon-type objects take minimal durability damage when thrown (non-blast)
    if wtype > 0 and wtype ~= 5 and odam > 1 then
        odam = 1
    end

    -- Base damage
    local damage
    if wtype == 5 then
        damage = math.floor(weight * str * (cha / 3)) + math.floor(max_hp * 0.01)
        damage = damage + gravity * math.floor(gravity / 2)
    else
        damage = math.floor((weight / 3) * str * (cha / 3)) + math.floor(max_hp * 0.01)
        damage = damage + math.floor(damage * 0.01 * (gravity / 4))
    end

    -- Weapon level percentage bonuses
    local wlvl = 0
    if obj:extra_flagged(ITEM.WEAPLVL5) then
        wlvl = 5; damage = math.floor(damage + damage * 0.5)
    elseif obj:extra_flagged(ITEM.WEAPLVL4) then
        wlvl = 4; damage = math.floor(damage + damage * 0.4)
    elseif obj:extra_flagged(ITEM.WEAPLVL3) then
        wlvl = 3; damage = math.floor(damage + damage * 0.3)
    elseif obj:extra_flagged(ITEM.WEAPLVL2) then
        wlvl = 2; damage = math.floor(damage + damage * 0.2)
    elseif obj:extra_flagged(ITEM.WEAPLVL1) then
        wlvl = 1; damage = math.floor(damage + damage * 0.1)
    end

    if not is_throw then
        penalty    = 15
        multithrow = false
        damage     = math.floor(damage * 0.45)
    else
        odam   = math.random(0, 1)
        damage = damage + math.floor(str * (ch:meter_current("powerlevel") * 0.00012 + math.random(1, 20)))
        damage = math.floor(damage + wlvl * (damage * 0.1))
    end

    -- Weapon level flat bonuses (additive after all multipliers)
    if wlvl == 5 then damage = damage + 25000
    elseif wlvl == 4 then damage = damage + 16000
    elseif wlvl == 3 then damage = damage + 10000
    elseif wlvl == 2 then damage = damage + 5000
    elseif wlvl >= 1 then damage = damage + 1000
    end

    -- Hit chance baseline (two rolls)
    local chance = dbat.axion_dice(0) + dbat.axion_dice(0)
    if ch:preference_get() == PREF.THROWING then
        chance = math.floor(chance - chance * 0.25)
    end

    -- Single-throw override from arg3
    if arg3 ~= "" then
        if arg3 == "1" or arg3 == "single" then
            multithrow = false
        else
            ch:send_line("Syntax: throw (obj | character) (target) <-- This will multithrow if able")
            ch:send_line("Syntax: throw (obj) (target) (1 | single) <-- This will not multi throw")
            return
        end
    end

    ch:improve_skill("throw", 1)
    ch:cooldown_set(5)

    local throw_count = math.random(1, 3)
    local perc  = ch:skill_get("throw") or 0
    local perc2 = vict:skill_get("dodge") or 0

    for i = 1, throw_count do
        if (vict:meter_current("powerlevel") or 0) <= 1 then break end

        local prob = dbat.axion_dice(penalty)
        local miss = perc - math.floor(perc2 / 10) < prob

        if miss then
            -- ICE hitting a demon melts the object
            if obj:extra_flagged(ITEM.ICE) then
                local vrace = vict:race_get()
                if vrace == "demon" then
                    act().message({
                        actor  = "You throw $p at $N@n, but it melts before touching $M!",
                        room   = "$n@n throws $p at $N@n, but it melts before touching $M!",
                        target = "$n@n throws $p at you, but it melts before touching you!",
                    }, { actor = ch, target = vict, tool = obj })
                    ch:meter_mod_int("stamina", -(math.floor(max_hp / 100) + weight))
                    obj:from_char()
                    obj:extract()
                    return
                end
            end

            if perc2 > 0 then
                act().message({
                    actor  = "You throw $p at $N@n, but $E manages to dodge it easily!",
                    room   = "$n@n throws $p at $N@n, but $E manages to dodge it easily!",
                    target = "$n@n throws $p at you, but you easily dodge it.",
                }, { actor = ch, target = vict, tool = obj })
            else
                act().message({
                    actor  = "You throw $p at $N@n, but unfortunatly miss!",
                    room   = "$n@n throws $p at $N@n, but unfortunatly misses!",
                    target = "$n@n throws $p at you, but thankfully misses you.",
                }, { actor = ch, target = vict, tool = obj })
            end

            ch:meter_mod_int("stamina", -(math.floor(max_hp / 100) + weight))
            if not obj:extra_flagged(ITEM.UNBREAKABLE) then
                obj:value_mod(VAL_HEALTH, -math.floor(odam / 2))
            end
            engage_combat(ch, vict)
            obj:from_char()
            if vict:room_get() then obj:to_room(vict:room_get()) end
            ch:meter_mod_int("stamina", -(math.floor(max_hp / 200) + weight))

            -- Multi-throw re-roll check
            local no_weapon = not ch:equipment_get(WEAR.WIELD1) and not ch:equipment_get(WEAR.WIELD2)
            if no_weapon then perc = perc + 20 end
            if not (perc + cha >= chance + penalty and multithrow and i < throw_count) then
                break
            end
        else
            -- Hit
            local dmg = damage

            local vnum = obj:vnum_get()
            local vrace = vict:race_get()

            local energize_ok = not ch:is_npc()
                and ch:condition_has("energize")
                and (ch:meter_current("ki") or 0) >= ch:meter_max("ki") * 0.02

            if energize_ok then
                local erg_skill = ch:skill_get("energize") or 0
                dmg = math.floor(dmg + dmg * (0.0016 * erg_skill))
                act().message({
                    actor  = "You charge $p with the energy in your fingertips! As it begins to @Yglow a bright hot @Rred@n you throw $p at $N@n full speed, and watch it smash into $M!",
                    room   = "$n@n charges $p with the energy in $s fingertips! As it begins to @Yglow a bright hot @Rred@n $e throws $p at $N@n full speed, and watches it smash into $M!",
                    target = "$n@n charges $p with the energy in $s fingertips! As it begins to @Yglow a bright hot @Rred@n $e throws $p at YOU@n full speed, and watches it smash into YOU!!",
                }, { actor = ch, target = vict, tool = obj })
                ch:meter_mod_int("ki", -math.floor(ch:meter_max("ki") * 0.02))
                ch:improve_skill("energize", 1)
            else
                act().message(HIT_MSGS[wtype] or HIT_MSGS[0], { actor = ch, target = vict, tool = obj })
            end

            -- Apply durability damage (C++ applies then re-checks with same odam for break)
            if not obj:extra_flagged(ITEM.UNBREAKABLE) then
                obj:value_mod(VAL_HEALTH, -odam)
            end

            -- Check if broken (replicates C++ double-odam check: health-after-apply - odam <= 0)
            if obj:value_get(VAL_HEALTH) - odam <= 0 and not obj:extra_flagged(ITEM.UNBREAKABLE) then
                act().message({
                    actor  = "You smile as $p breaks on $N's@n face!",
                    room   = "$n@n smiles as $p breaks on $N's@n face!",
                    target = "$n@n smiles as $p breaks on your face!",
                }, { actor = ch, target = vict, tool = obj })
                obj:extra_flag_set(ITEM.BROKEN, true)
            elseif dex >= dbat.axion_dice(0) then
                -- Critical hit
                if vrace == "android" or vrace == "mechanical" then
                    local crit_room = ch:room_get()
                    if crit_room then
                        crit_room:send_line("@RSome pieces of metal are sent flying!@n")
                    end
                elseif vrace == "majin" then
                    act().message({
                        actor  = "@RA wide hole is left in $N's gooey flesh!@n",
                        target = "@RA wide hole is left is your gooey flesh!@n",
                        room   = "@RA wide hole is left in $N@R's gooey flesh@n",
                    }, { actor = ch, target = vict })
                else
                    act().message({
                        actor  = "@RBlood flies out from the impact!@n",
                        target = "@RBlood flies out from the impact!@n",
                        room   = "@RBlood flies out from the impact!@n",
                    }, { actor = ch, target = vict })
                end

                -- ICE stamina drain on crit (not android/icer)
                if obj:extra_flagged(ITEM.ICE) and vrace ~= "android" and vrace ~= "icer" then
                    local st_drain = math.floor(vict:meter_max("stamina") * 0.005) + weight
                    vict:meter_mod_int("stamina", -st_drain)
                    act().message({
                        target = "@mYou lose some stamina to the @ccold@m!@n",
                        actor  = "@C$N@m loses some stamina to the @ccold@m!@n",
                        room   = "@C$N@m loses some stamina to the @ccold@m!@n",
                    }, { actor = ch, target = vict })
                end

                dmg = math.floor(dmg * 1.5)
            end

            -- HOT object burn effect
            if obj:extra_flagged(ITEM.HOT) then
                local fireproof = vict:bonus_flagged(BON.FIREPROOF)
                if vrace ~= "demon" and not fireproof then
                    act().message({
                        actor  = "@R$N@R is burned by it!@n",
                        target = "@RYou are burned by it!@n",
                        room   = "@R$N@R is burned by it!@n",
                    }, { actor = ch, target = vict })
                    vict:condition_apply("burned")
                    dmg = math.floor(dmg + dmg * 0.4)
                end
            end

            -- PREFERENCE_KI damage penalty
            if ch:preference_get() == PREF.KI then
                dmg = math.floor(dmg - dmg * 0.20)
            end

            -- Specific object vnums take reduced damage
            if vnum == 5898 or vnum == 5899 then
                dmg = math.floor(dmg * 0.35)
            end

            vict:damage({ powerlevel = math.floor(dmg) }, ch)
            obj:from_char()
            if vict:room_get() then obj:to_room(vict:room_get()) end
            ch:meter_mod_int("stamina", -stamcost)

            -- Multi-throw re-roll check
            local no_weapon = not ch:equipment_get(WEAR.WIELD1) and not ch:equipment_get(WEAR.WIELD2)
            if no_weapon then perc = perc + 12 end
            if not (perc + cha >= chance + penalty
                    and multithrow
                    and i < throw_count
                    and (vict:meter_current("powerlevel") or 0) > 1) then
                break
            end
        end
    end
end

local function throw_person(ch, tch, vict)
    if tch:is_same(vict) then
        ch:send_line("You can't throw someone at theirself.")
        return
    end
    if not tch:is_npc() and tch:player_flagged(PLR.IMMORTAL) then
        ch:send_line("The one you are throwing can't be harmed.")
        return
    end

    local max_hp    = ch:meter_max("powerlevel")
    local str       = ch:stat_get("strength") or 1
    local cha       = ch:stat_get("charisma") or 1
    local room      = ch:room_get()
    local gravity   = room and room:gravity_get() or 0
    local tch_weight = tch:der_total("weight")

    -- Speed-based grab check
    local grab = tch:der_total("speed_index") < ch:der_total("speed_index")
        and math.random(1, 106) < (ch:skill_get("throw") or 0)

    local stamcost_fail = math.floor(max_hp / 100) + tch_weight
    local stamcost_hit  = math.floor(max_hp / 200) + tch_weight

    if (ch:meter_current("stamina") or 0) < stamcost_fail then
        ch:send_line("You do not have enough stamina to do it...")
        return
    end
    if tch_weight + gravity > ch:der_total("weight_carry_capacity") then
        ch:send_line("The gravity has made them too heavy for you to throw!")
        return
    end

    if not grab then
        act().message({
            actor  = "@WYou try to grab @C$N@W and throw them, but they manage to dodge your attempt!@n",
            target = "@C$n@W tries to @RGRAB@W you and @RTHROW@W you, but you manage to dodge the attempt!@n",
            room   = "@C$n@W tries to @RGRAB@W @c$N@W and @RTHROW@W $M, but $E manages to dodge the attempt!@n",
        }, { actor = ch, target = tch })
        engage_combat(ch, tch)
        ch:cooldown_set(5)
        ch:meter_mod_int("stamina", -math.floor(max_hp / 200 + tch_weight))
        return
    end

    ch:cooldown_set(5)
    ch:improve_skill("throw", 1)

    local damage = math.floor((tch_weight * str) * (cha / 3)) + math.floor(max_hp / 100)
    damage = damage + gravity * math.floor(gravity / 2)

    local perc  = ch:skill_get("throw") or 0
    local perc2 = vict:skill_get("dodge") or 0
    local prob  = math.random(1, 106)
    local miss  = perc - math.floor(perc2 / 10) < prob

    -- Throw message (shown regardless of miss/hit)
    act().message({
        actor  = "@WYou grab @C$N@W and spinning around quickly you throw $M!@n",
        target = "@C$n@W grabs YOU and spinning around quickly $e throws you!@n",
        room   = "@C$n@W grabs @c$N@W and spinning around quickly $e throws $M!@n",
    }, { actor = ch, target = tch })

    if miss then
        if perc2 > 0 then
            act().message({
                actor  = "@WThrown through the air, YOU fly at @c$N@W, but $E manages to dodge and you manage recover your bearings a moment later!@n",
                target = "@WThrown through the air, @C$n@W flies at YOU, but you manage to dodge and @C$n@W recovers $s bearings a moment later!@n",
                room   = "@WThrown through the air, @C$n@W flies at @c$N@W, but $E manages to dodge and @C$n@W recovers $s bearings a moment later!@n",
            }, { actor = tch, target = vict })
        else
            act().message({
                actor  = "@WThrown through the air, YOU fly at @c$N@W, but the throw is a miss! You manage recover your bearings a moment later!@n",
                target = "@WThrown through the air, @C$n@W flies at YOU, but the throw is a miss! @C$n@W recovers $s bearings a moment later!@n",
                room   = "@WThrown through the air, @C$n@W flies at @c$N@W, but the throw is a miss! @C$n@W recovers $s bearings a moment later!@n",
            }, { actor = tch, target = vict })
        end
        ch:meter_mod_int("stamina", -stamcost_fail)
        engage_combat(ch, vict)
        engage_combat(ch, tch)
    else
        act().message({
            actor  = "@WThrown through the air, YOU fly at @c$N@W and smash into $M!@n",
            target = "@WThrown through the air, @C$n@W flies at YOU and smashes into YOU!@n",
            room   = "@WThrown through the air, @C$n@W flies at @c$N@W and smashes into $M!@n",
        }, { actor = tch, target = vict })
        vict:damage({ powerlevel = math.floor(damage) }, ch)
        tch:damage({ powerlevel = math.floor(damage) }, ch)
        ch:meter_mod_int("stamina", -stamcost_hit)
    end
end

return {
    id      = "throw",
    aliases = { { "throw", 4 } },

    execute = function(ctx)
        local ch     = ctx.ch
        local tokens = ctx.argparams.tokens
        local arg1   = tokens[1] or ""
        local arg2   = tokens[2] or ""
        local arg3   = tokens[3] or ""

        if arg1 == "" then
            ch:send_line("Throw what?")
            return
        end

        if is_sparring(ch) then
            ch:send_line("You can not spar with throw.")
            return
        end

        if ch:condition_has("mystic_melody") then
            ch:send_line("You are currently playing a song! Enter the song command in order to stop!")
            return
        end

        -- Find object in inventory or character in room
        local obj = find_in_inventory(ch, arg1)
        local tch
        if not obj then
            tch = find_in_room(ch, arg1)
            if not tch then
                ch:send_line("You do not have that object or character to throw!")
                return
            end
        end

        -- Find victim
        local vict
        if arg2 ~= "" then
            vict = find_in_room(ch, arg2)
        end
        if not vict then
            local f   = ch:fighting_get()
            local chr = ch:room_get()
            local fr  = f and f:room_get()
            if f and chr and fr and chr:is_same(fr) then
                vict = f
            end
        end
        if not vict then
            ch:send_line("Who do you want to target?")
            return
        end

        if (vict:meter_current("powerlevel") or 0) <= 1 then return end
        if not ch:is_npc() and not vict:is_npc() and vict:player_flagged(PLR.IMMORTAL) then return end

        if obj then
            throw_object(ch, obj, vict, arg3)
        elseif tch then
            throw_person(ch, tch, vict)
        end
    end,
}
