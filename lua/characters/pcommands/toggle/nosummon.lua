local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nosummon", aliases = { { "nosummon", 8 } }, flag = T.PRF.SUMMONABLE, off = "You are now safe from summoning by other players.\r\n", on = "You may now be summoned by other players.\r\n" }
