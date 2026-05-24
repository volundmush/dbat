const cdb = @import("cdb");
const obj_api = @import("objects_api.zig");

pub export fn char_inventory_iterate(ch: *cdb.char_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    obj_api.obj_contents_list_iterate(ch.carrying, recursive, callback, ctx);
}
