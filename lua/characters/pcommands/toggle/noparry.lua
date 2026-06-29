local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "noparry", aliases = { { "noparry", 6 } }, flag = T.PRF.NOPARRY, off = "You will now parry attacks.\r\n", on = "You will no longer parry attacks.\r\n" }
