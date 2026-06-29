local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "nomail", aliases = { { "nomail", 6 } }, flag = T.PRF.NMWARN, off = "You will now be told that you have mail on prompt.\r\n", on = "You will no longer be told that you have mail on prompt.\r\n" }
