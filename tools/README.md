# tools/

Developer checks for refactoring the server. None of this is part of the server build.

| script | what it checks |
|---|---|
| `verify.sh [ref]` | Behaviour against a git ref (default `HEAD`). Both builds must give the same `dbat -c` result and log, the same boot log, and byte-identical saved data after loading and re-saving every player and every zone's mobs. The RNG is seeded with a fixed value, because mob stats are rolled at boot. Each build runs on its own copy of `data/`. |
| `objdiff.sh [ref]` | Generated code against a git ref. Each function's normalised disassembly is compared, matched by name across files, so moving functions between files doesn't count as a change. Use it to show a refactor changed no code. Making a function or variable `static` counts as no change, but the optimisations that `static` enables (a never-written variable folded to a constant, for example) still show up; inspect those with `OBJDIFF_SHOW=fn1,fn2`, which prints the normalised diff. For deliberate renames, set `OBJDIFF_RENAME="old=new,..."`. |
| `deadcode.sh` | Lists functions that `main()` can never reach, via the linker's `--gc-sections` report. Code disabled by a constant condition (`if (false && ...)`) is invisible to it, so the compiler is the final check when you delete something. |
| `check-headers.sh` | Every header under `libs/dbat/include` compiles on its own. |
| `check-writes.sh` | Every file the game rewrites goes through `atomic_open()` (`dbat/atomicfile.h`), so a crash or full disk can't leave it half-written. It lists any raw write, append or update `fopen()` outside the exceptions in the script, such as append-only logs. |
| `check-members.py` | Direct member access to `struct char_data`, `obj_data` and `room_data` outside the `_utils` file that owns each one, found from clang's syntax tree and counting uses through macros. Use `--enforce char_data obj_data room_data` to fail if any remain, and `--show obj_data.name` to list them. Needs `clang`. |
| `fuzz.py TARGET` | Coverage-guided fuzzing with libFuzzer (`fuzz` preset): `load` for player files, `boot` for world files, `script` for DG scripts, `bot` for bot journals. Runs for `-t` seconds, then replays each saved failure and writes `report.txt`, grouped by where it happened. |
| `symbolize.sh [binary]` | Reads a backtrace from the syslog on stdin and prints each frame as `function at file:line`, static functions and inlined frames included. The default binary is `build/bin/dbat`. It must be the exact binary that wrote the log. |

- **Shared pieces:**
  - `build-tree.sh` is the shared builder.
  - `roundtrip.cpp` holds `roundtrip_dump()`, which `verify.sh` calls from gdb. For each tree, `verify.sh` checks whether it passes `g` to this and to `circle_srandom()`, so a ref from before `GameWorld` still works. It calls server functions, so each tree is built with its own copy when it has one.
- **Where they work:** the scripts build in `$TMPDIR` and need `gcc`, `gdb`, `objdump` and `python3`.
- **Typical loop:** make a change, run `tools/objdiff.sh` if it should be code-neutral, and run `tools/verify.sh` before committing.

## Tests

`libs/dbat/tests/` holds self-contained tests that need no `data/` files. Run them with `ctest` from a CMake build directory.

Game state lives in `GameWorld` (`libs/dbat/include/dbat/game_world.h`), so a test can build as many worlds as it wants. A test that builds a world by hand only has to set the global `logfile`, which stays global because crash handlers write to it.

To test a whole running game, use `libs/dbat/tests/harness.h`. It copies `libs/dbat/tests/fixtures/world/`, a small hand-written world, to a temp directory and boots it there with no sockets, so nothing touches your `data/`. A test can log players in, type commands, read their output and run pulses.

- `test_world.cpp` plays through game systems: fights, followers, scripts, zone resets, shops and saving.
- `test_bad_data.cpp` gives the game scripts with mistakes in them, damaged files and edits that would lose something, each in a process of its own.
- `test_gameplay.cpp` checks game rules that were once wrong, such as who a heal or healing glow lands on, what playing a song blocks, and what reduces damage.
- `test_commands.cpp` runs every command with awkward arguments, at three admin levels and in four states, under whichever sanitizers the build uses. Each case forks from the booted game, so one crash can't hide another. `dbat_test_commands <command>` runs one command and prints the full report. Use the ASan preset, because a plain build can't see most memory errors. It takes minutes, so skip it with `ctest -LE slow`, or run it alone with `ctest -R commands`. Failures in commands listed in `known_failures` are printed but don't fail the test.

- `test_bot.cpp` has four bots play the fixture world at random, checking the audit after every action. They use every race and sensei, editors, real logins over sockets, long input, dropped connections and clock jumps. A run is a journal of actions, so `dbat_test_bot --replay FILE` repeats it exactly. Failures matching `known_failures` in that file are printed but don't fail the test. Journals in `tests/corpus/` run as a test; `tests/corpus/open/` holds short journals for open bugs, to move up once fixed.

To hunt for bugs with everything you have, run `tools/hunt.sh`, and Ctrl-C it when you like. It runs bots at low priority on every core, as memory allows, groups failures by cause, shrinks an example of each, and writes `build-asan/hunt/report.md`. Seeds carry on from where the last hunt stopped. Options:

- `--world data` plays a scratch copy of a real data directory, with your real scripts, shops and players. The directory is only ever read. It's far slower per seed than the fixture, and each worker takes about 750 MB.
- `-t SECONDS` stops after that long, and `-j N` sets the workers.

`tools/shrink.py BINARY JOURNAL` cuts a failing journal down by hand. `tools/coverage.sh` reports which code the tests reach, and writes `never-run.txt` listing the functions they don't.

`tools/test_copyover.py BINARY [SCENARIO...]` runs real servers and clients through copyovers: players coming back with their rooms and objects, switched immortals, builders in editors, link-dead players, wizlock and command line options surviving, a missing binary, damaged copyover files, and the deadlock timer. `ctest` runs it, under the `slow` label.

`world_audit()` (`dbat/world_audit.h`) checks that the game's lists agree with each other. The tests call it after every step, `dbat -c` runs it after loading the world, and admins can run it in a live game with the `audit` command.

A test can pin the clock with `g->pinned_time`, which is what `game_now()` then returns. Game code should call `game_now(g)` and not `time(0)`.
