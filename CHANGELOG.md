** 2.9.3 (latest) **
- Fixing fireshield message to not get all mishmoshed. [Volund]
- Altered snet's history so staff log all messages while online. [Volund]
- Altered all headers to enclose declarations in `extern "C" {}`, made some minor adjustments to isolate C++ usage in headers to just two files left. This enables seamless Zig integration. [Volund]
- Added `CHANGELOG.md`, `LICENSE` and `CONTRIBUTING.md` [Volund]
- Fixed Focus damage calc. [PetalFoxx]
- Fixed weight logic being backwards. [PetalFoxx]
- Changed Advance command to require lower privileges. [PetalFoxx]
- Fixed poison interacting with charge. [PetalFoxx]
- Fixed grandmaster damage calculations. [PetalFoxx]
- Numerous small fixes to combat math. [PetalFoxx]
- Correct the offline zenni interest formula. [Volund]
- Updated build system to Zig 0.16. Shuffled some includes around to streamline what Zig can see. [Volund]
- Vials of Sacred Elixir will no longer corrupt character thirst if overindulged. [Volund]
- Lost tails of Icers and Bio-Androids will be properly named when torn off. [Volund]
- Zig now program entry point. [Volund]
- Added new internal API functions for targeting entities by ID, subscribing entities for iteration lists. IDs are not yet stable between runs. [Volund]
- ziglua added as a dependency and compiling. [Volund]

** 2.9.2 **
- Last version from before this file was created.