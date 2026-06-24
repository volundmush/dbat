local dbat     = require("dbat")
local act      = require("lua.libs.act")
local commands = require("lua.characters.commands")
local combat   = require("lua.libs.combat")

local AFF = dbat.consts.aff_flags
local MF  = dbat.consts.mob_flags
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local FIGHT_INTERVAL_MS = 5000

local function roll_balance(ch)
    if ch:is_npc() then
        local lvl = ch:stat_get("level") or 0
        if     lvl >= 100 then return math.random(80, 100)
        elseif lvl >= 80  then return math.random(75, 90)
        elseif lvl >= 70  then return math.random(70, 80)
        elseif lvl >= 60  then return math.random(65, 75)
        elseif lvl >= 50  then return math.random(50, 60)
        else return 0 end
    else
        local bal = ch:skill_get("balance") or 0
        return bal > 50 and bal or 0
    end
end

local function sec_roll_check(ch)
    local figure = 10 + math.floor((ch:stat_get("level") or 0) * 1.6)
    local chance = dbat.axion_dice(0) + dbat.axion_dice(0) + math.random(0, 20)
    return figure >= chance and 1 or 0
end

-- ── NPC mob_attack helpers ────────────────────────────────────────────────

local function pick_n_throw(ch, vict_name)
    if math.random(1, 20) < 18 then return false end
    local room = ch:room_get()
    for obj in room:contents_get() do
        if obj:weight_get() <= ch:carry_weight_max() then
            local oname = obj:name_get()
            ch:execute_command("get " .. oname, commands)
            ch:execute_command("throw " .. oname .. " " .. vict_name, commands)
            return true
        end
    end
    return false
end

