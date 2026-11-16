# CLAUDE.md

Project rules (overview, build, architecture rules, tests, coding standards) live in AGENTS.md and must be followed:

@AGENTS.md

Reference docs (current gameplay, module details): [docs/index.md](docs/index.md). Read the relevant page before working on a module.

## Commands

Run from an MSYS2 UCRT64 environment (commands in AGENTS.md). Extras:

```
build/project-mine-tests.exe --gtest_filter=Suite.Case   # single test (wildcards allowed: Suite.*)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug    # debug build (default is Release)
```

No linter is configured; the compiler runs with `-Wall -Wextra`.

## Build layout (CMakeLists.txt)

- Sources are picked up by `GLOB_RECURSE ... CONFIGURE_DEPENDS` over `src/*.cpp` and `tests/*.cpp`: new files need no CMake edits.
- The test executable compiles **all of `src/` except `src/main.cpp`** together with `tests/`. Keep only `main()` in `src/main.cpp`.
- Both targets share `setup_target()`: `src/` and `include/` (as SYSTEM) on the include path, raylib + `opengl32 gdi32 winmm`, fully `-static` link.
- GoogleTest is the MSYS2 package, found next to the compiler (`<compiler>/../lib`), not fetched.
- enet and tmxlite are bundled but **not linked yet**. To use them, add `tmxlite` / `enet ws2_32` to `setup_target()` (and copy `lib/libenet-7.dll` next to the exe for enet).
- `lib/` is passed to the linker as a relative path (see AGENTS.md). Don't replace it with an absolute path.
