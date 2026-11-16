# Testing

`tests/` mirrors the `src/` modules (`tests/ui/`, `tests/persistence/`, `tests/scene/menu/`, ...). The test executable compiles all of `src/` except `src/main.cpp`.

Shared helpers live in `tests/support/` and are included as `"support/..."`:

- `fake_canvas.hpp`: a recording `Canvas` for widget tests.
- `fake_keyboard.hpp`: a `Keyboard` whose pressed keys, held keys and typed characters the test sets.
- `fake_pointer.hpp`: a `Pointer` whose position and release the test sets.
- `fake_input.hpp`: a world-bindings `InputHandler` over a `FakeKeyboard`, for the world model and its systems.
- `temp_dir.hpp`: a temporary directory for persistence tests.

Rules for writing tests (Given/When/Then, only meaningful tests) are in [AGENTS.md](../AGENTS.md#tests).
