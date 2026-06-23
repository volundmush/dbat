local dbat = require("dbat")
local act  = dbat.lib.act
local AFF  = dbat.consts.aff_flags

local PULSE_3SEC = 30  -- 3 * PASSES_PER_SEC (10)

-- Small ki drain used on failed/partial attempts
local function ki_small_drain(ch)
    ch:meter_mod_int("ki", -math.floor(ch:meter_max("ki") * 0.002 + 150))
end

-- Clear any grapple or drag state before transforming
local function clear_holds(ch)
    local grappled = ch:grappled_get()
    if grappled then
        grappled:grappling_set(nil, 0)
        ch:grappled_set(nil, 0)
    end
    local grappling = ch:grappling_get()
    if grappling then
        ch:grappling_set(nil, 0)
        grappling:grappled_set(nil, 0)
    end
    local dragged = ch:dragging_get()
    if dragged then
        dragged:being_dragged_set(nil)
        ch:dragging_set(nil)
    end
end

local function do_hide(ch)
    clear_holds(ch)

    if dbat.axion_dice(0) > ch:stat_get("level") then
        ch:send_line("@MYour body starts to become loose and sag, but you lose focus and it reverts to its original shape!@n")
        ch:act_around("@m$n@M's body starts to become loose and sag, but $e seems to return normal a moment later.@n")
        ki_small_drain(ch)
        return
    end

    ch:send_line("@MYour body starts to become loose and sag. It continues to droop down until it begins to run down like a river of goo flowing from where your body was.@n")
    ch:act_around("@m$n@M's body starts to become loose and sag. Much of $s body begins to pour down and scatter around as pools of goo.@n")
    ki_small_drain(ch)
    ch:aff_flag_set(AFF.LIQUEFIED, true)
end

local function do_explode(ch, arg)
    if arg == "" then
        ch:send_line("Syntax: liquefy hide\nSyntax: liquefy explode (target)")
        return
    end

    if ch:meter_current("ki") < ch:meter_max("ki") * 0.10 + 150 then
        ch:send_line("You do not have enough ki for that action!")
        return
    end

    local vict = ch:acquire_room_target(arg)
    if not vict then
        ch:send_line("That target isn't here.")
        return
    end

    if not ch:can_kill(vict) then
        ch:send_line("You can't kill them!")
        return
    end

    clear_holds(ch)

    if dbat.axion_dice(0) > ch:stat_get("level") then
        ch:send_line("@MYour body starts to become loose and sag, but you lose focus and it reverts to its original shape!@n")
        ch:act_around("@m$n@M's body starts to become loose and sag, but $e seems to return normal a moment later.@n")
        ki_small_drain(ch)
        ch:wait_set(PULSE_3SEC)
        return
    end

    if ch:der_total("speed_index") < vict:der_total("speed_index") then
        -- Dodged
        act.message({
            actor  = "@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! However $E easily dodges and avoids your attempt!@n",
            target = "@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! However you are faster and managed to dodge the attempt.@n",
            room   = "@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! However $E easily dodges and avoids @m$n's@M attempt!@n",
        }, { actor = ch, target = vict })
        ki_small_drain(ch)
        if not ch:fighting_get()   then ch:start_fighting(vict) end
        if not vict:fighting_get() then vict:start_fighting(ch) end
        ch:wait_set(PULSE_3SEC * 2)
        return
    end

    if ch:meter_current("powerlevel") < vict:meter_current("powerlevel") * 2 then
        -- Resisted
        act.message({
            actor  = "@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! However as you force yourself in through $S mouth $E successfully resists and forces your back out!@n",
            target = "@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! However you think quickly and force $m out before $e has a chance to get fully into your body!@n",
            room   = "@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! However as $e forces $mself in through @R$N's@M mouth $E manages to resist and force @m$n@M back out!@n",
        }, { actor = ch, target = vict })
        ki_small_drain(ch)
        local dmg = math.floor(ch:meter_max("powerlevel") * 0.08)
        vict:damage({ powerlevel = dmg }, ch)
        ch:wait_set(PULSE_3SEC)
        return
    end

    -- Success: fill and explode
    act.message({
        actor  = "@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! As you fill $S body you expand outward until $s body explodes into a gory mess!@n",
        target = "@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! As $e fills your body it begins to expand until it is unable to take the strain any longer and explodes!@n",
        room   = "@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! As $e forces $mself in through @R$N's@M mouth $S body begins to expand until it can't take the strain any longer and explodes!@n",
    }, { actor = ch, target = vict })
    ki_small_drain(ch)
    vict:die(ch)
    ch:aff_flag_set(AFF.LIQUEFIED, true)
    ch:wait_set(60)  -- ~handle_cooldown(ch, 9) for average speed
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens

    if ch:race_get() ~= "majin" then
        ch:send_line("You are not capable of liquefying yourself right now. Try finding a giant blender maybe?")
        return
    end

    -- Toggle off
    if ch:aff_flagged(AFF.LIQUEFIED) then
        ch:act_around("@MSuddenly large chunks of goo start to hover up slowly. These very same chunks quickly begin to fly into each other, piling on as the ball of goo grows. Suddenly @m$n@M emerges as the ball of goo takes $s shape!@n")
        ch:send_line("@MYou begin to pull the liquid chunks of your body together. Those chunks hover upward and merge into each other until a large ball of goo is formed. Slowly your body emerges as the pieces of your body take on their old form!@n")
        ch:aff_flag_set(AFF.LIQUEFIED, false)
        ch:wait_set(PULSE_3SEC * 5)
        return
    end

    local subcommand = (arg[1] or ""):lower()

    if subcommand == "" then
        ch:send_line("Syntax: liquefy hide\nSyntax: liquefy explode (target)")
        return
    end

    if ch:meter_current("ki") < ch:meter_max("ki") * 0.002 + 150 then
        ch:send_line("You do not have enough ki to manage this level of body control!")
        return
    end

    if subcommand == "hide" then
        do_hide(ch)
    elseif subcommand == "explode" then
        do_explode(ch, arg[2] or "")
    else
        ch:send_line("Syntax: liquefy hide\nSyntax: liquefy explode (target)")
    end
end

return {
    id      = "liquefy",
    aliases = { { "liquefy", 6 } },
    execute = execute,
}
