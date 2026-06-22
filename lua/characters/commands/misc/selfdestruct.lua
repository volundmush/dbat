local dbat = require("dbat")
local act  = dbat.lib.act
local AFF  = dbat.consts.aff_flags
local PLR  = dbat.consts.player_flags
local MF   = dbat.consts.mob_flags

local function is_arena(ch)
    local vnum = ch:room_get():vnum_get()
    return vnum >= 17800 and vnum <= 17874
end

local function safe_to_kill(tch)
    if tch:player_flagged(PLR.IMMORTAL) then return false end
    if tch:mob_flagged(MF.NOKILL) then return false end
    return true
end

local function safe_discharge(ch)
    ch:send_line("@wYour body slowly stops flashing. Steam rises from your skin as you slowly let off the energy you built up in a safe manner.@n")
    ch:act_around("@w$n's body slowly stops flashing. Steam rises from $s skin as $e slowly lets off the energy $e built up in a safe manner.@n")
    ch:condition_remove("selfdestruct", "cancelled")
end

local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then return end

    if is_arena(ch) then
        ch:send_line("You can not use self destruct in the arena.")
        return
    end

    if ch:aff_flagged(AFF.SPIRIT) then
        ch:send_line("You are already dead!")
        return
    end

    if (ch:stat_get("level") or 0) < 9 then
        ch:send_line("You can't self destruct while protected by the newbie shield!")
        return
    end

    -- Bootstrap skill if needed
    if (ch:skill_get("selfd") or 0) == 0 then
        ch:skill_set("selfd", math.random(10, 20))
    end

    local phase = ch:condition_has("selfdestruct")
        and ch:condition_number_get("selfdestruct", "phase")
        or 0

    if phase == 0 then
        -- Phase 1: begin charging
        ch:send_line("@RYour body starts to glow @wwhite@R and flash. The flashes start out slowly but steadilly increase in speed. Your aura begins to burn around your body at the same time in a violent fashion!@n")
        ch:act_around("@R$n's body starts to glow @wwhite@R and flash. The flashes start out slowly but steadilly increase in speed. $n's aura begins to burn around $s body at the same time in a violent fashion!@n")
        ch:condition_apply("selfdestruct", "command", "selfdestruct")
        return
    end

    if phase == 1 then
        -- Not ready yet: cancel
        safe_discharge(ch)
        return
    end

    -- Phase 2: execute explosion
    local grapple_tch = ch:grappling_get()

    if grapple_tch and not safe_to_kill(grapple_tch) then
        safe_discharge(ch)
        ch:send_line("You can't kill them, the immortals won't allow it!")
        return
    end

    local dmg = ch:charge_get()
        + math.floor((ch:stat_get("powerlevel") or 0) * 0.6)
        + (ch:stat_get("stamina") or 0)
    ch:charge_set(0)
    ch:stat_set("suppression", 0)
    -- Attacker pays 1% current powerlevel
    local self_dmg = math.max(1, math.floor((ch:meter_current("powerlevel") or 0) * 0.01))
    ch:meter_mod_int("powerlevel", -self_dmg)

    ch:condition_remove("selfdestruct", "detonated")

    if grapple_tch then
        -- Focused explosion
        act.message({
            actor  = "@RYou EXPLODE! The explosion concentrates on @r$N@R, engulfing $M in a sphere of deadly energy!@n",
            target = "@R$n EXPLODES! The explosion concentrates on YOU, engulfing your body in a sphere of deadly energy!@n",
            room   = "@R$n EXPLODES! The explosion concentrates on @r$N@R, engulfing $M in a sphere of deadly energy!@n",
        }, { actor = ch, target = grapple_tch })
        grapple_tch:damage({ powerlevel = dmg }, ch)
    else
        -- Room-wide explosion
        local room_dmg = math.floor(dmg * 1.5)
        ch:send_line("@RYou EXPLODE! The explosion expands outward burning up all surroundings for a large distance. The explosion takes on the shape of a large energy dome with you at its center!@n")
        ch:act_around("@R$n EXPLODES! The explosion expands outward burning up all surroundings for a large distance. The explosion takes on the shape of a large energy dome with $n at its center!@n")
        for person in ch:room_get():people() do
            if not person:is_same(ch) then
                if person:mob_flagged(MF.NOKILL) then goto continue end
                if not person:is_npc() and (person:stat_get("level") or 0) <= 8 then goto continue end
                act.message({
                    actor  = "@r$N@R is caught in the explosion!@n",
                    target = "@RYou are caught in the explosion!@n",
                    room   = "@r$N@R is caught in the explosion!@n",
                }, { actor = ch, target = person })
                person:damage({ powerlevel = room_dmg }, ch)
                ::continue::
            end
        end
    end

    -- Skill improvement
    local sk = ch:skill_get("selfd") or 0
    ch:skill_set("selfd", math.min(100, sk + math.random(10, 20)))

    -- Survival check
    local race = ch:race_get()
    local is_majin_bio = (race == "majin" or race == "bio")
    if is_majin_bio and ch:meter_get("lifeforce") > 5000 then
        -- Majin/Bio survive: reduce LF to ~2%
        local max_lf = ch:meter_max("lifeforce")
        local target_lf = math.floor(max_lf * 0.02)
        local cur_lf   = ch:meter_current("lifeforce")
        if cur_lf > target_lf then
            ch:meter_mod_int("lifeforce", -(cur_lf - target_lf))
        end
        ch:meter_mod_int("powerlevel", -math.floor((ch:meter_max("powerlevel") or 0) * 0.01))
        ch:send_line("@WYour body begins to regenerate from the explosion!@n")
    else
        ch:die(nil)
    end
end

return {
    id      = "selfdestruct",
    aliases = { { "selfdestruct", 8 } },
    execute = execute,
}
