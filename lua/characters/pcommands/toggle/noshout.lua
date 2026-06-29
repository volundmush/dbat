local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "noshout", aliases = { { "noshout", 7 } }, flag = T.PRF.DEAF, off = "You can now hear shouts.\r\n", on = "You are now deaf to shouts.\r\n" }
