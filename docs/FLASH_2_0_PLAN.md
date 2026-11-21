# Flash 2.0 — A Balanced-Ternary, Photonic-Native Language

**Status:** Design proposal. Not yet implemented.
**Author:** Devin, reviewed with Micah.
**Scope:** Full language redesign. Multi-day, full-coverage. This document is
the contract: every change is named, motivated, scoped, and ordered. Nothing
in here is "and then a miracle occurs."

---

## 0. The thesis

Flash 1.0 is **Zag with trits bolted on**. That was the right bootstrap — it
got a working self-hosted compiler in weeks. But it is the wrong *language*.
Flash 1.0's identity crisis shows up in three places:

1. **Two dialects.** `program`-files (declarative, no control flow, equation
   style) and full-language files (imperative Zig-shaped) share nothing but a
   file extension. `photonic_massive.flash` is 64 hand-unrolled lines because
   the surface DSL has no `for`. Every feature is implemented twice.
2. **The host leaks through.** 1392 calls to `_zag_print` / `_zag_println` /
   `_zag_i64_to_str` across the examples. 1281 `as i64` casts exist *only* to
   feed the Zag runtime printer. Users are constantly reminded they're
   writing Zag.
3. **The photonic API is the least typed part of the language.** `raw { }`
   blocks contain magic identifiers (`fire_shield`, `inject_phase`,
   `set_laser_mw`) with no signatures, no types, no units. The types
   *already exist* in `flash/types/wave_types.zag` (`VoxelCoord`, `BeamId`,
   `WaveSample`, `HorizontalBeam`, `VerticalPipeline`, `POL_OFF/VERT/HORZ`)
   but the surface never sees them.

Flash 2.0 fixes all three by making Flash **a language whose native data is
trits, whose native compute is interference, and whose native I/O is light**.
It is allowed to *resemble* Zag (same family, same author, same effect
annotations, same `fn`/`let`/`struct` shape) — but it must not *be* Zag with
trits, and it must not *be* Zig. The resemblance is syntactic family; the
semantics are ternary-photonic.

This plan is organized into **12 workstreams (W1–W12)**, each with concrete
deliverables, the files it touches, the tests that gate it, and the order it
must run in. Workstreams are grouped into **4 phases** so you can ship
incrementally and never break the build.

---

## 1. Design principles (non-negotiable)

These are the rules every workstream obeys. If a proposal violates one, it
doesn't ship.

1. **Balanced ternary is the home radix.** `t<width>` counts trits. Range is
   symmetric `±(3^w − 1)/2`. Negation is digit-wise and free. There is no
   unsigned `t` family — balanced ternary has no unsigned variant. Binary
   `i*`/`u*` are interop types and pay their own embedding cost on ternary
   hardware. *Inherited from 1.0, kept.*

2. **Native types never pay for their own radix; foreign types pay for
   theirs.** The `trit_encoding_bits` cost model stays. *Inherited, kept.*

3. **One grammar, not two.** The `program`-dialect and the `fn`-dialect
   merge. `program name` becomes an optional module header, not a parser
   switch. Surface trit/wave/raw constructs are legal *inside* `fn` bodies.
   *New in 2.0.*

4. **The photonic API is typed, not raw.** `voxel`, `beam`, `phase`, `laser`,
   `polarity` are first-class types with unit-tagged literals (`mw`, `rad`,
   `axis`). `raw { }` becomes an *escape hatch for untyped register pokes*,
   not the primary way to talk to hardware. *New in 2.0.*

5. **Interference is a first-class operator.** `tritvec[N]` is a vector of
   trits; `@` is balanced-ternary interference (digit-wise convolution);
   `~` is digit-wise negation; `&` `|` `^` are digit-wise trit logic. This
   is the photonic equivalent of SIMD and the place where "light computing"
   actually pays off. *New in 2.0.*

6. **Balanced-ternary digit literals.** `+t++0--+0+` is a 9-trit literal.
   Width is checked at parse time. Users can write what the hardware sees.
   *New in 2.0.*

7. **The stdlib is Flash-native.** No `_zag_*` symbols in user code. `print`
   / `println` accept any numeric type. Format strings exist
   (`print("t9_sum={t}", s)`). The Zag runtime is a codegen detail, never a
   user-visible name. *New in 2.0.*

8. **Type inference at call sites.** `push(&xs, 9)` not `push[i32](&xs, 9)`.
   Method syntax `c.doubled()` not `Counter_doubled(c)`. Mangling is a
   codegen detail. *New in 2.0.*

9. **Unary minus on literals.** `let b: t9 = -9841;`. No more `0 - 9841`.
   *New in 2.0.*

