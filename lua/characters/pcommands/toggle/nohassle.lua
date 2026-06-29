local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nohassle", aliases = { { "nohassle", 8 } }, flag = T.PRF.NOHASSLE, admin_level = T.ADMLVL.IMMORT, off = "Nohassle disabled.\r\n", on = "Nohassle enabled.\r\n" }
