const cdb = @import("cdb");
const std = @import("std");

const FlagShift = std.math.Log2Int(cdb.bitvector_t);

pub fn get(values: []cdb.bitvector_t, pos: c_int) bool {
    const loc = location(pos, values.len) orelse return false;
    return (values[loc.index] & loc.mask) != 0;
}

pub fn toggle(values: []cdb.bitvector_t, pos: c_int) bool {
    const loc = location(pos, values.len) orelse return false;
    values[loc.index] ^= loc.mask;
    return (values[loc.index] & loc.mask) != 0;
}

pub fn set(values: []cdb.bitvector_t, pos: c_int, value: bool) void {
    const loc = location(pos, values.len) orelse return;
    if (value) {
        values[loc.index] |= loc.mask;
    } else {
        values[loc.index] &= ~loc.mask;
    }
}

fn location(pos: c_int, len: usize) ?struct { index: usize, mask: cdb.bitvector_t } {
    if (pos < 0) return null;
    const bit_pos: usize = @intCast(pos);
    const index = bit_pos / @bitSizeOf(cdb.bitvector_t);
    if (index >= len) return null;
    const shift: FlagShift = @intCast(bit_pos % @bitSizeOf(cdb.bitvector_t));
    return .{ .index = index, .mask = @as(cdb.bitvector_t, 1) << shift };
}
