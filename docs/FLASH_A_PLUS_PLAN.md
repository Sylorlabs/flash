# Flash A+ Plan — Compile-Time Safety Without Sacrificing Control

**Status:** Planning document. This is the long-running plan to bring Flash from
its current state (B+ on a good day) to A+ — where every issue class that C and
Rust catch is also caught by Flash, without sacrificing the low-level control
that photonic hardware programming requires.

**Companion:** [`FLASH_2_0_PLAN.md`](FLASH_2_0_PLAN.md) (W1-W12, completed),
[`FLASH_2_0_SPEC.md`](FLASH_2_0_SPEC.md)

---

## 0. Current state assessment

### What Flash already catches (B+ tier)

| Issue class | Status | How |
|-------------|--------|-----|
| Photonic effect violations | ✅ Caught | `@photonic` required, F0003 |
| Unit type mixing (mw/rad) | ✅ Caught | Distinct struct types |
| Radix wraparound (t vs i) | ✅ Caught | Cast required across radix |
| Tritvec width mismatch | ✅ Caught | Sema width check |
| Unknown identifiers | ✅ Caught | F0001 |
| Type mismatches | ✅ Caught | F0001 |
| Generic instantiation | ✅ Caught | F0005 |

### What Flash lets through (the gaps)

| Issue class | Current behavior | Severity | Who catches it |
|-------------|-----------------|----------|----------------|
| **Memory leaks** (L0012) | Warning only, 58 in self-host | P0 | Rust (borrow checker) |
| **Ignored return values** (A0102) | Warning only, 15 in self-host | P1 | Rust (`#[must_use]`) |
| **Off-by-one indexing** (A0101) | Warning only, 3 in self-host | P1 | Rust (bounds checks) |
| **Null dereference** | Not caught | P0 | Rust (Option), Zig (runtime) |
| **Switch non-exhaustiveness** | Not caught | P1 | Rust (match exhaustiveness) |
| **Use-after-free** | Not caught | P0 | Rust (borrow checker) |
| **Double-free** | Not caught | P0 | Rust (ownership) |
| **Uninitialized memory** | Partially caught | P1 | Rust (init required) |
| **`--check` on self-host** | 96 F0001 errors | P0 | (self-host can't check itself) |

### The core tension

"Compile-time memory safety without sacrificing control" is the Rust thesis.
The reason it's hard is that the borrow checker restricts *aliasing patterns*,
not just *types*. Flash can get most of the way there with a staged approach:

1. **Strict warnings → errors** (no language change, no control cost)
2. **Null safety** (language change, no control cost)
3. **Switch exhaustiveness** (language change, no control cost)
4. **Ownership via linear types** (language change, some control cost)
5. **Borrow checking** (language change, real control cost — the Rust tradeoff)

Steps 1-3 are clear wins. Step 4 is the decision point. Step 5 is the research
question. This plan covers 1-4 in detail and leaves 5 as a post-A+ research
direction.

---

## Phase 1: Strict mode and self-host `--check` (P1, no language change)

**Goal:** Make the existing warnings into errors, and make `flashc1 --check`
pass on its own source. This is the foundation — you can't claim A+ if your
own compiler can't check its own code.

### S1 — Fix self-host `--check` (96 F0001 errors)

**Problem.** `flashc1 --check` produces 96 errors on its own source because
the sema doesn't know about Zag-defined struct fields (Mat3, Beam, Wave, Trit)
and doesn't handle `@sizeOf`, `@strEq` as call expressions, and several other
builtin patterns.

**Root causes (by count):**
- 52 `unknown field` — Mat3 (27), Trit (15), Beam (6), Wave (2), void (2)
- 17 `unknown identifier` — variables from `@sizeOf` casts, `cell`, `lane`, `phys`
- 13 `incompatible operand` — `@sizeOf__Type` treated as unknown call
- 7 `struct field initializer` — void-typed fields from failed inference
- 6 `type mismatch` — `*i8` from `@sizeOf` cast chain

**Approach.**
1. Register Zag-defined structs (Mat3, Beam, Wave, Trit, etc.) in the sema
   with their fields. Either:
   - (a) Parse `flash/types/*.zag` struct definitions and register them, or
   - (b) Add a "builtin types" table in `fsema.flash` with hardcoded field lists.
   Option (b) is simpler and sufficient — the Zag structs are stable.
2. Handle `@sizeOf`, `@strEq`, `@alignOf` as builtin calls with known return
   types (`i32`/`i64`), not as unknown identifiers.
3. Fix the `void` inference cascade — when `@sizeOf` returns void (because the
   call isn't recognized), downstream field accesses on the result fail.

**Files touched.**
- `selfhost/fsema.flash` — `fs_register_decl` (add builtin struct registration),
  `fs_check_expr` (handle `@sizeOf` etc.), `fs_builtin_eff` (add builtins)
- `selfhost/fsema_builtins.flash` (new) — builtin type/field tables

**Tests gating S1.**
- `flashc1 --check` passes with 0 errors on `selfhost/flashc1.flash`
- All 91 existing tests still pass
- New test: `tests/check/self_check.flash` — `flashc1 --check` as a test

**Critic gate C1:** Does `--check` pass on ALL self-host sources, not just
`flashc1.flash`? Run `flashc1 --check` on every `.flash` file in `selfhost/`.

### S2 — Promote L0012 (leak) warnings to errors in strict mode

**Problem.** 58 string buffer leaks in the self-host compiler. These are real
memory bugs — `_zag_str_concat` results dropped without `_zag_str_free`.

**Approach.**
1. Add `--strict-leaks` flag (or make `--analyze-strict` cover L0012).
2. Fix all 58 leaks in `selfhost/*.flash` by binding intermediates to `let`
   and freeing them, or using leak-free helpers.
3. Once fixed, make L0012 an error by default in `--check` mode.

**Files touched.**
- `selfhost/fsema.flash` — leak detection (already exists in Zag analyzer)
- `selfhost/*.flash` — fix all 58 leak sites
- `selfhost/femit.flash` — may need leak-free string helpers

**Tests gating S2.**
- `flashc1 --check --strict-leaks` passes on self-host sources
- `tests/safety/no_leaks.flash` — a program with a deliberate leak is rejected

**Critic gate C2:** Are there leak patterns the analyzer doesn't catch? Write
5 adversarial leak patterns and verify all are caught.

### S3 — Promote A0101 (off-by-one) and A0102 (ignored return) to errors

**Problem.** 3 off-by-one warnings, 15 ignored return values. These are
agent-bug patterns — silent correctness issues.

**Approach.**
1. Fix the 3 A0101 sites (change `<=` to `<` in indexing loops).
2. Fix the 15 A0102 sites (bind ignored return values or explicitly discard).
3. Make A0101 and A0102 errors in `--check` mode.

**Files touched.**
- `selfhost/*.flash` — fix all 18 sites

**Tests gating S3.**
- `flashc1 --check` passes with 0 A0xxx warnings
- `tests/safety/no_off_by_one.flash` — rejected
- `tests/safety/no_ignored_return.flash` — rejected

**Critic gate C3:** Run the full test suite with `--analyze-strict`. Does
anything fail that shouldn't? Are there false positives?

---

## Phase 2: Null safety (P2, language change, no control cost)

**Goal:** Eliminate null pointer dereferences at compile time. `null` is only
legal in `?T` contexts. Raw pointers `*T` must be initialized.

### S4 — Make `null` illegal outside `?T`

**Problem.** Currently `null` is a valid value for any pointer type. This
means `let p: *i32 = null; *p` compiles and crashes at runtime.

**Approach.**
1. `null` is only valid as a value of type `?T` (optional).
2. `*T` pointers must be initialized with a valid address.
3. To check if an optional has a value, use `orelse` or `if (opt) |val|`.
4. To set an optional to null: `opt = null` (only if `opt: ?T`).
5. Null comparison (`p == null`) is only legal if `p: ?T`.

**Surface.**
```flash
let p: *i32 = &x;       // ok — initialized
let opt: ?*i32 = null;  // ok — optional
let bad: *i32 = null;   // ERROR — null not valid for *T
if (opt) |val| {        // ok — unwrap optional
    *val = 42;
}
```

**Files touched.**
- `selfhost/fsema.flash` — `fs_check_expr` (null literal type), `fs_check_stmt`
  (assignment type checking), `fs_check_binary` (null comparison)
- `selfhost/fparse.flash` — no change (null already parsed)
- `selfhost/femit.flash` — no change (null still emits as null)

**Tests gating S4.**
- `tests/safety/null_ok.flash` — valid null usage in `?T` contexts
- `tests/safety/null_violation.flash` — `*T = null` is rejected
- `tests/safety/null_deref.flash` — dereferencing `?T` without unwrap is rejected

**Critic gate C4:** Can you still write a linked list? A tree? Verify that
common data structures that use null sentinels still work via `?T`.

### S5 — Optional unwrap enforcement

**Problem.** Even with null restricted to `?T`, you can still dereference an
optional without checking: `let opt: ?*i32 = null; *opt` should be a compile
error.

**Approach.**
1. Dereferencing `*T` is legal.
2. Dereferencing `?T` is illegal — must unwrap first.
3. Unwrap syntax: `if (opt) |val| { ... }` or `opt orelse fallback`.
4. `try` on `!T` already works this way (error propagation). Apply the same
   pattern to `?T`.

**Files touched.**
- `selfhost/fsema.flash` — `fs_check_unary` (deref type check)

**Tests gating S5.**
- `tests/safety/optional_unwrap_ok.flash` — valid unwrap patterns
- `tests/safety/optional_unwrap_violation.flash` — direct deref of `?T` rejected

**Critic gate C5:** Does every `?T` in the self-host compiler get properly
unwrapped? Audit all optional usage in `selfhost/*.flash`.

---

## Phase 3: Switch exhaustiveness (P3, language change, no control cost)

**Goal:** `switch` on an `enum` must cover all variants (or have an `else` arm).
Non-exhaustive switches are a common bug source.

### S6 — Exhaustive switch checking

**Problem.** Currently `switch` doesn't check that all enum variants are
covered. If you add a new variant to an enum, existing switches silently
fall through.

**Approach.**
1. If the switch expression has a known enum type, collect all variants.
2. Check that every variant appears as a tag in some arm, OR that an `else`
   arm exists.
3. If neither, emit F0006: "non-exhaustive switch: missing variants X, Y".
4. If the switch expression type is not an enum (e.g., `i32`), require an
   `else` arm (can't enumerate all integers).

**Surface.**
```flash
enum Color { Red, Green, Blue }

fn name(c: Color) []u8 {
    switch (c) {
        Red => "red",
        Green => "green",
        // ERROR F0006: non-exhaustive switch: missing Blue
    }
}

fn name_ok(c: Color) []u8 {
    switch (c) {
        Red => "red",
        Green => "green",
        Blue => "blue",   // ok — exhaustive
    }
}

fn name_else(c: Color) []u8 {
    switch (c) {
        Red => "red",
        else => "other",  // ok — else covers rest
    }
}
```

**Files touched.**
- `selfhost/fsema.flash` — new function `fs_check_switch_exhaustive`
- `selfhost/fparse.flash` — no change (switch already parsed)

**Tests gating S6.**
- `tests/safety/switch_exhaustive_ok.flash` — exhaustive switch passes
- `tests/safety/switch_non_exhaustive.flash` — missing variant rejected
- `tests/safety/switch_else_ok.flash` — else arm passes for any type

**Critic gate C6:** Add a new variant to an existing enum in the self-host
compiler. Does every switch on that enum get flagged? Verify the error
message names the missing variant.

---

## Phase 4: Initialization safety (P4, language change, no control cost)

**Goal:** Variables must be initialized before use. No reading uninitialized
memory.

### S7 — Definite assignment analysis

**Problem.** Currently `let x: i32;` is legal and `x` is uninitialized. Reading
`x` before assignment is undefined behavior.

**Approach.**
1. `let x: i32;` declares an uninitialized variable.
2. Track assignment in the sema: a variable is "definitely assigned" after an
   assignment statement, or after an `if/else` where both branches assign.
3. Reading an uninitialized variable is F0007: "use of possibly uninitialized
   variable `x`".
4. `let x: i32 = 0;` is always safe (initialized at declaration).
5. Function parameters are always initialized (they're arguments).

**Surface.**
```flash
let x: i32;        // ok — declared, not initialized
x = 5;             // ok — now initialized
println(x);        // ok — definitely assigned

let y: i32;
if (cond) { y = 1; } else { y = 2; }
println(y);        // ok — both branches assign

let z: i32;
if (cond) { z = 1; }
println(z);        // ERROR F0007 — only one branch assigns
```

**Files touched.**
- `selfhost/fsema.flash` — new function `fs_check_definite_assignment`,
  track per-variable assignment state in `FsCtx`

**Tests gating S7.**
- `tests/safety/init_ok.flash` — all valid initialization patterns
- `tests/safety/uninit_use.flash` — reading uninitialized variable rejected
- `tests/safety/partial_init.flash` — one-branch assignment rejected

**Critic gate C7:** Can you still write a pattern where a variable is
initialized in a loop and read after? (`while (true) { x = 1; break; }`)
Verify this doesn't false-positive.

---

## Phase 5: Memory safety via linear types (P5, language change, some control cost)

**Goal:** Catch use-after-free, double-free, and leaks at compile time using
linear/affine types. This is the decision point — it adds *some* restrictions
on how you write code, but doesn't restrict aliasing (unlike a borrow checker).

### S8 — Affine ownership for heap allocations

**Problem.** `_zag_str_free`, `_zag_free` can be called on already-freed
pointers. Allocations can be leaked (never freed). This is the L0012 class
but at the language level, not the analyzer level.

**Approach.** Affine types: a value can be used *at most once*. This is weaker
than linear (exactly once) — it allows dropping (leaking is still possible but
detectable), but prevents double-use (double-free, use-after-free).

1. Values returned by `_zag_malloc`, `_zag_str_concat` (allocating functions)
   are "owned". Owned values can be:
   - Passed to a consuming function (e.g., `_zag_str_free`, or a function that
     takes ownership).
   - Returned from the current function (transferring ownership).
   - Stored in a struct field (the struct now owns it).
2. An owned value can only be used ONCE. After it's consumed, it's dead.
3. Reading a dead variable is F0008: "use of moved value `x`".
4. Calling `free` twice on the same value is caught (second use is dead).
5. Leaking (never consuming) is a warning, not an error (affine, not linear).

**What this does NOT restrict (the control preserved):**
- You can still have multiple raw pointers to the same data (aliasing is fine).
- You can still do pointer arithmetic.
- You can still manually manage memory.
- You can still call hardware directly.
- You can still have untyped `raw { }` blocks (in `@unsafe` mode).

**What this DOES restrict:**
- You can't use a value after freeing it.
- You can't free a value twice.
- You can't forget to consume an owned value (warning, not error).

**Surface.**
```flash
let s: []u8 = _zag_str_concat("a", "b");  // s is owned
_zag_str_free(s);                          // ok — consumed
_zag_str_free(s);                          // ERROR F0008 — use of moved value

let s2: []u8 = _zag_str_concat("x", "y");  // s2 is owned
// function ends without consuming s2 — WARNING L0020 — owned value leaked
```

**Files touched.**
- `selfhost/fsema.flash` — ownership tracking in `FsCtx`, `fs_check_expr`
  (mark consumed), `fs_check_stmt` (check for dead variable use)
- `selfhost/fparse.flash` — no change
- `selfhost/femit.flash` — no change (ownership is compile-time only)

**Tests gating S8.**
- `tests/safety/ownership_ok.flash` — valid own + consume patterns
- `tests/safety/double_free.flash` — double free rejected
- `tests/safety/use_after_free.flash` — use after free rejected
- `tests/safety/ownership_transfer.flash` — ownership via return and struct fields

**Critic gate C8:** Can you still write a doubly-linked list? A graph with
cycles? Verify that aliasing patterns still work (multiple pointers to same
allocation, only one is the owner). This is the key test for "without
sacrificing control."

### S9 — Ownership-aware string helpers

**Problem.** The 58 L0012 leaks in the self-host compiler are mostly from
nested `_zag_str_concat` calls. With affine ownership, these become warnings
(leaked owned values). But the fix is the same: use leak-free helpers.

**Approach.**
1. Create `s3` (string stack) helpers in `selfhost/std/str.flash`:
   - `s3_new() S3` — create a string builder
   - `s3_append(s: *S3, str: []u8) void` — append (consumes str if owned)
   - `s3_finish(s: S3) []u8` — produce final string (transfers ownership)
   - `s3_free(s: S3) void` — free the builder
2. Replace all nested `_zag_str_concat` in `selfhost/*.flash` with `s3` calls.
3. The ownership system verifies that every `s3_new` has a matching `s3_free`.

**Files touched.**
- `selfhost/std/str.flash` — `s3` implementation
- `selfhost/*.flash` — replace concat chains with `s3`

**Tests gating S9.**
- `flashc1 --check` passes with 0 L0012 warnings
- `tests/safety/s3_builder.flash` — s3 usage patterns

**Critic gate C9:** Are there string-building patterns that `s3` can't handle?
Verify that the self-host compiler's string emission still produces
byte-identical output.

---

## Phase 6: Integration and A+ gate (P6)

### S10 — Strict mode integration

**Goal.** `flashc --strict` enables all safety checks:
- L0012 (leaks) → error
- A0101 (off-by-one) → error
- A0102 (ignored return) → error
- F0006 (non-exhaustive switch) → error
- F0007 (uninitialized use) → error
- F0008 (use of moved value) → error
- Null safety enforced
- Optional unwrap enforced

**Approach.**
1. `--strict` flag sets all safety checks to error mode.
2. `--check` implies `--strict` (checking should be strict by default).
3. `--unsafe` flag disables safety checks (for `raw { }` blocks and hardware
   code that intentionally does unsafe things). This is the escape hatch that
   preserves control.

**Files touched.**
- `selfhost/flashc1.flash` — CLI flag handling
- `selfhost/fsema.flash` — strict mode propagation

**Tests gating S10.**
- `flashc1 --check --strict` passes on all self-host sources
- `tests/safety/unsafe_escape.flash` — `@unsafe` block allows raw operations
- Full test suite passes in strict mode

**Critic gate C10 (A+ gate):** The final gate. All of the following must hold:
1. `flashc1 --check --strict` passes on every `.flash` file in `selfhost/`
2. All 91+ existing tests pass
3. Bootstrap fixpoint holds (self-compiled compiler is byte-identical)
4. Every issue class in the "gaps" table is caught at compile time
5. `@unsafe` escape hatch works for hardware-direct code
6. No false positives on legitimate patterns (linked lists, trees, graphs)

---

## Dependency graph

```
S1 (self-check) ──> S2 (leak errors) ──> S3 (A0xxx errors)
                                              │
                                              v
S4 (null safety) ──> S5 (optional unwrap)     │
                                              │
S6 (switch exhaust) ──────────────────────────┤
                                              │
S7 (definite assignment) ─────────────────────┤
                                              │
S8 (affine ownership) ──> S9 (s3 helpers) ────┤
                                              │
                                              v
                                    S10 (strict mode + A+ gate)
```

- S1, S2, S3 are sequential (each builds on the previous).
- S4-S5, S6, S7, S8-S9 can run in parallel after S3.
- S10 is the final integration.

---

## Critique gates summary

| Gate | When | What it checks |
|------|------|----------------|
| C1 | After S1 | `--check` on ALL self-host sources |
| C2 | After S2 | 5 adversarial leak patterns caught |
| C3 | After S3 | No false positives in `--analyze-strict` |
| C4 | After S4 | Linked lists, trees still workable |
| C5 | After S5 | All `?T` in self-host properly unwrapped |
| C6 | After S6 | New enum variant flags all switches |
| C7 | After S7 | Loop-then-break doesn't false-positive |
| C8 | After S8 | Doubly-linked lists, graph cycles workable |
| C9 | After S9 | Byte-identical output from s3 refactor |
| C10 | After S10 | **A+ gate** — all gaps closed, no false positives, fixpoint holds |

---

## What this plan does NOT include (post-A+ research)

### Borrow checking (S11, not planned)

A full borrow checker (Rust-style) would add:
- Lifetime annotations on references
- Aliasing restrictions (one mutable XOR many immutable)
- Zero-cost compile-time data race detection

This is explicitly **not** in the A+ plan because:
1. It's a multi-year research effort.
2. It changes how people write code (the "fighting the borrow checker" experience).
3. The affine ownership in S8 catches the critical bugs (UAF, double-free) without
   restricting aliasing.
4. Flash's target users (photonic hardware programmers) need aliasing for
   hardware register access.

If the affine approach proves insufficient in practice, S11 is the next step.

### Compile-time function execution (S12, not planned)

`comptime` blocks (Zig-style) would allow:
- Compile-time code generation
- Reflection
- Macro-like metaprogramming

Not in scope. Flash's `const` (W11) covers constant folding. Full comptime is
a separate language design question.

### Concurrency safety (S13, not planned)

Data race detection (like Rust's Send/Sync) is not in scope. Flash's photonic
hardware context doesn't have threads in the traditional sense — the "parallelism"
is in the hardware (photonic beams), not in software threads.

---

## Estimated effort

| Phase | Workstreams | Effort | Risk |
|-------|-------------|--------|------|
| Phase 1 (strict mode) | S1, S2, S3 | Medium | Low — no language change |
| Phase 2 (null safety) | S4, S5 | Medium | Medium — breaking change |
| Phase 3 (exhaustiveness) | S6 | Low | Low — additive |
| Phase 4 (init safety) | S7 | Medium | Medium — flow analysis |
| Phase 5 (ownership) | S8, S9 | High | High — language semantics |
| Phase 6 (integration) | S10 | Low | Low — integration |

Phase 5 is the decision point. If affine ownership proves too restrictive in
practice (C8 fails), the plan stops at Phase 4 and Flash is "A" tier (catches
everything except memory safety). If C8 passes, Flash is "A+" tier.
