local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "notell", aliases = { { "notell", 6 } }, flag = T.PRF.NOTELL, off = "You can now hear tells.\r\n", on = "You are now deaf to tells.\r\n" }
