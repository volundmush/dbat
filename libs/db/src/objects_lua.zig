const zlua = @import("zlua");

const Lua = zlua.Lua;

pub fn register(lua: *Lua) void {
    lua.newTable();
    lua.setField(-2, "objects");
}
