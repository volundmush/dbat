local dbat = require("dbat")
local act  = require("lua.libs.act")

local AFF = dbat.consts.aff_flags
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local function block_calc(ch)
    local blocker = ch:blocked_by_get()
    if not blocker then return 1 end

    local function escape_acts(attempted)
        if attempted then
            act.around(ch,    "$n proves $s great skill and escapes from $N's attempted block!", {actor=ch, target=blocker})
            act.to_char(blocker, "$n proves $s great skill and escapes from your attempted block!", {actor=ch, target=blocker})
            act.to_char(ch,   "Using your great skill you manage to escape from $N's attempted block!", {actor=ch, target=blocker})
        else
            act.around(ch,    "$n proves $s great skill and escapes from $N!", {actor=ch, target=blocker})
            act.to_char(blocker, "$n proves $s great skill and escapes from you!", {actor=ch, target=blocker})
            act.to_char(ch,   "Using your great skill you manage to escape from $N!", {actor=ch, target=blocker})
        end
        ch:blocked_by_set(nil)
        blocker:blocking_set(nil)
    end

    if ch:der_total("speed_index") < blocker:der_total("speed_index")
       and blocker:position_get() > POS.SITTING then
        if not blocker:aff_flagged(AFF.BLIND) and not blocker:player_flagged(PLR.EYEC) then
            local minimum = math.min(100, (blocker:stat_get("charisma") or 0) + math.random(5, 20))
            local ea = ch:skill_get("escape_artist") or 0
            if ea == 0 or ea < math.random(minimum, 120) then
                act.around(ch,    "$n tries to leave, but can't outrun $N!", {actor=ch, target=blocker})
                act.to_char(blocker, "$n tries to leave, but can't outrun you!", {actor=ch, target=blocker})
                act.to_char(ch,   "You try to leave, but can't outrun $N!", {actor=ch, target=blocker})
                if ch:condition_has("flying") and not blocker:condition_has("flying") then
                    local alt = ch:condition_number_get("flying", "altitude")
                    if alt == 1 or alt == 2 then
                        if alt == 1 then
                            blocker:send_line("You're now floating in the air.")
                        else
                            blocker:send_line("You're now floating high in the sky.")
                        end
                        blocker:condition_add("flying", "skill", "fly")
                        blocker:condition_number_set("flying", "altitude", alt)
                    end
                end
                return 0
            end
        end
        escape_acts(true)
    elseif blocker:position_get() <= POS.SITTING then
        escape_acts(false)
    else
        escape_acts(true)
    end
    return 1
end

return { block_calc = block_calc }
