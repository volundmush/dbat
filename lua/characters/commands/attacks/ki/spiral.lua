local dbat = require("dbat")
local ki   = require("lua.libs.ki")
local function act() return dbat.lib.act end

local PLR = dbat.consts.player_flags

return {
    id      = "spiral",
    aliases = { { "spiral", 4 } },

    can_execute = function(ch)
        if ch:condition_has("spiral") then
            return false, "You are already using your Spiral Comet technique!"
        end
        return true
    end,

    execute = function(ctx)
        local ch  = ctx.ch
        local arg = ctx.argparams.tokens[1] or ""

        if not ki.can_grav(ch) then return end

        if (ch:skill_get("spiral") or 0) <= 0 then
            ch:send_line("You do not know the Spiral Comet technique.")
            return
        end

        if ch:limbcond_get(1) <= 0 and ch:limbcond_get(2) <= 0 then
            ch:send_line("You need at least one working arm to perform Spiral Comet!")
            return
        end

        local target = ch:acquire_room_target(arg)
        if not target then
            ch:send_line("Direct it at who?")
            return
        end

        local ki_needed = math.floor(ch:meter_max("ki") * 0.5)
        if ch:meter_current("ki") < ki_needed then
            ch:send_line("You don't have enough ki to begin the Spiral Comet.")
            return
        end

        if not target:is_npc() and target:player_flagged(PLR.IMMORTAL) then
            return
        end

        ch:improve_skill("spiral", 1)

        act().message({
            actor = "@mFlying to a spot above your intended target you begin to move so fast all that can be seen of you are trails of color. You focus your movements into a vortex and prepare to attack!@n",
            room  = "@w$n@m flies to a spot above and begins to move so fast all that can be seen of $m are trails of color. Suddenly $e focuses $s movements into a spinning vortex and you lose track of $s movements entirely!@n",
        }, { actor = ch })

        ch:condition_apply("spiral")
        local cond = ch:condition("spiral")
        if cond then
            cond:number_set("target_id", target:id_get())
            cond:number_set("skill",     ch:skill_get("spiral") or 0)
            cond:number_set("first",     1)
        end

        ch:cooldown_set(8)
    end,
}
