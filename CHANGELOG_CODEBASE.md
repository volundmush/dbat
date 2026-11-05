# 2.9.4 (latest)
- There was a rollback to C23 with Zig 0.17 support. Gameplay changes were ported. [Volund]
- Split `CHANGELOG.md` into `CHANGELOG_CODEBASE.md` and `CHANGELOG_GAMEPLAY.md`

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