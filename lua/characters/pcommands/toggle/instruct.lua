local T = require("lua.libs.pcommand_toggles")
return T.pref{ id = "instruct", aliases = { { "instruct", 7 } }, flag = T.PRF.INSTRUCT, off = "You will no longer instruct those you spar with.\r\n", on = "You will now instruct those you spar with.\r\n" }
