local dbat     = require("dbat")
local movement = require("lua.libs.movement")
local act      = require("lua.libs.act")
local combat   = require("lua.libs.combat")

local PLR = dbat.consts.player_flags
local AFF = dbat.consts.aff_flags
local PRF = dbat.consts.prf_flags
local RF  = dbat.consts.room_flags
local POS = dbat.consts.positions

local ALIAS_TO_DIR = {
    north=0, n=0, east=1, e=1, south=2, s=2, west=3, w=3,
    up=4, u=4, down=5, d=5,
    northwest=6, nw=6, northeast=7, ne=7,
    southeast=8, se=8, southwest=9, sw=9,
    inside=10, ["in"]=10, outside=11, out=11,
}

local DIR_NAMES = movement.DIR_NAMES

return {
    id = "move",
    priority = 100,
    aliases = {
        {"north",     5}, {"n",         1},
        {"south",     5}, {"s",         1},
        {"east",      4}, {"e",         1},
        {"west",      4}, {"w",         1},
        {"up",        2}, {"u",         1},
        {"down",      4}, {"d",         1},
        {"northwest", 6}, {"nw",        2},
        {"northeast", 6}, {"ne",        2},
        {"southeast", 6}, {"se",        2},
        {"southwest", 6}, {"sw",        2},
        {"inside",    3}, {"in",        2},
        {"outside",   4}, {"out",       3},
    },
    execute = function(ctx)
        local ch  = ctx.ch
        local dir = ALIAS_TO_DIR[ctx.alias]
        if dir == nil then return end

        if ch:is_npc() then
            movement.perform_move(ch, dir)
            return
        end

        if ch:player_flagged(PLR.SELFD) then
            ch:send_line("You are preparing to blow up!")
            return
        end
        if ch:aff_flagged(AFF.LIQUEFIED) then
            ch:send_line("You are liquefied right now!")
            return
        end

        local charge = ch:charge_get()
        local ki_max = ch:meter_max("ki")
        if charge >= ki_max * 0.51 then
            ch:send_line("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.")
            return
        elseif charge >= ki_max * 0.5 and ch:skill_get("concentration") < 100 then
            ch:send_line("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.")
            return
        elseif charge >= ki_max * 0.4 and ch:skill_get("concentration") < 80 then
            ch:send_line("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.")
            return
        elseif charge >= ki_max * 0.3 and ch:skill_get("concentration") < 70 then
            ch:send_line("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.")
            return
        elseif charge >= ki_max * 0.2 and ch:skill_get("concentration") < 60 then
            ch:send_line("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.")
            return
        end

        local drunk = ch:stat_get("drunk")
        if drunk > 4 and (math.random(1, 9) + drunk) >= math.random(14, 20) then
            ch:send_line("You wobble around and then fall on your ass.")
            act.around(ch, "@C$n@W wobbles around before falling on $s ass@n.", {actor=ch})
            ch:position_set(POS.SITTING)
            return
        end

        if ch:fighting_get() then
            ch:flee(DIR_NAMES[dir + 1])
            return
        end

        if ch:player_flagged(PLR.PILOTING) then
            ch:drive_vehicle(dir)
            return
        end

        if ch:player_flagged(PLR.HEALT) then
            ch:send_line("You are inside a healing tank!")
            return
        end

        local room = ch:room_get()
        if room then
            for obj in room:contents_get() do
                if obj:kicharge_get() > 0 and obj:user_get() == ch then
                    ch:send_line("You are too busy controlling your attack!")
                    return
                end
            end
        end

        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 and
           ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 and
           not ch:condition_has("flying") then
            ch:send_line("Unless you fly, you can't get far with no limbs.")
            return
        end

        if ch:grappling_get() or ch:grappled_get() then
            ch:send_line("You are grappling with someone!")
            return
        end

        local absorbing = ch:absorbing_get()
        if absorbing then
            ch:send_line("You are busy absorbing from %s!", absorbing:name_get())
            return
        end

        local absorbed_by = ch:absorbed_by_get()
        if absorbed_by then
            if dbat.axion_dice(0) < absorbed_by:skill_get("absorb") then
                ch:send_line("You are being held by %s, they are absorbing you!", absorbed_by:name_get())
                absorbed_by:send_line("%s struggles in your grasp!", ch:name_get())
                ch:wait_set(20)
                return
            else
                act.message({
                    room   = "@c$N@W manages to break loose of @C$n's@W hold!@n",
                    target = "@WYou manage to break loose of @C$n's@W hold!@n",
                    actor  = "@c$N@W manages to break loose of your hold!@n",
                }, {actor = absorbed_by, target = ch})
                ch:absorbed_by_set(nil)
                absorbed_by:absorbing_set(nil)
            end
        end

        if not combat.block_calc(ch) then return end

        if ch:eavesdrop_get() > 0 then
            ch:send_line("You stop eavesdropping.")
            ch:eavesdrop_set(0)
        end

        if ch:pref_flagged(PRF.ARENAWATCH) then
            ch:pref_flag_set(PRF.ARENAWATCH, false)
            ch:arena_idnum_set(-1)
        end

        local vnum = room and room:vnum_get() or 0
        if vnum ~= 0 and vnum ~= 1 then
            ch:loadroom_set(vnum)
        end

        local gravity = room and room:gravity_get() or 0
        local hp_max  = ch:stat_get("max_hit")
        local is_bardock = ch:sensei_get() == "bardock"

        if gravity == 10   and hp_max <= 10000     and not is_bardock then ch:send_line("The gravity slows you down some.") ch:wait_set(10)  end
        if gravity == 20   and hp_max <= 30000                        then ch:send_line("The gravity slows you down some.") ch:wait_set(20)  end
        if gravity == 30   and hp_max <= 100000                       then ch:send_line("The gravity slows you down some.") ch:wait_set(30)  end
        if gravity == 40   and hp_max <= 200000                       then ch:send_line("The gravity slows you down some.") ch:wait_set(30)  end
        if gravity == 50   and hp_max <= 300000                       then ch:send_line("The gravity slows you down some.") ch:wait_set(30)  end
        if gravity == 100  and hp_max <= 500000                       then ch:send_line("The gravity slows you down some.") ch:wait_set(30)  end
        if gravity == 200  and hp_max <= 1000000                      then ch:send_line("The gravity slows you down some.") ch:wait_set(30)  end
        if gravity == 300  and hp_max <= 8000000                      then ch:send_line("The gravity slows you down some.") ch:wait_set(30)  end
        if gravity == 400  and hp_max <= 15000000                     then ch:send_line("The gravity slows you down some.") ch:wait_set(30)  end
        if gravity == 500  and hp_max <= 25000000                     then ch:send_line("The gravity slows you down some.") ch:wait_set(40)  end
        if gravity == 1000 and hp_max <= 35000000                     then ch:send_line("The gravity slows you down some.") ch:wait_set(50)  end
        if gravity == 5000 and hp_max <= 100000000                    then ch:send_line("The gravity slows you down some.") ch:wait_set(50)  end
        if gravity == 10000 and hp_max <= 200000000                   then ch:send_line("The gravity slows you down some.") ch:wait_set(50)  end

        if room and room:flagged(RF.SPACE) and ch:admin_level_get() < 1 then
            ch:send_line("You struggle to cross the vast distance.")
            ch:wait_set(60)
        elseif ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 and
               ch:limbcond_get(1) <= 0 and not ch:condition_has("flying") then
            act.to_char(ch, "@wYou slowly pull yourself along with your arm...@n", {actor=ch})
            act.around(ch, "@C$n@w slowly pulls $mself along with one arm...@n", {actor=ch})
            if ch:limbcond_get(2) < 50 then
                ch:send_line("@RYour left arm is damaged by the forced use!@n")
                ch:limbcond_set(2, ch:limbcond_get(2) - math.random(1, 5))
                if ch:limbcond_get(2) <= 0 then
                    act.to_char(ch, "@RYour left arm falls apart!@n", {actor=ch})
                    act.around(ch, "@r$n's@R left arm falls apart!@n", {actor=ch})
                end
            end
            ch:wait_set(50)
        elseif ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 and
               ch:limbcond_get(2) <= 0 and not ch:condition_has("flying") then
            act.to_char(ch, "@wYou slowly pull yourself along with your arm...@n", {actor=ch})
            act.around(ch, "@C$n@w slowly pulls $mself along with one arm...@n", {actor=ch})
            if ch:limbcond_get(1) < 50 then
                ch:send_line("@RYour right arm is damaged by the forced use!@n")
                ch:limbcond_set(1, ch:limbcond_get(1) - math.random(1, 5))
                if ch:limbcond_get(1) <= 0 then
                    act.to_char(ch, "@RYour right arm falls apart!@n", {actor=ch})
                    act.around(ch, "@r$n's@R right arm falls apart!@n", {actor=ch})
                end
            end
            ch:wait_set(50)
        elseif ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 and
               not ch:condition_has("flying") then
            act.to_char(ch, "@wYou slowly pull yourself along with your arms...@n", {actor=ch})
            act.around(ch, "@C$n@w slowly pulls $mself along with one arms...@n", {actor=ch})
            if ch:limbcond_get(2) < 50 then
                ch:send_line("@RYour left arm is damaged by the forced use!@n")
                ch:limbcond_set(2, ch:limbcond_get(2) - math.random(1, 5))
                if ch:limbcond_get(2) <= 0 then
                    act.to_char(ch, "@RYour left arm falls apart!@n", {actor=ch})
                    act.around(ch, "@r$n's@R left arm falls apart!@n", {actor=ch})
                end
            end
            if ch:limbcond_get(1) < 50 then
                ch:send_line("@RYour right arm is damaged by the forced use!@n")
                ch:limbcond_set(1, ch:limbcond_get(1) - math.random(1, 5))
                if ch:limbcond_get(1) <= 0 then
                    act.to_char(ch, "@RYour right arm falls apart!@n", {actor=ch})
                    act.around(ch, "@r$n's@R right arm falls apart!@n", {actor=ch})
                end
            end
            ch:wait_set(30)
        elseif ch:limbcond_get(3) <= 0 and not ch:condition_has("flying") then
            act.to_char(ch, "@wYou hop on one leg...@n", {actor=ch})
            act.around(ch, "@C$n@w hops on one leg...@n", {actor=ch})
            if ch:limbcond_get(4) < 50 then
                ch:send_line("@RYour left leg is damaged by the forced use!@n")
                ch:limbcond_set(4, ch:limbcond_get(4) - math.random(1, 5))
                if ch:limbcond_get(4) <= 0 then
                    act.to_char(ch, "@RYour left leg falls apart!@n", {actor=ch})
                    act.around(ch, "@r$n's@R left leg falls apart!@n", {actor=ch})
                end
            end
            ch:wait_set(20)
        elseif ch:limbcond_get(4) <= 0 and not ch:condition_has("flying") then
            act.to_char(ch, "@wYou hop on one leg...@n", {actor=ch})
            act.around(ch, "@C$n@w hops on one leg...@n", {actor=ch})
            if ch:limbcond_get(3) < 50 then
                ch:send_line("@RYour right leg is damaged by the forced use!@n")
                ch:limbcond_set(3, ch:limbcond_get(3) - math.random(1, 5))
                if ch:limbcond_get(3) <= 0 then
                    act.to_char(ch, "@RYour right leg falls apart!@n", {actor=ch})
                    act.around(ch, "@r$n's@R right leg falls apart!@n", {actor=ch})
                end
            end
            ch:wait_set(20)
        elseif ch:position_get() == POS.RESTING then
            act.to_char(ch, "@wYou crawl on your hands and knees.@n", {actor=ch})
            act.around(ch, "@C$n@w crawls on $s hands and knees.@n", {actor=ch})
            local chair = ch:sits_get()
            if chair then
                chair:sitting_set(nil)
                ch:sits_set(nil)
            end
            ch:wait_set(30)
        elseif ch:position_get() == POS.SITTING then
            act.to_char(ch, "@wYou shuffle on your hands and knees.@n", {actor=ch})
            act.around(ch, "@C$n@w shuffles on $s hands and knees.@n", {actor=ch})
            local chair = ch:sits_get()
            if chair then
                chair:sitting_set(nil)
                ch:sits_set(nil)
            end
            ch:wait_set(20)
        elseif ch:position_get() < POS.RESTING then
            ch:send_line("You are in no condition to move! Try standing...")
            return
        end

        movement.perform_move(ch, dir)
        ch:rdisplay_clear()
    end,
}
