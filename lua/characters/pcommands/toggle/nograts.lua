local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nograts", aliases = { { "nograts", 7 } }, flag = T.PRF.NOGRATZ, off = "You can now hear the congratulation messages.\r\n", on = "You are now deaf to the congratulation messages.\r\n" }
