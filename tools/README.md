# tools/

Developer checks for refactoring the server. None of this is part of the server build.

| script | what it checks |
|---|---|
| `verify.sh [ref]` | Behaviour against a git ref (default `HEAD`). Both builds must give the same `dbat -c` result and log, the same boot log, and byte-identical saved data after loading and re-saving every player and every zone's mobs. The RNG is seeded with a fixed value, because mob stats are rolled at boot. Each build runs on its own copy of `data/`. |
| `objdiff.sh [ref]` | Generated code against a git ref. Each function's normalised disassembly is compared, matched by name across files, so moving functions between files doesn't count as a change. Use it to show a refactor changed no code. For deliberate renames, set `OBJDIFF_RENAME="old=new,..."`. |
| `deadcode.sh` | Lists functions that `main()` can never reach, via the linker's `--gc-sections` report. Code disabled by a constant condition (`if (false && ...)`) is invisible to it, so the compiler is the final check when you delete something. |
| `check-headers.sh` | Every header under `libs/dbat/include` compiles on its own. |

- **Shared pieces:**
  - `build-tree.sh` is the shared builder.
  - `roundtrip.c` holds `roundtrip_dump()`, which `verify.sh` calls from gdb.
- **Where they work:** the scripts build in `$TMPDIR` and need `gcc`, `gdb`, `objdump` and `python3`.
- **Typical loop:** make a change, run `tools/objdiff.sh` if it should be code-neutral, and run `tools/verify.sh` before committing.
