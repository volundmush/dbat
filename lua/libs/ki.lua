local M = {}

-- Threshold table from can_grav() in src/character_utils.cpp.
-- Each entry: if room gravity == gravity AND pl < pl_min then block (unless bardock-exception).
local GRAVITY_THRESHOLDS = {
    { gravity = 10,    pl = 5000     },
    { gravity = 20,    pl = 20000    },
    { gravity = 30,    pl = 50000    },
    { gravity = 40,    pl = 100000   },
    { gravity = 50,    pl = 200000   },
    { gravity = 100,   pl = 400000   },
    { gravity = 200,   pl = 1000000  },
    { gravity = 300,   pl = 5000000  },
    { gravity = 400,   pl = 8000000  },
    { gravity = 500,   pl = 15000000 },
    { gravity = 1000,  pl = 25000000 },
    { gravity = 5000,  pl = 100000000 },
    { gravity = 10000, pl = 200000000 },
}

function M.can_grav(ch)
    local gravity = ch:room_get():gravity_get()
    if gravity <= 0 then return true end
    local pl = ch:meter_max("powerlevel")
    for _, t in ipairs(GRAVITY_THRESHOLDS) do
        if gravity == t.gravity and pl < t.pl then
            -- Bardock sensei and NPCs are exempt from the gravity 10 check
            if gravity == 10 and (ch:sensei_get() == "bardock" or ch:is_npc()) then
                return true
            end
            return false, "You are barely able to move in this crushing gravity!"
        end
    end
    return true
end

return M
