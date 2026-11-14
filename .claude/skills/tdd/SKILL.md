---
name: tdd
description: Test-driven development workflow for this C++/raylib game - write the failing GoogleTest first (red), implement the minimum to pass (green), then refactor. Use this skill whenever the user asks to create new code, add a feature, system, class, function or game rule, change how existing code behaves, or fix a bug, even if they never mention tests or TDD. Any request that results in new or changed behaviour in src/ goes through this skill before the first line of production code is written.
---

# TDD: red, green, refactor

Every new or changed behaviour in this project is driven by a test written before the production code. The point is not ceremony: a test written first proves it can fail, pins down the behaviour before the implementation biases you, and forces game logic into a shape that can run without a window.

Project conventions (build commands, Given/When/Then, naming, smart pointers, early return) are in `AGENTS.md` and `CLAUDE.md`. Follow them; this skill only adds the order of work.

## When this applies

Applies to anything that adds or changes behaviour: new systems, new rules, changed calculations, state transitions, bug fixes.

Does not apply to changes with no behaviour to assert: pure renames or moves, comments, docs, build configuration, asset changes. For a pure refactor, run the existing suite before and after instead of writing new tests.

Code that only draws or talks to raylib (window, input polling, rendering) cannot be asserted meaningfully. Do not write a fake test for it. Instead, pull the decision out of it into plain code, test that, and leave the raylib call as a thin shell. Example: test `belt.item_positions()` and let the draw function just loop over the result. If a request is 100% rendering with no logic to extract, say so and skip the cycle rather than inventing a trivial test.

## The cycle

Work in small steps: one behaviour per cycle, not the whole feature at once.

### 1. List the behaviours

Before writing anything, write down in the reply the behaviours the request implies as short sentences: the main rule, the boundaries, the invalid cases. "A stack accepts items up to MaxStackSize", "adding beyond the limit returns the overflow", "adding zero changes nothing". This list is the plan; each line becomes one test. If the request is ambiguous about a rule (what happens on overflow?), ask or state the assumption here, because the test is about to turn it into a spec.

### 2. Red: write the test and watch it fail

Write the test for the first behaviour in `tests/<unit>_test.cpp`, in Given / When / Then form with PascalCase suite and case names.

Then get to a real red in two moves:

1. Add only what the test needs to compile: the header with declarations and a stub body that returns a wrong default (`return 0;`, `return {};`). No logic. A compile error alone is a weak red, because it also breaks every other test in the single test executable and does not show the assertion can fail.
2. Build and run the test. It has to fail on its assertion, with a message that matches the behaviour that is missing.

```
cmake --build build
build/project-mine-tests.exe --gtest_filter=Suite.Case
```

Read the failure. If the test passes at this point, it is not testing the new behaviour: either the behaviour already exists or the assertion is wrong. Fix the test before going on. Never move to implementation without having seen the failure output.

### 3. Green: minimum implementation

Write the simplest production code that makes this test pass. Do not implement behaviours that have no test yet; they get their own cycle. Build and run the whole suite, not just the new test, so regressions show up now:

```
cmake --build build
ctest --test-dir build --output-on-failure
```

If something fails, fix the production code. Only change the test when the test itself was wrong, and say so explicitly.

### 4. Refactor

With everything green, clean up names, duplication and structure in both production and test code, then run the suite again. Skip if there is nothing to improve.

### 5. Next behaviour

Go back to step 2 with the next line of the list until the list is done.

## Changing existing behaviour and fixing bugs

- Changed rule: first update or add the test that describes the new rule and watch it fail against the current code. Then change the code. Existing tests that encoded the old rule are updated in the red step, not afterwards to make things pass.
- Bug: first write a test that reproduces the bug and fails. Then fix. The test stays as the regression guard.

## What a meaningful test is

A test earns its place when it would catch a real mistake in a game rule. Aim for rules, calculations, state transitions and edge cases (zero, limit, limit plus one, empty, invalid input). Leave out getters and setters, constructors that just store values, and library behaviour. Assert through the public interface on observable results, not on private state or on how the implementation works, so the test survives refactoring. One behaviour per test; the name says the behaviour (`AddingBeyondStackLimitReturnsOverflow`), not the method.

## If the build cannot run

The red and green checks need a real run. If the build environment is unavailable (MSYS2 UCRT64 not on the path, configure failing), say so plainly and stop to sort that out instead of claiming red or green from reading the code.

## Report

End with a short summary so the user can see the cycle happened:

```
Behaviours: <the list, each marked done>
Red: <test name> failed with <the key line of the failure>
Green: <N> tests passed, 0 failed
Files: <tests and sources touched>
```
