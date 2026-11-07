# 2.9.4 (latest)
- There was a rollback to C23 with optional Zig 0.17 support. Gameplay changes were ported. [Volund]
- Split `CHANGELOG.md` into `CHANGELOG_CODEBASE.md` and `CHANGELOG_GAMEPLAY.md`
- Removed Zig from the build. CMake now builds everything as C++26 (`-std=gnu++26`) with GCC 16; the fuzz preset still uses Clang, for libFuzzer. [Volund]
- Renamed every source file from `.c` to `.cpp`, and wrapped every header in `extern "C"` so symbols stay unmangled. [Volund]
- Headers are now C++: renamed from `.h` to `.hpp`, with all `extern "C"` wrapping removed, so the game's symbols use C++ linkage. [Volund]
- Stricter compiler warnings for C++ (format, sign-compare, cast-qual, cast-align, fall-through, pedantic and others), with `-Werror` in the debug and asan presets. Added `tools/analyze.sh` for GCC's static analyzer. [Volund]

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