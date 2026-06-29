local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nowiz", aliases = { { "nowiz", 5 } }, flag = T.PRF.NOWIZ, admin_level = T.ADMLVL.IMMORT, off = "You can now hear the Wiz-channel.\r\n", on = "You are now deaf to the Wiz-channel.\r\n" }
