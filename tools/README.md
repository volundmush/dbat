# tools/

Developer checks for refactoring the server. None of this is part of the server build.

| script | what it checks |
|---|---|
| `verify.sh [--data DIR] [ref]` | Behaviour against a git ref (default `HEAD`), on `data/` or on DIR. Both builds must give the same `dbat -c` result and log, the same boot log, the same `statedump.txt` (what the boot loaded, from `statedump.cpp`), and byte-identical saved data after `roundtrip.cpp` loads and re-saves every player (with script variables, aliases and rent file), every account, every zone's world files, and the etc files the game writes (config, time, auction, assemblies, the player index, boards and houses). The RNG is seeded with a fixed value, because mob stats are rolled at boot, and the clock is pinned, because saves stamp the time. Each build runs on its own copy of the data. `VERIFY_SHARED_ROUNDTRIP=1` builds the ref with this tree's `roundtrip.cpp`; `VERIFY_KEEP=1` keeps the work directory. |
| `objdiff.sh [ref]` | Generated code against a git ref. Each function's normalised disassembly is compared, matched by name across files, so moving functions between files doesn't count as a change. Use it to show a refactor changed no code. Making a function or variable `static` counts as no change, but the optimisations that `static` enables (a never-written variable folded to a constant, for example) still show up; inspect those with `OBJDIFF_SHOW=fn1,fn2`, which prints the normalised diff. For deliberate renames, set `OBJDIFF_RENAME="old=new,..."`. |
| `deadcode.sh` | Lists functions that `main()` can never reach, via the linker's `--gc-sections` report. Code disabled by a constant condition (`if (false && ...)`) is invisible to it, so the compiler is the final check when you delete something. |
| `check-headers.sh` | Every header under `libs/dbat/include` compiles on its own. |
| `check-writes.sh` | Every file the game rewrites goes through `io::text_writer` (`dbat/io/writer.hpp`), so a crash or full disk can't leave it half-written. It lists any raw write, append or update `fopen()` outside the exceptions in the script, such as append-only logs. |
| `check-portable.sh` | The file layer (`src/io`) and the loaders and savers (`src/db`) use only standard C++ for files: it fails on a POSIX or Linux header, or a call such as `stat`, `unlink`, `getline`, `strlcpy` or `strcasecmp`, outside the exceptions in the script. |
| `check-olc.sh` | OLC stays on its editor sessions (`dbat/olc/session.hpp`) and `std::string`: it fails if the old Oasis machinery comes back (`oasis_olc_data`, `oasis.hpp`, the `OLC_*()` macros, `cleanup_olc()`, `str_udup()`), or if an OLC function parses a `char *` line, apart from nanny's two handlers. |
| `check-members.py` | Direct member access to `struct char_data`, `obj_data` and `room_data` outside the `_utils` file that owns each one, found from clang's syntax tree and counting uses through macros. Use `--enforce char_data obj_data room_data` to fail if any remain, and `--show obj_data.name` to list them. Needs `clang`. |
| `analyze.sh` | Runs GCC's static analyzer (`-fanalyzer`) over the server, about 20 seconds, and lists its findings grouped by kind, rarest first. It can't follow objects handed to global lists or files closed by a helper, so most leak reports, and some others, are false: a list of places to look, not failures. |
| `fuzz.py TARGET` | Coverage-guided fuzzing with libFuzzer (`fuzz` preset): `load` for player files, `boot` for world files, `script` for DG scripts, `bot` for bot journals, `olc` for lines typed at an OLC editor, `net` for raw bytes a client sends over a real socket from the first prompt on (a 0xff byte ends one write, 0xfe lets ten pulses pass). Runs for `-t` seconds, then replays each saved failure and writes `report.txt`, grouped by where it happened. |
| `cmdcompare.py BASE` | Checks that the working tree's commands behave exactly as commit BASE's did, for refactoring them: builds BASE in a worktree (with this tree's tests), runs both sides' command, technique, login, OLC and script transcripts and bot seeds, and shows the first cases that differ. About 5 minutes. `--keep` keeps the transcripts, and `--script-world DIR` also runs every trigger in that world (such as the real data). |
| `dg_strict.py WORLD` | The triggers in a world that the strict DG grammar refuses (a block keyword with no block, a block left open, text after `else` or `end`, and the like), and the lines it reads differently from the old driver, each with its line number. |
| `symbolize.sh [binary]` | Reads a backtrace from the syslog on stdin and prints each frame as `function at file:line`, static functions and inlined frames included. The default binary is `build/bin/dbat`. It must be the exact binary that wrote the log. |

- **Shared pieces:**
  - `build-tree.sh` is the shared builder.
  - `statedump.cpp` holds `state_dump()`, which `verify.sh` calls from gdb before `roundtrip_dump()`. A ref without one is built with this tree's copy.
  - `roundtrip.cpp` holds `roundtrip_dump()`, which `verify.sh` calls from gdb. For each tree, `verify.sh` checks whether it passes `g` to this and to `circle_srandom()`, so a ref from before `GameWorld` still works. It calls server functions, so each tree is built with its own copy when it has one.
- **Where they work:** the scripts build in `$TMPDIR` and need `gcc`, `gdb`, `objdump` and `python3`.
- **Typical loop:** make a change, run `tools/objdiff.sh` if it should be code-neutral, and run `tools/verify.sh` before committing.

## Tests

`libs/dbat/tests/` holds self-contained tests that need no `data/` files. Run them with `ctest` from a CMake build directory.

Game state lives in `GameWorld` (`libs/dbat/include/dbat/game_world.hpp`), so a test can build as many worlds as it wants. A test that builds a world by hand only has to set the global `logfile`, which stays global because crash handlers write to it.

To test a whole running game, use `libs/dbat/tests/harness.hpp`. It copies `libs/dbat/tests/fixtures/world/`, a small hand-written world, to a temp directory and boots it there with no sockets, so nothing touches your `data/`. A test can log players in, type commands, read their output and run pulses.

- `test_world.cpp` plays through game systems: fights, followers, scripts, zone resets, shops and saving.
- `test_bad_data.cpp` gives the game scripts with mistakes in them, damaged files and edits that would lose something, each in a process of its own.
- `test_gameplay.cpp` checks game rules that were once wrong, such as who a heal or healing glow lands on, what playing a song blocks, and what reduces damage.
- `test_olc.cpp` (`dbat_test_olc`) walks a builder through every OLC editor's menus, from opening to saving or not, with a watcher standing by who sees the game's log. Each journey forks from the booted game into its own copy of the world, so the files it saves are its own; the transcript has what the builder and the watcher saw after every line, then every file the journey changed, in full. `dbat_test_olc PREFIX` runs the journeys whose names start with PREFIX (such as `redit`).
- `test_commands.cpp` runs every command with awkward arguments, at three admin levels and in four states, under whichever sanitizers the build uses. Each case forks from the booted game, so one crash can't hide another. `dbat_test_commands <command>` runs one command and prints the full report. Use the ASan preset, because a plain build can't see most memory errors. It takes minutes, so skip it with `ctest -LE slow`, or run it alone with `ctest -R commands`. Failures in commands listed in `known_failures` are printed but don't fail the test. With `--transcript FILE` it writes what every case did instead (each with its own seed and the clock stopped: what each player saw and saved, the generator's state, the fixture's rooms), for `cmdcompare.py`; `--full` writes saved files whole rather than as hashes, and `--shard K/N` shares the commands out.
- `test_techniques.cpp` runs every attack by four attackers (one wielding a sword) at three skill levels and every perfection, against a player, a mob, an object and while fighting, at full and low charge, and in situations such as a defender stepping in, zanzoken, an android absorbing, no ki or no arms, so that the attacks' later branches are reached. It takes the same `--transcript`, `--full` and `--shard` options, and is labelled slow.

- `test_scripts.cpp` runs every trigger in a world on its own, to compare two builds of the script engine. Each goes on a stand-in owner with no other triggers, in the first room of its zone, beside two players; each of its types is fired through the game's dispatcher for it, and the script queue runs on until nothing waits. The transcript has what the dispatchers returned, what the players saw and saved, the owner's state and the log. `--synthetic` adds the triggers in `tests/scripts` (every construct, DG command, field and dispatcher, and the strict grammar's errors), and `--world DIR` runs another world's triggers.
- `test_bot.cpp` has four bots play the fixture world at random, checking the audit after every action. They use every race and sensei, editors, real logins over sockets, long input, dropped connections and clock jumps. A run is a journal of actions, so `dbat_test_bot --replay FILE` repeats it exactly. Failures matching `known_failures` in that file are printed but don't fail the test. Journals in `tests/corpus/` run as a test; `tests/corpus/open/` holds short journals for open bugs, to move up once fixed.

To hunt for bugs with everything you have, run `tools/hunt.sh`, and Ctrl-C it when you like. It runs bots at low priority on every core, as memory allows, groups failures by cause, shrinks an example of each, and writes `build-asan/hunt/report.md`. Seeds carry on from where the last hunt stopped. Options:

- `--world data` plays a scratch copy of a real data directory, with your real scripts, shops and players. The directory is only ever read. It's far slower per seed than the fixture, and each worker takes about 750 MB.
- `-t SECONDS` stops after that long, and `-j N` sets the workers.

`tools/shrink.py BINARY JOURNAL` cuts a failing journal down by hand. `tools/coverage.sh` reports which code the tests reach, and writes `never-run.txt` listing the functions they don't.

`tools/test_copyover.py BINARY [SCENARIO...]` runs real servers and clients through copyovers: players coming back with their rooms and objects, switched immortals, builders in editors, link-dead players, wizlock and command line options surviving, a missing binary, damaged copyover files, and the deadlock timer. `ctest` runs it, under the `slow` label.

`world_audit()` (`dbat/world_audit.hpp`) checks that the game's lists agree with each other. The tests call it after every step, `dbat -c` runs it after loading the world, and admins can run it in a live game with the `audit` command.

A test can pin the clock with `g->pinned_time`, which is what `game_now()` then returns. Game code should call `game_now(g)` and not `time(0)`.
