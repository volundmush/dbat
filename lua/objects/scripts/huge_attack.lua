local dbat = require("dbat")
local act = dbat.lib.act

local MF = dbat.consts.mob_flags

local M = {}

local function script_user(obj, script)
    return obj:user_get() or dbat.characters.by_id(script:number_get("user_id"))
end

function M.provoke_room_mobs(obj, script)
    local user = script_user(obj, script)
    if not user then return end

    local room = obj:room_get()
    if not room then return end

    for ch in room:people() do
        if ch:is_npc() and not ch:fighting_get() and not ch:mob_flagged(MF.NOKILL) then
            act.around(ch, "@W$n@R leaps at @C$N@R desperately!@n", { actor = ch, target = user })
            act.to_char(user, "@W$n@R leaps at YOU desperately!@n", { actor = ch, target = user })
            ch:launch_attack(ch:is_humanoid() and "punch" or "bite", user)
        end
    end
end

return M
