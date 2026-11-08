# 2.9.4 (latest)
- There was a rollback to C23 with optional Zig 0.17 support. Gameplay changes were ported. [Volund]
- Split `CHANGELOG.md` into `CHANGELOG_CODEBASE.md` and `CHANGELOG_GAMEPLAY.md`
- Removed Zig from the build. CMake now builds everything as C++26 (`-std=gnu++26`) with GCC 16; the fuzz preset still uses Clang, for libFuzzer. [Volund]
- Renamed every source file from `.c` to `.cpp`, and wrapped every header in `extern "C"` so symbols stay unmangled. [Volund]
- Headers are now C++: renamed from `.h` to `.hpp`, with all `extern "C"` wrapping removed, so the game's symbols use C++ linkage. [Volund]
- Stricter compiler warnings for C++ (format, sign-compare, cast-qual, cast-align, fall-through, pedantic and others), with `-Werror` in the debug and asan presets. Added `tools/analyze.sh` for GCC's static analyzer. [Volund]
- Every struct field has a default member initializer, and `CREATE`/`RECREATE` are gone: the game allocates with `new`/`delete`, and grows arrays with `resize_array()` (`dbat/arrays.hpp`). [Volund]
- Loops over characters, objects, connections, room contents, inventories and followers go through `dbat/iterate.hpp` (`for (char_data *ch : room_people(g, room))`, or closures such as `char_iter_all`). Each loop sees the members as they were when it began and passes over any destroyed or moved before their turn, so scripts that purge things mid-loop are safe. `world_audit` checks that every character and object can be found by its id, which this relies on. [Volund]
- Characters and objects refer to each other by id rather than by pointer (fighting, master, blocks, grappling, absorbing, defending, dragging, mind links, poisoners, seats, users, targets, walls, posts, carrying players, the summoned dragon, the object being restrung or iedited). The getters take the `GameWorld` and return null once the other entity has gone, where a pointer would have dangled. [Volund]
- Shops, guilds and zones are kept in `vnum_table`s (`dbat/registry.hpp`): owned, never moving, in vnum order, so a room or editor can hold a pointer to its zone. `shop_rnum`, `guild_rnum` and `zone_rnum` are gone, and with them the `top_*` counters and the OLC code that renumbered tables on insert. [Volund]
- Rooms and the mob, object and trigger prototypes are `vnum_table`s too, and every rnum type is gone: a room, prototype or trigger is found by its vnum (`room_of`, `obj_proto_of`, `mob_proto_of`), a character's `in_room` and an object's `item_number` hold vnums. `real_room()`, `real_object()` and `real_mobile()` are deleted, since `room_of`, `obj_proto_of` and `mob_proto_of` already say whether a vnum is in use. OLC no longer shifts tables, exits, zone commands, start rooms or occupants when a room or prototype is added or deleted, and deleting an object prototype also takes it out of shop stock lists. Waiting room triggers no longer need re-pointing, since rooms never move. [Volund]
- The accessors that take a room's vnum (`room_get_name(g, vnum)` and the rest, formerly `rroom_*`) are overloads of the ones that take the room, and are safe on a vnum with no room: getters give nothing and setters do nothing, so `NOWHERE` is simply a vnum no room has. `read_object()` and `read_mobile()` no longer take `REAL`/`VIRTUAL`. [Volund]
- Characters and objects are registered by id in `id_table`s (`g->characters`, `g->objects`), which replace both the `next`-linked global lists and DG's UID hash: being in the game and being findable by id are now one thing. `char_register()`/`obj_register()` and their `unregister` partners replace `add_to_lookup_table()`/`remove_from_lookup_table()`; unregistering removes only the entity registered under that id, so freeing a copy of a character (one loaded from a player file, say) no longer makes the real one unfindable. A player who returns to the menu is unregistered until they enter the game again. [Volund]
- Connections (`g->descriptors`, by `desc_num`) and attached triggers (`g->triggers`, by a serial id) are `id_table`s too, replacing the `next`/`next_in_world` lists; triggers waiting to be freed are a `std::vector`. Connection numbers still wrap at 1000, but skip any still in use. [Volund]
- `tools/verify.sh` round trip now saves every zone's mob, object, room, shop, guild and zone files as well as players. [Volund]

# 2.9.3
- Altered all headers to enclose declarations in `extern "C" {}`, made some minor adjustments to isolate C++ usage in headers to just two files left. This enables seamless Zig integration. [Volund]
- Added `CHANGELOG.md`, `LICENSE` and `CONTRIBUTING.md` [Volund]
- Updated build system to Zig 0.16. Shuffled some includes around to streamline what Zig can see. [Volund]
- Zig now program entry point. [Volund]
- Added new internal API functions for targeting entities by ID, subscribing entities for iteration lists. IDs are not yet stable between runs. [Volund]
- ziglua added as a dependency and compiling. [Volund]
- Program no longer chdir()'s into data directory. [Volund]
- Removed the RPP Bank Feature that should've been dead a while ago. [Volund]
- Created new APIs for Characters, Objects, Rooms, Zones, Shops, Guilds, Exits. [Volund]
- Created a new template-based search.hpp for closure-based searches in .cpp [Volund]
- Laying groundwork for eliminating C arrays for entity database. [Volund]

# 2.9.2
- Last version from before this file was created.