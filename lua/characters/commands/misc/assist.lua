local dbat   = require("dbat")

local function execute(ctx)
    local ch  = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if ch:fighting_get() then
        ch:send_line("You're already fighting!  How can you assist someone else?")
        return
    end

    if arg == "" then
        ch:send_line("Whom do you wish to assist?")
        return
    end

    local helpee = ch:acquire_room_target(arg)
    if not helpee then
        ch:send_line("That person isn't here.")
        return
    end
    if helpee == ch then
        ch:send_line("You can't help yourself any more than this!")
        return
    end

    local opponent = helpee:fighting_get()
    if not opponent then
        for vict in ch:room_get():people() do
            if vict:fighting_get() == helpee then
                opponent = vict
                break
            end
        end
    end

    local actlib = dbat.lib.act
    if not opponent then
        actlib.message({ actor = "But nobody is fighting $N!" }, { actor = ch, target = helpee })
        return
    end
    if not ch:can_see(opponent) then
        actlib.message({ actor = "You can't see who is fighting $N!" }, { actor = ch, target = helpee })
        return
    end

    ch:reveal_hiding(0)
    ch:send_line("You join the fight!")
    actlib.message({ target = "$N assists you!", room = "$n assists $N." }, { actor = ch, target = helpee })
    if not ch:fighting_get() then
        ch:start_fighting(opponent)
    end
    if not opponent:fighting_get() then
        opponent:start_fighting(ch)
    end
end

return { id = "assist", aliases = { { "assist", 5 } }, execute = execute }