10. **Resemblance to Zag is allowed; resemblance to Zig is incidental.**
    Flash keeps `fn`/`let`/`struct`/`enum`/`union`/`switch`/`orelse`/`catch`
    /`@realtime`/`@total` because Zag uses them and Zag is the sister
    language. Flash does *not* adopt Zig-specific idioms (no
    `std.debug.print`, no `.{ ... }` anon struct syntax, no `comptime`
    keyword — see W11 for Flash's compile-time story).

---

## 2. Phase map

| Phase | Workstreams | Goal | Days (est.) |
|-------|-------------|------|-------------|
| **P1 — Foundation** | W1, W2, W3 | One grammar, Flash-native stdlib surface, unary minus + small fixes | ~1.5 |
| **P2 — Trit-native** | W4, W5 | Balanced-ternary digit literals, `tritvec[N]` + interference ops | ~1.5 |
| **P3 — Photonic-native** | W6, W7, W8 | Typed photonic API, unit literals, `raw {}` demoted to escape hatch | ~1.5 |
| **P4 — Polish + spec** | W9, W10, W11, W12 | Inference, method syntax, compile-time, formal spec, migration | ~1.5 |

**Total: ~6 working days**, but each workstream is independently shippable
and the build stays green between workstreams. The day estimates are for the
*implementation*; the design in this document is the multi-day thorough part
you asked for.

---

## 3. Workstream detail

### W1 — Unify the two dialects into one grammar  *(P1, foundational)*

**Problem.** Today the first token of a file picks a parser:
- `program` → `flash/compiler/parse.zag` (surface, declarative, no control
  flow, equation-style `let trit_add(a,b) = sum`).
- anything else → `flash/compiler/flash_parse.zag` (full language, Zig-shaped,
  `fn`/`if`/`while`).

Consequences:
- `photonic_massive.flash` is 64 hand-unrolled lines because surface has no
  `for`.
- `raw { }` can't sit inside a `fn` that prepares its arguments.
- Every feature is implemented twice (surface codegen in `flash_codegen.zag`,
  full-language codegen in `flash_emit_zag.zag`).
- Two sema paths (`flash_sema_surface.zag` vs `flash_sema.zag`); the
  README admits full-language `--check` is "pending" in stage-1.

**Goal.** One grammar. `program name` is an optional module header (like
Zag's `script;` profile selector, not a parser switch). Surface constructs
(`trit a = +1`, `wave carrier = default`, `optical mat3_mul(...) = out`,
`raw { }`) are legal at top level *and* inside `fn` bodies. The declarative
equation form `let f(a,b) = out` becomes sugar for a `fn` declaration (see
W1.3).

**Files touched.**
- `flash/compiler/parse.zag` (458 lines) — retire or fold into
  `flash_parse.zag`.
- `flash/compiler/flash_parse.zag` (669 lines) — become the only parser;
  absorb surface constructs.
- `flash/compiler/flash_sema_surface.zag` (334 lines) — fold into
  `flash_sema.zag` (927 lines).
- `flash/compiler/flash_codegen.zag` (1271 lines, surface) and
  `flash_emit_zag.zag` (824 lines, full) — merge into one emitter, or keep
  two backends behind a shared IR (see W1.4).
- `selfhost/sparse.flash` (450) + `selfhost/scodegen.flash` (731) — stage-1
  surface path; merge into `selfhost/fparse.flash` (888) + `femit.flash`
  (786).
- `selfhost/slex.flash` (153) — fold into `selfhost/flex.flash` (411).

**Substeps.**

**W1.1 — Define the unified grammar (EBNF).** Write
`docs/FLASH_2_0_GRAMMAR.md` with the full EBNF. Top level is a sequence of
declarations: `fn`, `struct`, `enum`, `union`, `let` (top-level binding),
`trit`/`wave`/`p32`/`sat_i16` (typed top-level bindings, sugar for `let
x: T = ...`), `optical` (photonic declaration, see W6), `raw { }` (now legal
at top level *and* as a statement), `error { }`, `@import`, `@annotation`.
`program name` is an optional first line that sets the module name and
enables photonic defaults (see W6.2). Inside `fn` bodies: same statement set
as today plus `raw { }` blocks and typed photonic calls.

**W1.2 — Make `let f(a,b) = out` sugar for `fn`.** Today's surface equation
`let trit_add(a, b) = sum_ab` declares a *binding* that the codegen matches
against a known `trit_add` primitive. In 2.0 it desugars to:
```flash
fn trit_add(a: trit, b: trit) trit { return <primitive>trit_add(a, b); }
```
The `<primitive>` marker is a codegen hook (W6 covers how primitives bind to
hardware). This lets surface users keep their terse style while the parser
produces one AST. The `out` name on the RHS becomes the return value —
`let f(a,b) = out` means "declare f, its body computes out, return out."

**W1.3 — Make `program` optional.** A file without `program` is a library
module. A file with `program name` is an executable module with photonic
defaults on (auto-import of `flash/photonic` stdlib, `@photonic` effect
allowed in `fn` signatures). This replaces the parser-switch with a
profile selector, exactly the way Zag's `script;` works.

**W1.4 — One codegen path, two backends.** Don't try to merge
`flash_codegen.zag` and `flash_emit_zag.zag` line-by-line on day one.
Instead: introduce `flash/ir/flash_ir.flash` (already exists as a stub) as
the single lowering target. Both surface and full-language parse to the same
IR; the IR lowers to Zag shims (current path) or, later, to a native
ternary backend. This is the *architectural* fix that makes the unification
permanent. W1 ships the IR skeleton + the surface→IR lowering; full-language
moves to IR in W2 once the stdlib surface is clean.

**Tests gating W1.**
- `tests/unify/round_trip.flash` — a file that uses `program`, `fn`, `trit`,
  `wave`, `raw { }`, `for`, and `let f(a,b) = out` in the same compilation.
  Must compile and run.
- `tests/unify/photonic_massive_loop.flash` — `photonic_massive.flash`
  rewritten as a `for` loop. Output must be byte-identical to the current
  hand-unrolled version (golden manifest comparison).
- All existing examples must still compile (regression gate).

**Risk.** This is the biggest mechanical change. Mitigation: do it on a
branch, keep stage-0 (`flashc` in Zag) as the reference, and require
byte-identical manifest output for every existing example before merging.

---

### W2 — Flash-native stdlib surface (kill `_zag_*` in user code)  *(P1)*

**Problem.** 1392 `_zag_print` / `_zag_println` / `_zag_i64_to_str` calls in
examples. 1281 `as i64` casts that exist only to feed the printer. Users
type Zag runtime names. The nicer `flash_fmt_int` / `flash_println_str`
exists in `selfhost/std/fmt.flash` but isn't the default and doesn't cover
all numeric types.

**Goal.** User code never names a `_zag_*` symbol. The Flash stdlib
(`selfhost/std/`) exposes `print`, `println`, `fmt`, and accepts any numeric
type without manual casts. Zag runtime calls are emitted by codegen, never
written by humans.

**Files touched.**
- `selfhost/std/fmt.flash` — extend to cover `trit`, `t9`, `t27`, `i32`,
  `i64`, `f32`, `p32`, `?T`, `!T`, `tritvec[N]` (W5).
- `selfhost/std/io.flash` — become the canonical `print` / `println` /
  `eprint` / `eprintln` surface.
- `flash/stdlib/flash_slim.zag` — keep as the codegen-emitted runtime header
  (it's a codegen detail, not user-facing).
- All `examples/*.flash` — rewrite to use the new surface.
- `flash/compiler/flash_emit_zag.zag` — emit calls to Flash stdlib names,
  not `_zag_*`.

**Substeps.**

**W2.1 — Define the print surface.**
```flash
print("t9_sum="); print(s); println();           // bare
println("x=", x, " y=", y);                      // variadic, any types
println(fmt("t9_sum={t} prod={t}", s, prod));    // format string
```
- `print` / `println` accept any number of args of any printable type.
- `fmt("...", args...)` returns a `[]u8` with `{}`-style holes. Format
  specs: `{t}` trit/t-unit, `{i}` integer, `{f}` float, `{p}` posit, `{s}`
  string, `{v}` tritvec, `{x}` hex, `{$}` raw bytes.
- `println()` with no args prints a newline.
- `eprint` / `eprintln` go to stderr.

**W2.2 — Make codegen emit Flash names.** Today `flash_emit_zag.zag` lowers
`_zag_print("...")` directly. Change it so the *user* writes `println(...)`
and codegen lowers that to whatever the runtime wants (`_zag_print` on the
binary host, native photonic display on ternary hardware). The user never
sees `_zag_*`.

**W2.3 — Type-aware formatting.** `fmt("{t}", s)` where `s: t9` must format
the *balanced-ternary* representation, not `as i64` then decimal. This means
`fmt.flash` needs a `format_tunit(value, width)` that prints the digit
string `+t++0--+0+` (or decimal, gated by a format flag). This is the first
place where the stdlib becomes *ternary-aware*, not just a Zag wrapper.

**W2.4 — Rewrite every example.** Mechanical pass. `gp_demo.flash`'s
```flash
_zag_print("fib10=");
_zag_println(_zag_i64_to_str(f as i64));
```
becomes
```flash
println("fib10=", f);
```
Regression gate: output must be byte-identical to current examples.

**Tests gating W2.**
- `tests/stdlib/fmt.flash` — every format spec, every numeric type.
- `tests/stdlib/no_zag_symbols.flash` — a test that *greps the source* and
  fails if any `_zag_` identifier appears in a `.flash` file outside
  `selfhost/std/` and `flash/stdlib/`. This makes the rule permanent.
- All examples re-pass with byte-identical output.

**Risk.** Variadic `println` needs language support (W10's inference helps
but variadics are separate). Mitigation: ship `print`/`println` with a
fixed-arity set first (1, 2, 3 args), add variadics in W10.

---

### W3 — Small syntax fixes that erode trust  *(P1, quick wins)*

These are tiny but they're the things a user trips on every 5 minutes.

**W3.1 — Unary minus on literals.** `let b: t9 = -9841;`. Today the examples
write `0 - 9841` because unary `-` doesn't parse on literals (or didn't).
Fix the lexer/parser to accept `-` as a prefix operator on numeric literals.
Files: `flash/compiler/flash_lex.zag`, `flash_parse.zag`, `selfhost/flex.flash`,
`selfhost/fparse.flash`. Test: `tests/syntax/unary_minus.flash`.

**W3.2 — Real scoped blocks for `group { }`.** Today `group { }` lowers to
`if (1) { }`. That's a placeholder. Either:
- (a) Make `group` a real scope with shadowing + block-local `defer`, or
- (b) Drop `group` entirely and let normal `{ }` be a scoped block (Zag
  doesn't have `group`; it's a Flash invention that didn't earn its keep).

**Recommendation: (b).** Remove `group`/`group name { }`, document normal
`{ }` as the scoped-block form, keep `defer` as block-scoped. Fewer
keywords, same power. Files: `flash_parse.zag`, `flash_emit_zag.zag`,
`selfhost/fparse.flash`, `selfhost/femit.flash`. Update
`examples/grouping_demo.flash`.

**W3.3 — `defer` semantics, documented.** Pin down: `defer` runs at block
exit (normal or early return), LIFO order, may not outlive its block. This
matches Zag's `defer`. Write it into the spec (W12).

**W3.4 — Numeric underscore literals, keep.** `1_000_000` and `0x_FF` stay.
Already in 1.0. Just document them in the spec.

**Tests gating W3.** `tests/syntax/unary_minus.flash`,
`tests/syntax/scoped_block.flash`, `tests/syntax/defer.flash`.

---

### W4 — Balanced-ternary digit literals  *(P2, trit-native)*

**Problem.** Today you can spell a single trit (`+1`, `0`, `T`) but for
`t9`/`t27` you write decimal `9841`. Balanced ternary's whole point is
digit-wise ops; users can't write the digit sequence the hardware sees.

**Goal.** A balanced-ternary literal syntax for `t<width>` types.

**Syntax.**
```flash
let a: t9  = t++t+0-+0+;          // 9 trits, MSB first, +/0/-
let b: t9  = t++t+0-+0+t;         // 't' or 'T' allowed for -1
let mask: t9 = t+++++++++;        // all +1
let neg:   t9 = ttttttttt;         // all -1
let zero:  t9 = t000000000;        // all 0
let wide: t27 = t++++++++++++++++++++++++++  // 27 trits
```

Rules:
- Prefix `t` (lowercase) introduces a balanced-ternary literal. (We don't
  use `T` as the prefix because `T` is already the −1 *digit* glyph —
  `tT` would be ambiguous. `t` prefix, `T` digit.)
- Digits are `+` ( +1), `0` (0), `-` or `T`/`t` (−1). Both `-` and `T` are
  accepted for −1 to match 1.0's `trit b = T` idiom.
- The digit count must equal the `t<width>` of the target type. Mismatch is
  a compile error: `let a: t9 = t+++;  // error: 3 trits, expected 9`.
- MSB is leftmost (matches how balanced ternary is conventionally written
  and how the hardware shifts).
- The literal's value is `sum(d_i * 3^(w-1-i))` for `i` in `0..w-1`.

**Files touched.**
- `flash/compiler/flash_lex.zag` — new token kind `TRIT_LITERAL` for
  `t[+0Tt-]+`.
- `flash/compiler/flash_parse.zag` — parse `TRIT_LITERAL` into an integer
  constant tagged with its digit count; sema checks count == width.
- `flash/compiler/flash_sema.zag` — width check + value computation.
- `selfhost/flex.flash`, `selfhost/fparse.flash`, `selfhost/fsema.flash` —
  stage-1 parity.
- `selfhost/std/fmt.flash` — `fmt("{t}", x)` prints the digit form by
  default for `t`-types (W2.3 dependency).

**Tests gating W4.**
- `tests/trit/literals.flash` — every digit combination for t1, t3, t9;
  width-mismatch errors; mixed `+`/`0`/`T`/`-`.
- `tests/trit/literal_value.flash` — assert `t++t+0-+0+` equals the decimal
  computed by the formula.
- `tests/trit/literal_print.flash` — `fmt("{t}", x)` round-trips.

**Why this matters for photonic hardware.** Interference patterns *are*
trit-digit sequences. When a user is designing an optical interference
kernel, they want to write the pattern, not its decimal encoding. This is
the readable form of `tritvec` (W5) literals.

---

### W5 — `tritvec[N]` and the interference operator `@`  *(P2, the big one)*

**Problem.** `photonic_massive.flash` is 64 lines of
`let trit_add(v60, negative) = v61` hand-unrolled because (a) surface has no
`for` (fixed by W1) and (b) there's no vector trit type, so each trit is a
scalar binding. On photonic hardware, a single optical operation acts on a
*vector* of trits at once. The language has no way to express that.

**Goal.** A first-class vector-of-trits type and an interference operator
that maps directly to one optical operation.

**Type.**
```flash
tritvec[N]            // N trits, N is a compile-time constant
tritvec[9] a = t++t+0-+0+;
tritvec[27] b = ...;
```
- `tritvec[N]` is a fixed-size, stack-allocated vector of N trits.
- `N` must be a compile-time constant (W11's compile-time story enables
  parameterized `tritvec[N]` in generics).
- Literal: the `t...` syntax from W4, with the target type being
  `tritvec[N]` instead of `t<N>`. Same digits, same rules. `t9` and
  `tritvec[9]` share literal syntax; the difference is `t9` is a scalar
  integer (arithmetic), `tritvec[9]` is a digit vector (interference).

**Operators.**

| Op | Name | Semantics | Photonic mapping |
|----|------|-----------|------------------|
| `@` | interference | digit-wise balanced-ternary **consensus**: `c[i] = cons(a[i], b[i])` — `+@+=+`, `-@-=-`, `+@-=0`, `0@x=x`, `x@0=x` (see `TRIT_OPS_V1.md` §2.2; frozen semantics — this is the shipped/tested behavior, corrected from an earlier draft that spec'd trit multiplication; numeric multiply stays on `*` / `trit_mul`) | one optical interference op on the vector |
| `~` | negate | digit-wise negation (`+`↔`-`, `0`→`0`) | phase flip, free on hardware |
| `&` | and | digit-wise trit AND (Kleene/min) | polarization merge |
| `\|` | or | digit-wise trit OR (Kleene/max) | polarization merge |
| `^` | xor | digit-wise trit XOR (sum mod 3) | interference differential |
| `==` `!=` | eq | vector compare → `bool` | reduction |
| `[]` | index | `a[i]` → `trit`, `i` is `i32` | voxel read |
| `len` | length | `a.len` → `i32` = N | compile-time constant |

**Example.** `photonic_massive.flash` becomes:
```flash
program photonic_massive

let pos = tritvec[64] of +1;     // 'of X' fills with trit X
let neg = ~pos;                   // digit-wise negate
let zero = tritvec[64] of 0;

let v1 = pos @ neg;               // interference
let v2 = v1 @ pos;
let v3 = ~(v2 & zero);            // nand via & + ~
// ... one line per step, 64 steps in a for loop:
let v = tritvec[64] of 0;
for i in 0..64 {
    v = (v @ neg) @ pos;
    v = ~(v & zero);
}
```
That's the whole 64-line file in 6 lines. The interference operator `@`
collapses the `trit_add`/`trit_mul`/`trit_nand`/`trit_neg` chain into
vector ops the hardware does in one shot.

**Files touched.**
- `flash/types/tritvec.zag` (new) — the type descriptor, storage
  (`storage_bytes = ceil(N/2)` packed, or `N` bytes unpacked for the binary
  host), and primitive ops.
- `flash/ternary/trit.zag` — extend with vector truth tables
  (`tritvec_mul`, `tritvec_neg`, `tritvec_and`, `tritvec_or`, `tritvec_xor`).
- `flash/compiler/flash_lex.zag` — new tokens `@`, `of` (contextual keyword).
- `flash/compiler/flash_parse.zag` — parse `tritvec[N]`, `@`, `~`, `of`.
- `flash/compiler/flash_sema.zag` — type-check vector ops, width matching
  for `@` (both operands same N), `bool` result for `==`.
- `flash/compiler/flash_emit_zag.zag` — lower to loops over
  `tritvec_*` primitives on the binary host; lower to one `@photonic` op
  on ternary hardware (W6 covers the photonic lowering).
- `selfhost/flex.flash`, `selfhost/fparse.flash`, `selfhost/fsema.flash`,
  `selfhost/femit.flash` — stage-1 parity.
- `selfhost/std/fmt.flash` — `fmt("{v}", v)` prints the digit string.

**Photonic lowering (the point of this workstream).** On ternary-photonic
hardware, `a @ b` lowers to *one* optical interference operation: route
`a` and `b` as wave carriers into an interference chamber, read the result
voxel. The manifest emitter (W6) generates one `PathSegment` per `@` op,
not one per trit. This is where "light-based computing" stops being
marketing and becomes a language-level win: the user writes one operator,
the hardware does one op, the manifest is one path.

**Tests gating W5.**
- `tests/tritvec/ops.flash` — every operator on `tritvec[9]`, golden
  outputs.
- `tests/tritvec/photonic_massive.flash` — the rewritten example,
  byte-identical manifest to the hand-unrolled 1.0 version (proves the
  lowering is equivalent).
- `tests/tritvec/width_mismatch.flash` — `a @ b` where `a: tritvec[9]`,
  `b: tritvec[27]` is a compile error.
- `tests/tritvec/index.flash` — `a[i]` reads, `a[i] = t` writes, out-of-
  bounds is a runtime trap (or compile error if `i` is compile-time).

**Risk.** `@` as a token may conflict with existing `@annotation` syntax.
Mitigation: `@annotation` is `@` followed by an identifier; `@` the operator
is `@` between two expressions. The lexer disambiguates by context (after
an expression, `@` is the operator; at statement start, `@` is an
annotation). This is the same trick Zag uses for `@realtime` vs `@` in
expressions. Test it explicitly in `tests/tritvec/at_disambig.flash`.

---

### W6 — Typed photonic API (replace `raw { }` as the primary surface)  *(P3)*

**Problem.** `raw { }` blocks contain magic identifiers with no signatures:
```flash
raw {
    fire_shield(0, 0, 0, 2.5);
    inject_phase(1, 0, 0, 0.785);
    set_laser_mw(0, 0, 3.0);
}
```
The types *already exist* in `flash/types/wave_types.zag` (`VoxelCoord`,
`BeamId`, `WaveSample`, `HorizontalBeam`, `VerticalPipeline`,
`POL_OFF/VERT/HORZ`) and the typed wrappers exist in `flash/wave/raw.zag`
(`fn fire_shield(target: VoxelCoord, mw: f32) !void`). But the surface
`raw { }` bypasses all of it and calls the magic names directly with integer
tuples.

**Goal.** The primary photonic surface is *typed*. `raw { }` becomes an
escape hatch for untyped register pokes, demoted and discouraged.

**Typed surface.**
```flash
program optical_demo

let v = voxel(2, 1, 0);                    // VoxelCoord
let b = beam(wall_n, slot 0);              // BeamId
let p = phase(rad 0.785);                  // Phase
let l = laser(b, mw 1.5);                  // Laser
let pol = polarity(axis_x, 1.0);           // Polarity

fire_shield(v, mw 1.5);                    // typed call, returns !void
inject_phase(v, p);                         // typed, phase is Phase not f32
set_laser(b, mw 2.5);                       // typed, beam is BeamId
target_voxel(v);                            // typed
manual_polarity(v, pol);                    // typed
```

**Unit literals (the key new bit).**
```flash
mw 1.5        // milliwatts, type Power
rad 0.785     // radians, type Phase
axis_x        // enum value, type Axis
axis_y, axis_z
```
Unit literals are *typed*. `fire_shield(v, 1.5)` is a compile error — you
must write `mw 1.5`. This makes the "I passed radians where milliwatts go"
class of bug impossible. The units are part of the type system, not
comments.

**Types (promoted from `flash/types/wave_types.zag` to first-class).**
- `voxel` = `VoxelCoord { x, y, z: i32 }`. Constructor `voxel(x, y, z)`.
- `beam` = `BeamId { wall, slot: i32 }`. Constructor `beam(wall, slot)`.
  `wall_n`/`wall_e`/`wall_s`/`wall_w` are enum values.
- `phase` = newtype over `f32` representing radians. Constructor `rad(x)`.
- `power` = newtype over `f32` representing milliwatts. Constructor `mw(x)`.
- `axis` = enum `{ x, y, z }`.
- `polarity` = struct `{ axis: Axis, value: trit }`. The `value` is a trit
  (`+1`/`0`/`-1`), matching `POL_VERT`/`POL_OFF`/`POL_HORZ`.
- `laser` = `HorizontalBeam` (already in `wave_types.zag`). Constructed by
  `laser(beam, mw)`.

**Files touched.**
- `flash/types/wave_types.zag` — already has the structs; promote them to
  the user-visible surface (re-export through the stdlib).
- `flash/stdlib/photonic.flash` (new) — the user-facing module: `voxel`,
  `beam`, `phase`, `power`, `axis`, `polarity`, `laser`, `fire_shield`,
  `inject_phase`, `set_laser`, `target_voxel`, `manual_polarity`. A
  `program`-module auto-imports this (W1.3).
- `flash/compiler/flash_parse.zag` — parse unit literals `mw`, `rad` as
  typed constructors (contextual keywords).
- `flash/compiler/flash_sema.zag` — type-check unit literals against
  parameter types; `fire_shield(v, 1.5)` is an error.
- `flash/compiler/flash_codegen.zag` / `flash_emit_zag.zag` — lower typed
  calls to the existing `flash/wave/raw.zag` wrappers (which already call
  the `_flash_raw_*` externs). The magic-identifier path is retired.
- `selfhost/lib/hardware_manager.flash` — already ports the spatial side;
  wire it to the typed surface.

**`raw { }` demoted.** `raw { }` stays as an escape hatch for direct
register pokes (e.g. debugging a hardware bug the typed API can't express).
But:
- It requires `@unsafe` annotation on the enclosing `fn` (Zag's effect
  model, applied here).
- It can only name the `_flash_raw_*` externs, not the magic short names.
- The linter (W12) emits a warning whenever `raw { }` is used outside of
  `@unsafe`-annotated code.

**Tests gating W6.**
- `tests/photonic/typed_calls.flash` — every typed call, golden manifest.
- `tests/photonic/unit_mismatch.flash` — `fire_shield(v, 1.5)` is a compile
  error; `fire_shield(v, mw 1.5)` compiles.
- `tests/photonic/raw_demoted.flash` — `raw { }` outside `@unsafe` is a
  warning; inside `@unsafe` is allowed.
- `tests/photonic/manifest_parity.flash` — typed calls produce byte-identical
  manifests to the 1.0 `raw { }` version (proves the lowering is equivalent).

**Why this matters.** This is the change that makes Flash *photonic-native*
instead of *photonic-capable*. The hardware API becomes a typed algebra,
not a string of magic names. The unit literals alone prevent a whole class
of real-world bugs (mixing up radians and milliwatts in a laser command is
the kind of thing that physically breaks hardware).

---

### W7 — Optical matrix and wave carrier as typed expressions  *(P3)*

**Problem.** Today `optical mat3_identity() = id` and
`let mat3_apply(composed, vx, vy, vz) = result` are surface DSL magic. The
`mat3` type exists (`flash/types/mat3.zag`) but the surface syntax is
declarative-only and can't be used inside `fn` bodies.

**Goal.** `mat3` and wave carriers become typed expressions usable anywhere.

**Surface.**
```flash
let id: mat3 = mat3_identity();
let rot: mat3 = mat3_rotation_xy_90();
let composed: mat3 = mat3_mul(rot, id);
let r: tritvec[3] = mat3_apply(composed, vec3(vx, vy, vz));
```
- `mat3` is a first-class type (struct of 9 `f32` / `p32`).
- `vec3` is `tritvec[3]` or a dedicated `vec3 { x, y, z: trit }` — pick one
  (recommendation: `vec3` is `tritvec[3]` with named accessors, so it
  interops with W5).
- `wave carrier` becomes `let carrier: wave = default;` — `wave` is a
  first-class type for a wave carrier (already in `flash/types/wave.zag`).

**Files touched.**
- `flash/types/mat3.zag` — already canonical; promote to user surface.
- `flash/types/wave.zag` — already canonical; promote.
- `flash/compiler/flash_parse.zag` — parse `mat3`, `wave`, `vec3` as types.
- `flash/compiler/matrix_codegen.zag` — lower `mat3_mul` / `mat3_apply` to
  the existing `matrix_rt.zag` `mat3_route()` (nine `PathSegment`s). The
  manifest output stays the same; only the *input* syntax changes.

**Tests gating W7.**
- `tests/photonic/mat3_typed.flash` — the `optical_mat.flash` example
  rewritten with typed `mat3`, byte-identical manifest.
- `tests/photonic/wave_carrier.flash` — `let c: wave = default;` in a `fn`.

---

### W8 — Photonic effect system (`@photonic`, `@realtime`, `@io`)  *(P3)*

**Problem.** Flash 1.0 has `@photonic` and `@realtime` annotations that are
"preserved in emitted shims" but not *checked*. Zag has a real effect
system (`@realtime` proves no alloc/lock/IO; `@total` proves termination).
Flash should inherit Zag's effect model and add a `@photonic` effect for
"this fn issues photonic operations."

**Goal.** Effects are checked at compile time, not just preserved.

**Effects.**
- `@realtime` — no alloc, no lock, no blocking IO. Inherited from Zag.
- `@io` — may do IO (file, network, photonic hardware). Inherited.
- `@photonic` — may issue photonic operations (`fire_shield`, `inject_phase`,
  etc.). New in 2.0. A `fn` that calls `fire_shield` must be `@photonic`
  (or call into something that is). This makes the photonic capability
  transitively visible in a function's signature.
- `@total` — proves termination. Inherited.
- `@noalloc` — no allocation. Inherited.
- `@unsafe` — may use `raw { }` and unchecked pointer ops. Inherited.

**Example.**
```flash
@photonic @realtime
fn pulse(v: voxel, p: power) void {
    fire_shield(v, p);            // ok: @photonic allows this
}
```
A `fn` without `@photonic` that calls `fire_shield` is a compile error.

**Files touched.**
- `flash/compiler/flash_sema.zag` — implement effect checking (Zag's
  `selfhost/analyze.zag` is the reference; port the relevant checks).
- `selfhost/fsema.flash` — stage-1 parity.
- `flash/stdlib/photonic.flash` — the typed photonic fns are declared
  `@photonic @io`.

**Tests gating W8.**
- `tests/effects/photonic_ok.flash` — `@photonic fn` calls `fire_shield`,
  compiles.
- `tests/effects/photonic_violation.flash` — non-`@photonic fn` calls
  `fire_shield`, compile error.
- `tests/effects/realtime_violation.flash` — `@realtime fn` calls `print`
  (which is `@io`), compile error.

---

### W9 — Type inference at call sites + method syntax  *(P4)*

**Problem.** Today generics require explicit type args at every call:
`push[i32](&xs, 9)`, `first_or[i32](xs, -1)`, `Counter_doubled(c)`. The type
is right there in the argument; the user shouldn't have to repeat it. And
methods are mangled to `Type_name`, leaking a codegen detail into user
syntax.

**Goal.**
- Type inference: `push(&xs, 9)` infers `T = i32` from `xs: ArrayList[i32]`.
- Method syntax: `c.doubled()` not `Counter_doubled(c)`.

**Inference.** Monomorphization stays; the compiler just infers the type
args from value args before monomorphizing. Zag's spec already says
"Function type arguments may be explicit or inferred from value arguments"
— Flash 1.0 just didn't implement it. W9 implements it.

**Method syntax.** A declaration `fn (c: Counter) doubled() i32 { ... }`
defines a method. Call site `c.doubled()` lowers to `Counter_doubled(c)`.
The mangling stays as the codegen symbol; the *user* writes `c.doubled()`.
This is purely a parser + sema change; codegen is unchanged.

**Files touched.**
- `flash/compiler/flash_parse.zag` — parse `c.method()` as a method call;
  resolve to `Type_method` symbol.
- `flash/compiler/flash_sema.zag` — infer generic type args from value
  args; resolve method symbols.
- `selfhost/fparse.flash`, `selfhost/fsema.flash` — stage-1 parity.

**Tests gating W9.**
- `tests/types/inference.flash` — `push(&xs, 9)`, `first_or(xs, -1)`.
- `tests/types/method.flash` — `c.doubled()`, `c.hits`.
- `tests/types/inference_ambiguous.flash` — `first_or(empty, -1)` where T
  can't be inferred is a compile error with a clear message.

---

### W10 — Variadic `println` and `fmt` (depends on W2, W9)  *(P4)*

**Problem.** W2 ships `println` with fixed arity (1, 2, 3 args). Real
ergonomics need variadic.

**Goal.** `println("x=", x, " y=", y)` for any number of args.

**Approach.** Variadics in Flash are *typed tuples* lowered to a slice +
per-element formatters. `println(args...)` becomes:
```flash
fn println(args: any...) void {   // any = type-erased, formatted via fmt
    for a in args { print(fmt_value(a)); }
    print("\n");
}
```
`any` is a tagged union of printable types (the set `fmt` supports). This
is a constrained variadic, not C-style `...`. It's the same shape as Zag's
planned variadics.

**Files touched.**
- `flash/compiler/flash_parse.zag` — parse `args...` parameter and
  `f(args...)` call.
- `selfhost/std/fmt.flash` — `fmt_value(any) []u8` dispatch.
- `selfhost/std/io.flash` — variadic `println` / `eprintln`.

**Tests gating W10.** `tests/stdlib/variadic.flash` — 0, 1, 5, mixed-type
args.

---

### W11 — Compile-time (`const`) and parameterized `tritvec[N]`  *(P4)*

**Problem.** `tritvec[N]` needs `N` to be a compile-time constant. W5 ships
with `N` as a literal. Real use needs `const N = 9; tritvec[N]`. Zag v1
explicitly excludes compile-time execution from the portable core; Flash
2.0 needs a *minimal* compile-time story, not full `comptime`.

**Goal.** `const` bindings evaluated at compile time, usable as type
parameters and array sizes. No full `comptime` blocks, no macros, no
reflection — just constant folding with type-level use.

**Surface.**
```flash
const WIDTH: i32 = 9;
const MAX: t9 = t+++++++++;

fn scale(v: tritvec[WIDTH]) tritvec[WIDTH] {   // WIDTH is const
    return v @ tritvec[WIDTH] of +1;
}
```
- `const` bindings are evaluated at compile time (must be literal or
  const-expression).
- `const` usable as `tritvec[N]`, array sizes, and generic type args.
- No `comptime` blocks, no compile-time functions, no reflection. This is
  *constant folding with type-level use*, not Zig's full comptime.

**Files touched.**
- `flash/compiler/flash_parse.zag` — parse `const`.
- `flash/compiler/flash_sema.zag` — evaluate `const` expressions, allow as
  type params.
- `selfhost/fparse.flash`, `selfhost/fsema.flash` — stage-1 parity.

**Tests gating W11.** `tests/const/basic.flash`, `tests/const/type_param.flash`.

---

### W12 — Formal spec, linter, migration guide  *(P4)*

**W12.1 — `docs/FLASH_2_0_SPEC.md`.** The normative language spec, modeled
on Zag's `V1_LANGUAGE_SPEC.md`. Sections: program model, types (t-family,
tritvec, photonic, interop), bindings and control flow, functions/methods/
generics, optionals and errors, photonic API, effects, compile-time,
explicitly-unsupported list, conformance requirements. Every construct in
this plan gets a section.

**W12.2 — `docs/FLASH_2_0_GRAMMAR.md`.** The full EBNF (started in W1.1,
finished here).

**W12.3 — Linter.** Port Zag's lint families (L0xxx leaks, E0xxx
efficiency, B0xxx bloat, A0xxx agent-bugs) plus Flash-specific:
- **P0xxx — Photonic lints.** `raw { }` outside `@unsafe` (warning).
  Mixing `mw` and `rad` in arithmetic (error). `tritvec` ops on
  mismatched widths (error, already in sema). Photonic call in non-
  `@photonic fn` (error, already in effects).
- **T0xxx — Trit lints.** `0 - x` where `-x` works (W3.1 style).
  `as i64` to feed a print (suggest `fmt`). Decimal literal where a trit
  literal exists and the target is `t<width>` (suggestion).

**W12.4 — `docs/FLASH_2_0_MIGRATION.md`.** A 1.0 → 2.0 migration guide with
a mechanical translation table:
| 1.0 | 2.0 |
|-----|-----|
| `_zag_print("x="); _zag_println(_zag_i64_to_str(x as i64));` | `println("x=", x);` |
| `let b: t9 = 0 - 9841;` | `let b: t9 = -9841;` |
| `push[i32](&xs, 9)` | `push(&xs, 9)` |
| `Counter_doubled(c)` | `c.doubled()` |
| `raw { fire_shield(0,0,0,2.5); }` | `fire_shield(voxel(0,0,0), mw 2.5);` |
| `let trit_add(a,b) = sum` (surface) | `fn trit_add(a:trit,b:trit) trit { ... }` or keep sugar |
| 64-line hand-unrolled trit pipeline | `tritvec[64]` + `for` + `@` |
| `group { }` | `{ }` |

Plus a `flashc migrate` command that does the mechanical substitutions
automatically (best-effort, with a diff for review).

**W12.5 — Update README.** Rewrite the README to lead with the 2.0 identity
(ternary-photonic-native), the typed photonic API, `tritvec` + `@`, and the
unified grammar. Move the 1.0 bootstrap history to an appendix.

---

## 4. Dependency graph and order

```
W1 (unify grammar) ──┬──> W2 (stdlib surface) ──> W10 (variadic)
                     │
                     ├──> W3 (small fixes)        [independent, quick]
                     │
                     ├──> W4 (trit literals) ──> W5 (tritvec + @)
                     │
                     ├──> W6 (typed photonic) ──> W7 (mat3/wave typed)
                     │                       └──> W8 (effects)
                     │
                     ├──> W9 (inference + methods)
                     ├──> W11 (const)
                     └──> W12 (spec + linter + migration)  [last]
```

- W1 is the foundation; everything else depends on the unified parser.
- W2, W3, W4 can run in parallel after W1.
- W5 depends on W4 (literals) and W1 (for loops in surface).
- W6, W7, W8 form the photonic phase; W6 first, then W7/W8 in parallel.
- W9, W10, W11 are independent polish; W10 depends on W2.
- W12 is last (it documents everything else).

---

## 5. What does *not* change (the Zag-family resemblance, kept on purpose)

To be explicit about what Flash 2.0 *keeps* from Zag (because it's the
sister language and the resemblance is fine):

- `fn name(params) ReturnType { }` function syntax.
- `let name: Type = expr;` bindings.
- `struct`, `enum`, `union`, `switch` with `|capture|`.
- `?T` optionals, `!T` error unions, `orelse`, `catch |e|`, `try`.
- `error { Name, ... }` error sets.
- `@import("path")` and `@import("path") as name`.
- `@realtime`, `@total`, `@noalloc`, `@io`, `@unsafe` effects (Zag's set).
- `*T` pointers, `[]T` slices, `&place`, `p.*`.
- `as Type` explicit casts.
- `defer` (block-scoped).
- Numeric underscore literals (`1_000_000`).
- The `trit_encoding_bits` cost model.
- The `t<width>` family (t1..t80) and the interop families
  (`i*`/`u*`/`f32`/`f64`/`p*`/`sat_*`/`fixed_*`).

This is the *syntactic family*. The *semantics* — balanced ternary as home
radix, interference as a first-class op, typed photonic API, trit digit
literals, `tritvec` — are Flash's own.

---

## 6. What is *removed* or *demoted* (the Zig-isms and 1.0 warts)

| Removed/demoted | Why | Replacement |
|-----------------|-----|-------------|
| Two-dialect auto-detect | Double implementation cost, no control flow in surface | One grammar (W1) |
| `_zag_*` in user code | Host runtime leaks through | Flash stdlib (W2) |
| `0 - 9841` literal negation | Parser gap | Unary `-` (W3.1) |
| `group { }` / `group name { }` | Lowers to `if (1)`, not a real feature | Normal `{ }` (W3.2) |
| Magic `raw { }` identifiers | Untyped, undocumented | Typed photonic API (W6); `raw { }` demoted to `@unsafe` escape hatch |
| `Counter_doubled(c)` method mangling in user syntax | Codegen detail leaked | `c.doubled()` (W9) |
| `push[i32](&xs, 9)` explicit type args | Type is in the args | Inference (W9) |
| Hand-unrolled trit pipelines | No `for` in surface, no `tritvec` | `tritvec[N]` + `@` + `for` (W1, W5) |
| Decimal-only `t<width>` literals | Can't write what hardware sees | `t++t+0-+0+` digit literals (W4) |

---

## 7. The 2.0 "hello world" (target syntax after all workstreams)

This is what a Flash 2.0 program looks like once everything lands. It
exercises every workstream in one file:

```flash
program optical_engine

@import("std/fmt")
@import("std/photonic")

const WIDTH: i32 = 9

// W4: balanced-ternary digit literal
let seed: t9 = t++t+0-+0+

// W5: tritvec + interference
fn interfere(a: tritvec[WIDTH], b: tritvec[WIDTH]) tritvec[WIDTH] @photonic {
    return a @ b          // one optical interference op
}

// W6: typed photonic API with unit literals
@photonic @realtime
fn pulse(v: voxel, p: power, ph: phase) void {
    fire_shield(v, p)
    inject_phase(v, ph)
}

// W8: effects checked. W9: method syntax + inference.
fn main() i32 {
    let v: voxel = voxel(2, 1, 0)
    pulse(v, mw 1.5, rad 0.785)        // W6 unit literals

    let a: tritvec[WIDTH] = tritvec[WIDTH] of +1
    let b: tritvec[WIDTH] = ~a          // W5 digit-wise negate
    let c: tritvec[WIDTH] = interfere(a, b)

    // W2: Flash-native stdlib, no _zag_*
    println("seed=", seed, " c=", fmt("{v}", c))

    // W7: typed mat3
    let rot: mat3 = mat3_rotation_xy_90()
    let r: tritvec[3] = mat3_apply(rot, vec3(+1, 0, 0))
    println("r=", fmt("{v}", r))

    return 0
}
```

Compare to the 1.0 equivalent (which would be two files — a `program`-dialect
surface for the photonic part and a `fn`-dialect file for the logic, with
`_zag_print` everywhere and `raw { }` for the hardware calls). The 2.0
version is one file, typed end-to-end, and the photonic operations are
first-class language constructs.

---

## 8. Verification strategy (how we know each workstream is done)

Every workstream ships with:

1. **A feature test** under `tests/<workstream>/` that exercises the new
   construct.
2. **A regression gate** that every existing example still compiles and
   produces byte-identical output (manifests and stdout) to 1.0, *where the
   semantics are unchanged*. Where 2.0 changes semantics (e.g. typed
   photonic API replacing `raw { }`), the test asserts byte-identical
   *manifests* (the hardware-level output) even though the source changed.
3. **A stage-1 parity check** that the self-hosted `flashc1` produces the
   same output as stage-0 `flashc` for the new construct.
4. **A lint test** (W12) where applicable.

The full suite (`tests/run_all.sh`) must stay green after every workstream.
No workstream merges with a red test.

---

## 9. Risks and mitigations

| Risk | Mitigation |
|------|------------|
| W1 (unify grammar) is a big mechanical change that could break everything | Branch it; keep stage-0 as reference; require byte-identical manifests for all existing examples before merge |
| `@` operator vs `@annotation` lexer ambiguity | Contextual disambiguation (after expression = operator, at statement start = annotation); explicit test in W5 |
| `tritvec[N]` lowering on the binary host is slow (loops over trits) | That's correct emulation behavior; the win is on photonic hardware. Document it. The binary host is a bootstrap, not the target. |
| Typed photonic API (W6) changes manifest emission | Assert byte-identical manifests to 1.0 `raw { }` path; the typed wrappers already call the same `_flash_raw_*` externs |
| Effect checking (W8) could reject existing examples | Make effects opt-in per-fn; only enforce when an annotation is present. Unannotated fns are unchecked (Zag's model). |
| Variadic `println` (W10) needs language support | Ship fixed-arity first in W2; W10 adds variadics. No user is blocked. |
| Migration cost for existing users (just you, for now) | `flashc migrate` command (W12.4) does mechanical substitutions; manual review for the semantic changes. |
| Self-host (stage-1) parity for every new construct doubles the work | Accept the cost; it's the price of self-hosting. Stage-1 parity tests catch drift early. |

---

## 10. Out of scope for 2.0 (explicit non-goals)

To keep this plan shippable, these are *not* in 2.0:

- **Full `comptime` / macros / reflection.** W11 ships `const` only. Full
  compile-time execution is a 3.0 question.
- **A native ternary backend (no Zag).** Flash 2.0 still lowers to Zag
  shims. The IR introduced in W1.4 *enables* a native backend later, but
  writing one is a separate project.
- **Async / concurrency / atomics.** Zag v1 explicitly excludes these;
  Flash 2.0 inherits that.
- **A package manager / module system beyond `@import`.** Same as Zag v1.
- **Borrow checking / ownership.** Same as Zag v1. Pointer validity is the
  programmer's responsibility.
- **Unicode string semantics.** Strings are `[]u8` bytes, same as Zag.
- **A GPU compute path beyond the existing `engine_dispatch_kernel`.** The
  photonic path *is* Flash's "GPU."

---

## 11. Summary — why this makes Flash *superior* for balanced-ternary photonic hardware

1. **The home radix is balanced ternary, and the syntax lets you write it.**
   `t++t+0-+0+` digit literals (W4) and `t<width>` types mean users write
   what the hardware sees, not decimal encodings.
2. **Interference is a first-class operator.** `tritvec[N] a @ b` (W5) is
   *one optical operation* on a vector of trits. This is the language-level
   expression of what photonic hardware does in one shot. No other language
   has this.
3. **The photonic API is typed with units.** `fire_shield(voxel(2,1,0), mw
   1.5)` (W6) makes the "radians vs milliwatts" class of bug a compile
   error. The hardware API becomes an algebra, not magic names.
4. **Effects make the photonic capability visible.** `@photonic` (W8) means
   a function's signature tells you it talks to the hardware. No surprise
   laser fires.
5. **One grammar, Flash-native stdlib.** No `_zag_*`, no two dialects, no
   hand-unrolled pipelines (W1, W2). Users think in Flash, not in "Zag with
   trits."
6. **The cost model is honest.** `trit_encoding_bits` per family (inherited
   from 1.0) means the type system *prices* radix embedding. Native types
   pay nothing; foreign types pay their own way.

Flash 2.0 is the language where balanced ternary is the home radix,
interference is the native compute, light is the native I/O, and the syntax
lets you write all three directly. That's the superiority claim, and this
plan is how it gets built.

---

## 12. Workstream checklist (print this and tick boxes)

- [ ] **W1** — Unify grammar: EBNF, `let f(a,b)=out` sugar, `program`
      optional, IR skeleton. Tests: `unify/round_trip`,
      `unify/photonic_massive_loop`, all examples re-pass.
- [ ] **W2** — Flash stdlib: `print`/`println`/`fmt`, codegen emits Flash
      names, ternary-aware formatting, rewrite examples. Tests:
      `stdlib/fmt`, `stdlib/no_zag_symbols`.
- [ ] **W3** — Small fixes: unary `-`, drop `group` for `{ }`, `defer`
      semantics, underscore literals documented. Tests:
      `syntax/unary_minus`, `syntax/scoped_block`, `syntax/defer`.
- [ ] **W4** — Trit digit literals: `t++t+0-+0+` syntax, width check,
      `fmt("{t}", x)`. Tests: `trit/literals`, `trit/literal_value`,
      `trit/literal_print`.
- [ ] **W5** — `tritvec[N]` + `@`: type, operators (`@ ~ & | ^`), `of`,
      photonic lowering (one path per `@`). Tests: `tritvec/ops`,
      `tritvec/photonic_massive`, `tritvec/width_mismatch`,
      `tritvec/index`, `tritvec/at_disambig`.
- [ ] **W6** — Typed photonic API: `voxel`/`beam`/`phase`/`power`/`axis`/
      `polarity`/`laser`, unit literals (`mw`/`rad`), `raw { }` demoted to
      `@unsafe`. Tests: `photonic/typed_calls`, `photonic/unit_mismatch`,
      `photonic/raw_demoted`, `photonic/manifest_parity`.
- [ ] **W7** — Typed `mat3`/`wave`/`vec3`: first-class types, usable in
      `fn`. Tests: `photonic/mat3_typed`, `photonic/wave_carrier`.
- [ ] **W8** — Effects: `@photonic` checked, inherit `@realtime`/`@io`/
      `@total`/`@noalloc`/`@unsafe`. Tests: `effects/photonic_ok`,
      `effects/photonic_violation`, `effects/realtime_violation`.
- [ ] **W9** — Inference + methods: `push(&xs, 9)`, `c.doubled()`. Tests:
      `types/inference`, `types/method`, `types/inference_ambiguous`.
- [ ] **W10** — Variadic `println`/`fmt`. Tests: `stdlib/variadic`.
- [ ] **W11** — `const` for type params (`tritvec[WIDTH]`). Tests:
      `const/basic`, `const/type_param`.
- [ ] **W12** — Spec (`FLASH_2_0_SPEC.md`), grammar (`FLASH_2_0_GRAMMAR.md`),
      linter (P0xxx/T0xxx), migration guide + `flashc migrate`, README
      rewrite.

When every box is ticked, Flash 2.0 is done.
