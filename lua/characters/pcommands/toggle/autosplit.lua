local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "autosplit", aliases = { { "autosplit", 6 } }, flag = T.PRF.AUTOSPLIT, admin_level = T.ADMLVL.IMMORT, off = "Autosplit disabled.\r\n", on = "Autosplit enabled.\r\n" }
