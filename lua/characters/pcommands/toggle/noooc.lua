local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "noooc", aliases = { { "noooc", 5 } }, flag = T.PRF.NOGOSS, off = "You can now hear ooc.\r\n", on = "You are now deaf to ooc.\r\n" }