local function mob_attack(ch, cond, vict_name)
    local power = math.random(1, 5)
    local bonus = math.floor((ch:stat_get("level") or 0) * 0.1)
    local special = 0

    power = power + bonus
    if math.random(1, 4) == 4 then power = power + 10 end
    if power > 20 then power = 20 end

    local race = ch:race_get()
    local dragonpass = true
    if race == "dragon" then
        local vnum = ch:vnum_get()
        if vnum >= 81 and vnum <= 87 then
            dragonpass = true
            special = math.random(40, 100)
        else
            dragonpass = false
        end
    end

    local pl_max = ch:meter_max("powerlevel")
    local pl_pct = pl_max > 0 and (ch:meter_current("powerlevel") / pl_max) or 1
    if dbat.axion_dice(-10) > 90 and pl_pct <= 0.5 and
       not ch:mob_flagged(MF.POWERUP) and ch:vnum_get() ~= 25 and
       race ~= "android" and race ~= "animal" and ch:sensei_get() ~= "commoner" then
        ch:execute_command("powerup", commands)
        return
    end

    local ki_max = ch:meter_max("ki")
    local mobcharge = math.floor(cond:number_get("mobcharge") or 0)
    if ch:meter_current("ki") >= ki_max * 0.05 and ch:is_humanoid() and
       (race ~= "dragon" or dragonpass) then
        local lvl = ch:stat_get("level") or 0
        local function mob_charge_tick()
            mobcharge = mobcharge + 1
            if lvl > 80 then mobcharge = mobcharge + 1 end
            cond:number_set("mobcharge", mobcharge)
        end
        if mobcharge <= 0 and math.random(1, 10) >= 8 then
            act.around(ch, "@wAn aura flares up around @R$n@w!@n", {actor=ch})
            mob_charge_tick()
        elseif mobcharge <= 5 then
            act.around(ch, "@wThe aura burns brighter around @R$n@w!@n", {actor=ch})
            mob_charge_tick()
        elseif mobcharge == 6 then
            act.around(ch, "@wThe aura around @R$n@w flashes!@n", {actor=ch})
            mobcharge = mobcharge + 1
            cond:number_set("mobcharge", mobcharge)
            special = 100
        end
    end

    if ch:is_humanoid() and dragonpass then
        if ch:aff_flagged(AFF.PARALYZE) or ch:aff_flagged(AFF.ENSNARED) then return end

        local sensei = ch:sensei_get()
        if special < 100 then
            if sensei == "shadowdancer" and math.random(1, 3) == 3 then
                ch:execute_command("throw ass " .. vict_name, commands)
            elseif race == "android" and ch:mob_flagged(MF.REPAIR) and
                   pl_pct <= 0.5 and math.random(1, 20) >= 16 then
                ch:execute_command("repair", commands)
            elseif race == "android" and ch:mob_flagged(MF.ABSORB) and
                   math.random(1, 20) >= 19 then
                ch:execute_command("absorb " .. vict_name, commands)
            elseif (race == "bio" or race == "majin") and
                   pl_pct <= 0.5 and math.random(1, 20) >= 17 then
                ch:execute_command("regenerate 25", commands)
            elseif race == "namekian" and pl_pct <= 0.5 and math.random(1, 20) == 20 then
                ch:execute_command("regenerate 25", commands)
            elseif pick_n_throw(ch, vict_name) then
                -- handled
            elseif ch:mob_flagged(MF.KNOWKAIO) and math.random(1, 50) >= 46 then
                if     math.random(1, 10) == 10 then ch:execute_command("kaioken 20", commands)
                elseif math.random(1, 10) >= 8  then ch:execute_command("kaioken 10", commands)
                else                                  ch:execute_command("kaioken 5",  commands)
                end
            else
                local has_weapon = ch:wielded_weapon_type() ~= nil
                if power <= 5 then
                    if has_weapon then ch:execute_command("attack " .. vict_name, commands)
                    elseif math.random(1, 5) == 5  then ch:execute_command("kick "     .. vict_name, commands)
                    elseif math.random(1, 10) == 10 then ch:execute_command("elbow "   .. vict_name, commands)
                    else                                  ch:execute_command("punch "   .. vict_name, commands)
                    end
                elseif power <= 8 then
                    if has_weapon then ch:execute_command("attack " .. vict_name, commands)
                    elseif math.random(1, 5) == 5  then ch:execute_command("punch "    .. vict_name, commands)
                    elseif math.random(1, 10) == 10 then ch:execute_command("knee "    .. vict_name, commands)
                    else                                  ch:execute_command("kick "    .. vict_name, commands)
                    end
                elseif power <= 10 then
                    if     math.random(1, 5) == 5  then ch:execute_command("knee "     .. vict_name, commands)
                    elseif math.random(1, 10) == 10 then ch:execute_command("uppercut " .. vict_name, commands)
                    else                                  ch:execute_command("elbow "   .. vict_name, commands)
                    end
                elseif power <= 12 then
                    if     math.random(1, 5) == 5  then ch:execute_command("elbow "     .. vict_name, commands)
                    elseif math.random(1, 10) == 10 then ch:execute_command("roundhouse " .. vict_name, commands)
                    elseif math.random(1, 8) == 8  then ch:execute_command("trip "      .. vict_name, commands)
                    else                                  ch:execute_command("knee "     .. vict_name, commands)
                    end
                elseif power <= 14 then
                    if (sensei == "bardock" or sensei == "kurzak") and math.random(1, 2) == 2 then
                        ch:execute_command("headbutt " .. vict_name, commands)
                    elseif (race == "icer" or race == "bio") and math.random(1, 2) == 2 then
                        ch:execute_command("tailwhip " .. vict_name, commands)
                    elseif math.random(1, 8) == 8 then
                        ch:execute_command("trip " .. vict_name, commands)
                    else
                        ch:execute_command("uppercut " .. vict_name, commands)
                    end
                elseif power <= 16 then
                    if (sensei == "bardock" or sensei == "kurzak") and math.random(1, 2) == 2 then
                        ch:execute_command("headbutt " .. vict_name, commands)
                    elseif (race == "icer" or race == "bio") and math.random(1, 2) == 2 then
                        ch:execute_command("tailwhip " .. vict_name, commands)
                    elseif math.random(1, 8) >= 7 then
                        ch:execute_command("trip " .. vict_name, commands)
                    else
                        ch:execute_command("roundhouse " .. vict_name, commands)
                    end
                elseif power <= 18 then
                    ch:execute_command("slam " .. vict_name, commands)
                else
                    ch:execute_command("heeldrop " .. vict_name, commands)
                end
            end
        else
            -- charged special attacks
            local function fire_charged(fn)
                if mobcharge == 7 then
                    cond:number_set("mobcharge", 0)
                    fn()
                end
            end
            local function dragon_or_charged(fn)
                if race == "dragon" and math.random(1, 4) == 4 then
                    ch:execute_command("breath " .. vict_name, commands)
                else
                    fire_charged(fn)
                end
            end

            if power <= 4 then
                if special > 80 then ch:execute_command("zanzoken", commands) end
                fire_charged(function() ch:execute_command("kiball " .. vict_name, commands) end)
            elseif power <= 8 then
                if special > 80 then ch:execute_command("zanzoken", commands) end
                fire_charged(function() ch:execute_command("kiblast " .. vict_name, commands) end)
            elseif power <= 11 then
                if special > 80 then ch:execute_command("zanzoken", commands) end
                dragon_or_charged(function() ch:execute_command("beam " .. vict_name, commands) end)
            elseif power <= 14 then
                if special > 80 then ch:execute_command("zanzoken", commands) end
                dragon_or_charged(function() ch:execute_command("renzokou " .. vict_name, commands) end)
            elseif power <= 16 then
                dragon_or_charged(function() ch:execute_command("tsuihidan " .. vict_name, commands) end)
            elseif power <= 18 then
                dragon_or_charged(function() ch:execute_command("shogekiha " .. vict_name, commands) end)
            else
                if race == "dragon" then ch:execute_command("breath " .. vict_name, commands) end
                fire_charged(function()
                    if     sensei == "roshi"   then
                        if     special >= 100 then ch:execute_command("kakusanha "   .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("kienzan "     .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("kamehameha "  .. vict_name, commands)
                        elseif special >= 50  then ch:execute_command("barrier 40",                commands)
                        else                       ch:execute_command("barrier 25",                commands)
                        end
                    elseif sensei == "frieza"  then
                        if     special >= 100 then ch:execute_command("deathball "   .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("kienzan "     .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("deathbeam "   .. vict_name, commands)
                        elseif special >= 50  then ch:execute_command("barrier 40",                commands)
                        else                       ch:execute_command("barrier 25",                commands)
                        end
                    elseif sensei == "krane"   then
                        if     special >= 100 then ch:execute_command("tribeam "     .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("hasshuken",                 commands)
                        elseif special >= 70  then ch:execute_command("dodonpa "     .. vict_name, commands)
                        elseif special >= 50  then ch:execute_command("barrier 40",                commands)
                        else                       ch:execute_command("barrier 25",                commands)
                        end
                    elseif sensei == "piccolo" then
                        if     special >= 100 then ch:execute_command("scatter "     .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("sbc "         .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("masenko "     .. vict_name, commands)
                        elseif special >= 50  then ch:execute_command("barrier 40",                commands)
                        else                       ch:execute_command("barrier 25",                commands)
                        end
                    elseif sensei == "bardock" then
                        if     special >= 100 then ch:execute_command("finalflash "  .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("bigbang "     .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("galikgun "    .. vict_name, commands)
                        elseif special >= 50  then ch:execute_command("barrier 40",                commands)
                        else                       ch:execute_command("barrier 25",                commands)
                        end
                    elseif sensei == "andsix"  then
                        if     special >= 100 then ch:execute_command("hellflash "   .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("kousengan "   .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("dualbeam "    .. vict_name, commands)
                        elseif special >= 50  then ch:execute_command("barrier 40",                commands)
                        else                       ch:execute_command("barrier 25",                commands)
                        end
                    elseif sensei == "nail"    then
                        if     special >= 100 then ch:execute_command("regenerate 50",             commands)
                        elseif special >= 80  then ch:execute_command("heal self",                 commands)
                        elseif special >= 70  then ch:execute_command("masenko "     .. vict_name, commands)
                        else                       ch:execute_command("zanzoken",                  commands)
                        end
                    elseif sensei == "kurzak"  then
                        if     special >= 100 then ch:execute_command("ensnare "     .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("seishou "     .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("renzokou "    .. vict_name, commands)
                        elseif special >= 50  then ch:execute_command("barrier 40",                commands)
                        else                       ch:execute_command("barrier 25",                commands)
                        end
                    elseif sensei == "jinto"   then
                        if     special >= 100 then ch:execute_command("nova "        .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("starbreaker " .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("trip "        .. vict_name, commands)
                        else                       ch:execute_command("zanzoken",                  commands)
                        end
                    elseif sensei == "tsuna"   then
                        if     special >= 100 then ch:execute_command("koteiru "     .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("waterrazor "  .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("waterspikes " .. vict_name, commands)
                        else                       ch:execute_command("barrier 20",                commands)
                        end
                    elseif sensei == "tapion"  then
                        if     special >= 100 then ch:execute_command("phoenix "     .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("darkness "    .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("twinslash "   .. vict_name, commands)
                        else                       ch:execute_command("zanzoken",                  commands)
                        end
                    elseif sensei == "kabito"  then
                        if     special >= 100 then ch:execute_command("barrage "     .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("psychic "     .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("heal "        .. vict_name, commands)
                        else                       ch:execute_command("zanzoken",                  commands)
                        end
                    elseif sensei == "dabura"  then
                        if     special >= 100 then ch:execute_command("hellspear "   .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("honoo "       .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("fireshield",               commands)
                        else                       ch:execute_command("zanzoken",                  commands)
                        end
                    elseif sensei == "ginyu"   then
                        if     special >= 100 then ch:execute_command("spiral "      .. vict_name, commands)
                        elseif special >= 80  then ch:execute_command("crusher "     .. vict_name, commands)
                        elseif special >= 70  then ch:execute_command("eraser "      .. vict_name, commands)
                        else                       ch:execute_command("zanzoken",                  commands)
                        end
                    end
                end)
            end
        end
    elseif not ch:is_humanoid() or not dragonpass then
        if race == "serpent" and math.random(1, 5) == 5 then
            ch:execute_command("strike " .. vict_name, commands)
        elseif race == "dragon" and math.random(1, 12) >= 10 and ch:vnum_get() ~= 17917 then
            ch:execute_command("breath " .. vict_name, commands)
        elseif math.random(1, 10) >= 7 and (ch:stat_get("level") or 0) >= 10 then
            ch:execute_command("ram " .. vict_name, commands)
        else
            ch:execute_command("bite " .. vict_name, commands)
        end
    end

    dbat.dgscripts.fight_mtrigger(ch)
end

-- ── NPC combat helpers ────────────────────────────────────────────────────

local function flee_mob(ch, msg)
    act.around(ch, msg, {actor=ch})
    for obj in ch:inventory_get() do
        obj:extract()
    end
    ch:extract()
end

local function npc_combat_ai(ch, cond)
    local vict = ch:fighting_get()
    if not vict then return end
    if not ch:is_npc() or ch:mob_flagged(MF.DUMMY) then return end

    local foe_flying = vict:condition_has("flying")
    local ch_flying  = ch:condition_has("flying")

    -- Altitude matching
    if foe_flying and not ch_flying and ch:is_humanoid() and ch:stat_get("level") > 10 then
        ch:execute_command("fly", commands)
        return
    end
    if not foe_flying and ch_flying then
        ch:execute_command("fly", commands)
        return
    end
    if foe_flying and ch_flying and
       ch:condition_number_get("flying", "altitude") < vict:condition_number_get("flying", "altitude") then
        ch:execute_command("fly high", commands)
        return
    end

    -- Non-flyers facing airborne foe may flee
    if foe_flying and not ch_flying and not ch:is_humanoid() and
       ch:position_get() > POS.RESTING and math.random(1, 30) >= 22 and not combat.block_calc(ch) then
        flee_mob(ch, "$n@G flees in terror and you lose sight of $m!")
        return
    end
    if foe_flying and ch:is_humanoid() and ch:stat_get("level") <= 10 and
       math.random(1, 30) >= 22 and not combat.block_calc(ch) then
        flee_mob(ch, "$n@G turns and runs away. You lose sight of $m!")
        return
    end

    -- Position recovery
    local pos = ch:position_get()
    if (pos == POS.SITTING or pos == POS.RESTING) and sec_roll_check(ch) == 1 then
        ch:execute_command("stand", commands)
        return
    end
    if ch:aff_flagged(AFF.PARA) and
       ch:stat_get("intelligence") + 10 < math.random(1, 60) then
        act.to_char(ch, "@yYou fail to overcome your paralysis!@n", {actor=ch})
        act.around(ch, "@Y$n @ystruggles with $s paralysis!@n", {actor=ch})
        return
    end
    if pos == POS.SLEEPING and not ch:aff_flagged(AFF.KNOCKED) and sec_roll_check(ch) == 1 then
        ch:execute_command("wake", commands)
        ch:execute_command("stand", commands)
        return
    end

    -- Can't attack if out of range or incapacitated
    if ch:room_get() ~= vict:room_get() or ch:aff_flagged(AFF.KNOCKED) or
       pos == POS.SITTING or pos == POS.RESTING or pos == POS.SLEEPING then
        return
    end

    if math.random(1, 30) <= 12 then return end

    mob_attack(ch, cond, vict:name_get() or "")
end

-- ── PC combat helpers ─────────────────────────────────────────────────────

local function tick_position_advantage(ch)
    local vict = ch:fighting_get()
    if not vict then return end

    if not ch:condition_has("advantageous_position") then
        if roll_balance(ch) > dbat.axion_dice(0) and math.random(1, 10) >= 7 then
            if not vict:condition_has("advantageous_position") then
                ch:send_line("@YYou manage to move into an advantageous position!@n")
                act.around(ch, "@y$n@Y manages to move into an advantageous position!@n", {actor=ch})
                ch:condition_add("advantageous_position")
            else
                if roll_balance(ch) > vict:roll_balance() then
                    act.to_char(ch,   "@YYou struggle to gain a better position than @y$N@Y and succeed!@n",    {actor=ch, target=vict})
                    act.to_char(vict, "@y$n@Y struggles to gain a better position than you and succeeds!@n",    {actor=ch, target=vict})
                    act.around(ch,    "@y$n@Y struggles to gain a better position than @y$N@Y and succeeds!@n", {actor=ch, target=vict})
                    vict:condition_remove("advantageous_position", "outmaneuvered")
                    ch:condition_add("advantageous_position")
                end
            end
        end
    else
        if roll_balance(ch) < dbat.axion_dice(-30) or ch:position_get() < POS.STANDING then
            ch:send_line("@YYou are moved out of your position!@n")
            act.around(ch, "@y$n@Y is moved out of $s position!@n", {actor=ch})
            ch:condition_remove("advantageous_position", "lost")
        end
    end
end

local function tick_wimp_flee(ch)
    local wimp = ch:wimp_level_get()
    if wimp > 0 and ch:meter_current("powerlevel") < wimp and ch:meter_current("powerlevel") > 0 then
        ch:send_line("You wimp out, and attempt to flee!")
        ch:flee("")
    end
end

local function tick_disguise_slip(ch)
    if not ch:player_flagged(PLR.DISGUISED) then return end
    if not ch:fighting_get() then return end
    if ch:skill_get("disguise") < math.random(1, 125) then
        ch:send_line("Your disguise comes off because of your swift movements!")
        ch:player_flag_set(PLR.DISGUISED, false)
        act.around(ch, "@W$n's@W disguise comes off because of $s swift movements!@n", {actor=ch})
    end
end

-- ── Condition definition ──────────────────────────────────────────────────

return {
    id         = "fighting",
    name       = "Fighting",
    tags       = { "fighting", "in_combat" },
    persistent = false,
    on_apply = function(ch, cond)
        cond:schedule_event("combat_tick", FIGHT_INTERVAL_MS, FIGHT_INTERVAL_MS)
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("combat_tick")
    end,
    on_event = function(ch, cond, event)
        if event ~= "combat_tick" then return end
        tick_position_advantage(ch)
        if not ch:is_npc() then
            tick_wimp_flee(ch)
            tick_disguise_slip(ch)
        else
            npc_combat_ai(ch, cond)
        end
    end,
}
