# Flash H — The Hardware-Native Plan (revised)

**Status:** Design proposal, revision 2. Incorporates the external
architecture review (2026-11-21); the review's verdict — "correctness ahead
of sophistication" — is the ordering principle of this document.
**Author:** Devin, direction set with Micah; architecture review by
independent critique.
**Scope:** Post-2.0/A+ plan. Six phases P0–P6, nineteen workstreams, each
with files touched, gating tests, and risks.

**Companions:** [`FLASH_2_0_PLAN.md`](FLASH_2_0_PLAN.md) (W1–W12, landed),
[`FLASH_2_0_SPEC.md`](FLASH_2_0_SPEC.md),
[`FLASH_A_PLUS_PLAN.md`](FLASH_A_PLUS_PLAN.md) (S1–S10),
[`OS_READINESS.md`](OS_READINESS.md).

---

## 0. The thesis (revised)

The original draft aimed at "the fastest and best language for native
photonic ternary hardware." The review was right: that goal cannot be a
design assumption — it can only be a *demonstrated result*. The strategic
target is:

> **Flash is a verifiable, hardware-aware balanced-ternary systems language
> with explicit control, a reproducible compiler, and a validated path from
> program semantics to photonic execution.**

The differentiation comes from three properties the project can actually
build and test: **correct ternary semantics**, **explicit photonic resource
control**, and **trustworthy optimization evidence**. Hardware performance
leadership is claimed only when measured on real devices.

What the revision changes, concretely:

- Semantics freeze comes **first** (P0), not last — because the emulator,
  the optimizer, and the spec all need it.
- One **vertical slice** (P1) proves the whole path end-to-end before
  breadth begins.
- The emulator is extended (P2) with **measurement discipline**: counted,
  modeled, and measured numbers are labeled and never conflated.
- Verification and agent interfaces (P3) land **before** optimization —
  diagnostics move early, safety boundaries are auditable.
- Optimization (P4) requires a **typed dependency-aware IR and proven
  rewrite rules**; the equivalence gate precedes the optimizer, not
  follows it.
- `flashd` (P5) is an **exploration engine with explicit promotion**, not
  an authority that rewrites release artifacts.
- Language polish and certification (P6) close the loop last.

The review also caught real defects in the first draft, now fixed here:

| Draft claim | Reality / correction |
|---|---|
| "12 PPU32 op classes" | `Ppu32Class` has **13** members. Fixed throughout. |
| `a @ ~a → 0`, `a @ 0 → a`, `~(a@b) → ~a@~b` as rewrites | **Invalid under the 2.0 spec's digit-wise-multiplication semantics**; valid under the *consensus* semantics the tests actually implement. The semantics themselves disagree across docs — freezing them is P0 work. |
| Ban host metrics (`ns`, `cycles`, `x86`) from output | Wrong tool. Replaced with a *provenance label* on every reported number (§5, H-E3). Host metrics stay, separately labeled. |
| `control { verify: off }` | Too broad. Replaced by **named, auditable per-check exemptions** (H-V2). |
| `tritvec[N]` removed; `t<width>` is the vector | Semantic decision, not a storage one. Deferred to a design gate (H-L1); default posture now: keep the distinction + zero-cost views until semantics are proven. |
| `flashd` publishes improved artifacts | Now an exploration engine producing versioned candidates; promotion is explicit and gated (H-D1). |
| e-graph finds "most mathematically efficient" program | Corrected: finds the lowest-cost *represented* expression under a declared cost model. Global hardware optimality is not claimed. |

---

## 1. Design principles (non-negotiable)

1. **Correctness ahead of sophistication.** No advanced component ships on
   semantics that aren't frozen and independently verified. Foundational
   gates are load-bearing — you do not proceed through a failed one.

2. **Measurement provenance.** Every performance number Flash reports is
   labeled as one of: **counted** (exact simulated quantity),
   **modeled** (estimated with declared parameters, version, and
   uncertainty), or **measured** (physical device data). Never report a
   modeled number as a measurement; never report a host number as a
   photonic number.

3. **Managed by default, control on request.** The default language stays
   simple; `control { }` scopes opt down per named knob; `raw { }` under
   `@unsafe` is the floor. Exemptions are narrow and auditable — never a
   blanket "stop checking."

4. **Safe code is checked; unsafe code is visible and limited; neither is
   mislabeled.** The type system and the verification report both say
   exactly which guarantees hold and which were waived.

5. **A target-specific package owns hardware specifics.** The core
   language describes portable computations and effects; `std.ppu32` owns
   registers, instruction encodings, and chamber topology. Full control
   without coupling every program to one processor.

6. **AI agents are first-class users.** Small regular grammar, stable
   diagnostic codes, machine-readable errors and reports, enumerable
   knobs — from the start, not retrofitted.

7. **Balanced ternary remains the home radix.** `t<width>` native, foreign
   radices pay their own embedding cost.

---

## 2. Current state (what H builds on)

| Asset | State | Gap H closes |
|-------|-------|--------------|
| `flasht/isa/ppu32.flash` | 13-class ISA enum + `Ppu32Insn` operand struct | No normative encoding spec (8-bit class + 24-bit operand layout undefined), no carry/fault semantics |
| `flasht/qemu_ppu32.zag` | Bring-up interpreter: parses + executes `LatticeRoute`, `PcmPhaseShift` | 11 classes unexecuted; no machine state; a parser, not a reference machine |
| `flash/runtime/chamber_sim.zag`, `raw_sim.zag` | 8×8×8 Active Lattice device model | Hardwired 8³; disconnected from PPU32 execution |
| `flash/ir/flash_ir.flash` (FIR) | Post-emit IR: routes, PCM, raw externs, `FirTritAlu` | Not an optimization IR: no def-use, no CFG, no effect ordering, op names carried as strings |
| Typed photonic API (W6/W7) | `fire_shield`, `inject_phase`, `set_laser`, `target_voxel`, `manual_polarity`, mat3/wave | ISA classes `PcmCouple`, `TritRegLoad`, `TritAlu`, `Nop`, mode enter/exit have no typed surface; no `std.ppu32` split |
| `@` interference operator | Implemented as **consensus** semantics per `tests/trit/interfere.flash`; spec'd as **digit-wise multiplication** in `FLASH_2_0_PLAN.md` W5 | **The documents disagree.** P0 freezes one semantics and aligns spec, impl, and tests. |
| A+ checks (S4–S8) | Landed as tests | Not all promoted to always-on errors; `--check` self-host still red |
| `control { }`, `flashd`, JSON diagnostics | Do not exist | New |

---

## 3. Phase map (revised execution order)

| Phase | Workstreams | Goal | Exit gate |
|-------|-------------|------|-----------|
| **P0 — Freeze semantics** | F1, F2 | Normative ISA, operator truth tables, encoding, bootstrap reliability | No unspecified opcode behavior; all reference semantic tests pass |
| **P1 — Vertical slice** | S1 | Source → FIR → encoded PPU32 → independent interpreter → verified state | Exact end-state + encoding round-trips, differential tests |
| **P2 — Emulation + measurement** | E1, E2, E3 | All 13 classes, chamber model, deterministic counters with provenance | Functional conformance; no unsupported physical claims |
| **P3 — Verification + agent interfaces** | V1, V2, V3, C1, C2, C3, C4 | JSON diagnostics, auditable safety boundaries, layered `flash verify`, control model | Compiler+verifier reject invalid programs correctly; control surface enumerable |
| **P4 — Proven optimization** | O0, O1, O2, O3 | Typed dep-aware IR, equivalence harness, proven rewrites, bounded search | Zero known semantic regressions; demonstrated cost improvement |
| **P5 — Daemon (conditional)** | D1, D2 | `flashd` exploration engine; separate e-graph track | Incremental benefit over P4 optimization, no build instability |
| **P6 — Polish + certify** | L1, L2, L3 | Scalar/vector resolution, docs, benchmarks, reproducible conformance | Documented behavior; externally reproducible results |

---

## 4. P0 — Freeze semantics and repair the foundation

*Nothing else is built until this phase is green. The review was blunt and
correct: the emulator, optimizer, and spec are all downstream of semantics
that are currently unfrozen and partially contradictory.*

### H-F1 — Normative machine + operator specification  *(P0)*

**Problem.** Three normative gaps, each a real defect:

1. **The `@` operator has two documented semantics.** `FLASH_2_0_PLAN.md`
   W5 defines `@` as digit-wise trit *multiplication* (under which
   `a @ ~a = -a²`, `a @ 0 = 0`). `tests/trit/interfere.flash` implements
   *consensus* (`+@+=+`, `-@-=-`, `+@-=0`, `0@x=x`) — a different operator
   entirely. Optimizer rules written against either one are silently wrong
   under the other.
2. **The PPU32 ISA is under-specified.** `Ppu32Insn` declares ~15 operand
   fields but the instruction format ("8-bit class + 24-bit operands") has
   no defined field encoding. `TritAlu Add` has no specified carry
   behavior even though `trit_math.zag` returns sum+carry. A voxel-read
   op has no encoding. Fault conditions are undefined.
3. **Bootstrap reliability is asserted, not gated.** `AGENTS.md` documents
   transient self-compilation segfaults; the plan treated fixpoint as a
   settled gate.

**Deliverables.**

- **`docs/PPU32_ISA_V1.md`** — normative ISA: complete per-class encoding
  (operand field layout within the 24-bit operand space, or an explicit
  variable-length/secondary-word format if fields don't fit — decide and
  write it down), register semantics, `TritAlu` carry/overflow rules,
  memory behavior, fault conditions, instruction ordering guarantees,
  and device-op semantics. Adds the voxel-read op with a real encoding
  (name: `RawReadVoxel`, class id assigned in the spec).
- **`docs/TRIT_OPS_V1.md`** — executable operator semantics: the canonical
  truth tables for every trit operation (`@`, `~`, `&`, `|`, `^`, add,
  mul, nand, neg), including the resolution of the `@` contradiction —
  **decision: consensus semantics** (matches the implemented tests and
  the physical superposition reading; multiplication semantics is
  renamed `trit_mul` where numeric multiplication is meant, keeping the
  two operations distinct and each unambiguous). The doc is executable:
  the tables live as data the test suite reads, not prose.
- **Bootstrap reliability gate** — `build.sh` self-compilation gets a
  retry-with-log wrapper plus a recorded flake rate; transient segfaults
  become measured, not folklore. `tests/bootstrap_gate.sh` asserts
  fixpoint across N=5 consecutive clean builds.
- **Target boundary freeze** — restate `OS_READINESS.md` contract as
  normative: `ppu32` lowers photonic surface routes only; full-language
  → PPU32 is rejected; `rayos-ternary` reserved. (Already enforced by
  `os_target_contract_gate.sh`; documented here as a frozen boundary.)

**Files touched.**
- `docs/PPU32_ISA_V1.md`, `docs/TRIT_OPS_V1.md` (new, normative).
- `docs/FLASH_2_0_SPEC.md`, `FLASH_2_0_GRAMMAR.md` — align `@` semantics
  to the frozen table.
- `build.sh`, `tests/run_all.sh`, `tests/bootstrap_gate.sh` (new).

**Gate.** Every opcode has an encoding/decoding round-trip test and a
reference semantic test; every trit op's truth table is verified
exhaustively (3² for binary ops) against the executable spec — *before*
the full emulator is considered complete. No unspecified opcode behavior
remains.

---

### H-F2 — Baseline test suite hardening  *(P0)*

**Problem.** Golden tests alone certify "output matched last time," not
"output is right." The review's operating rule — no feature accepted
solely because a golden test passes — becomes a suite-level property.

**Deliverables.**
- For every existing golden test, add a **negative pair** where one
  exists: malformed input → fault (not silent accept), boundary values,
  semantic properties (`x @ x == x` for consensus; `x + y == y + x`), and
  a regression entry for each bug fixed during H.
- Property-based smoke layer for trit ops: exhaustively enumerate all 3²
  pairs for binary ops and 3³ for three-input cases — small enough to be
  exhaustive, not sampled.

**Files touched.**
- `tests/` — new `tests/negative/`, `tests/property/` directories;
  `run_all.sh` picks them up.

**Gate.** Suite stays 100% green with the new negative/property layers;
fuzz lex smoke (exists) extended to the PPU32 parser.

---

## 5. P1 — One complete vertical slice

### H-S1 — Source → FIR → encoded PPU32 → independent interpreter → verified state  *(P1)*

**Problem.** "It compiles" and "the manifest looks right" are not the same
as "it executes correctly on the target model." No end-to-end proof of
the full path exists.

**Goal.** Take **one** representative photonic program —
`examples/ternary_logic.flash` is the candidate (trit ops + interference,
small manifest) — and prove the entire chain:

1. `flashc` compiles it to FIR + manifest (existing).
2. `flasht` encodes to `.ppu32` per `PPU32_ISA_V1.md` (F1's encoding is
   the authority; fix `emit_ppu32.flash` to match it).
3. An **independent reference interpreter** executes the encoded image.
   *Independent* means: written against `PPU32_ISA_V1.md`, not ported
   from `emit_ppu32`'s assumptions — separate author path so compiler
   and interpreter can't share the same bug. Implement it as
   `selfhost/ref_interp.flash`, a deliberately dumb, spec-literal
   executor (no shared code with the encoder).
4. Machine end-state is verified: trit registers, PCM grid, and emitted
   side-effects match the spec-computed expected state (hand-computed
   for this one program, checked into the test).
5. Malformed-input and fault paths exercised: truncated encoding,
   out-of-range register, `Raw*` outside mode-enter — each produces the
   spec'd fault, never a skip.

**Files touched.**
- `flasht/emit_ppu32.flash` — conform to `PPU32_ISA_V1.md` encoding.
- `selfhost/ref_interp.flash` (new) — independent reference interpreter.
- `tests/slice/` — end-to-end fixtures + hand-computed expected states.

**Gate.** Exact end-state match and encoding round-trips on the slice
program, plus all fault cases behaving per spec. This is the *template*
every later workstream's verification copies.

---

## 6. P2 — Finish emulation and measurement discipline

### H-E1 — Complete PPU32 machine model  *(P2)*

**Problem.** The bring-up interpreter executes 2 of 13 classes. Porting
`qemu_ppu32.zag` to Flash doesn't make it correct — it's a parser, not a
reference machine.

**Goal.** A complete machine model per `PPU32_ISA_V1.md`:

```flash
struct Ppu32State {
    trit_regs: [32]i8,          // trit register file
    trit_carry: i8,             // carry flag per ISA carry rules
    pcm_rows: i32,
    pcm_cols: i32,
    pcm_grid: *i8,              // PCM phase state, rows×cols
    chamber: ChamberState,      // E2 device model
    mode: i32,                  // raw-mode flag per ISA fault rules
    pc: i32,
    counters: PhotonCounters,   // E3
    faults: i32,                // sticky fault log (structured, V3)
}
```

- All 13 `Ppu32Class`es execute: `Nop` (real timing semantics), `PcmPhaseShift`,
  `PcmCouple`, `LatticeRoute`, `TritRegLoad`, `TritAlu` (Add/Mul/Nand/Neg
  with ISA-defined carry), all 7 `Raw*` ops, and the new `RawReadVoxel`.
- `Raw*` outside `RawModeEnter`/`Exit` is a **machine fault** — the
  emulator checks the contract; it does not silently accept.
- Written in Flash (`selfhost/`): the toolchain is self-hosted; its
  emulator should be too. `ref_interp` (S1) stays deliberately separate —
  this emulator is the full-featured one; ref_interp is the dumb spec
  oracle used by differential tests.

**Files touched.**
- `selfhost/qemu_ppu32.flash` (new) — full emulator.
- `flasht/isa/ppu32.flash` — per-class semantics table conforming to
  `PPU32_ISA_V1.md`.
- `selfhost/lib/` — reuse `hardware_manager`, `manifest`, `spatial_probe`.

**Tests gating E1.**
- `tests/emulator/all_classes.flash` — all 13 classes execute; golden
  end-state.
- `tests/emulator/faults.ppu32` — every ISA-defined fault triggers
  exactly.
- `tests/emulator/differential.sh` — full emulator vs. `ref_interp`
  agree on every test image (the independence property, exercised).
- All existing `.ppu32` examples execute to their declared end-state.

---

### H-E2 — Chamber device model  *(P2)*

**Problem.** `chamber_sim`/`raw_sim` model an 8×8×8 Active Lattice —
hardwired, in Zag, disconnected from PPU32 execution.

**Goal.** The chamber becomes the emulator's photonic device:
- Parametric lattice edge (default 8), coordinates checked against the
  *target descriptor* (C1), not a hardcoded constant.
- `fire_shield`, `inject_phase`, `set_laser_mw`, `target_voxel`,
  `manual_polarity`, `RawReadVoxel` act on explicit `ChamberState`.
- Interference executes as superposition in the lattice per the frozen
  `@` semantics — device truth table == language truth table ==
  `TRIT_OPS_V1.md`. One source of truth.
- Voxel readback lets tests assert chamber end-state directly.

**Honest bound (from the review):** the model is functional — ISA-, unit-,
and state-accurate. Loss, crosstalk, thermal are deferred (non-goals);
the model is honest about what it does not simulate, and E3's provenance
labels prevent modeled numbers from masquerading as physics.

**Files touched.**
- `selfhost/lib/chamber.flash` (new; port of `chamber_sim`+`raw_sim`,
  parametric edge).
- `selfhost/qemu_ppu32.flash` — `Raw*` dispatch.

**Tests gating E2.**
- `tests/emulator/chamber_state.flash` — `raw_chamber.flash` → `.ppu32`
  → executed; voxel end-state differential vs. the existing Zag sim.
- `tests/emulator/lattice_edge.flash` — edge 4/8/16; out-of-lattice
  coordinate is the spec'd fault.
- Interference differential: device `@` == `TRIT_OPS_V1.md` for all 3²
  input pairs.

---

### H-E3 — Counters with measurement provenance + `flasht stat`  *(P2)*

**Problem.** Nothing measures a Flash program in photonic terms — but the
fix is not "pretend the model is hardware." The fix is labeled numbers.

**Goal.** The emulator accumulates counters; `flasht stat <image>` reports
them with a provenance label on every line.

**Counter table (provenance in parentheses).**
| Counter | Unit | Provenance |
|---------|------|------------|
| `insn_count` | count | **counted** — exact |
| `trit_reg_writes` | count | **counted** — exact |
| `interfere_ops` | count | **counted** — exact |
| `pcm_ops` | count | **counted** — exact |
| `chamber_writes` | count | **counted** — exact |
| `route_um` | µm of route | **counted** — under declared geometry |
| `energy_uws` | µW·s | **modeled** — parameters: power/µs per op class, model version |
| `optical_latency` | path units | **modeled** — parameters: route+PCM depth model, model version |

- **Modeled numbers carry the model**: `energy_uws` reports alongside
  `cost_model_version` and the parameter table used (power per op class,
  per-route, per-PCM-shift). Changing parameters changes the reported
  number and the version — never silently.
- **Host metrics stay, labeled.** Compiler throughput, simulator
  wall-clock, build latency are legitimate engineering metrics — they
  appear in a separate `host:` section, clearly labeled as not-photonic.
  (Replaces the draft's banned-token lint, which was the wrong tool —
  a real device's latency is legitimately reported in ns.)
- **Measured numbers** arrive only when a real device exists; the report
  schema already has the field so the discipline is built before the
  hardware is.
- Per-program and per-region counters (`RawModeEnter` brackets; an
  `@region` marker is cheap to add).
- `flasht stat` emits text and `--json`; the JSON schema is stable and
  versioned.

**Files touched.**
- `selfhost/qemu_ppu32.flash` — `PhotonCounters` accumulation +
  provenance labels.
- `selfhost/opt/cost_model.flash` (new) — the declared parameter table
  (also feeds O3).
- `flasht`/driver CLI — `stat` subcommand + JSON.

**Tests gating E3.**
- `tests/emulator/counters.flash` — counted fields asserted exactly.
- `tests/emulator/provenance.sh` — every numeric line in `stat` output
  carries counted/modeled/measured/host label (the lint that replaces
  the token ban).
- `tests/emulator/counters_deterministic.sh` — same image → byte-
  identical report.
- `tests/emulator/model_version.sh` — changing the parameter table
  changes `cost_model_version` in the report.

---

## 7. P3 — Verification and agent interfaces

*Structured diagnostics move early per the review — an agent should be
able to consume compiler errors from the beginning of execution, not
after most features exist.*

### H-V1 — Structured diagnostics early  *(P3, first)*

**Problem.** Diagnostics are human strings; agent-authored code needs
machine-parseable errors. Also per review: **diagnostics must prioritize
repairs over unsafe switches** — an OOB error must not lead with
"disable bounds checking."

**Goal.**
- Every diagnostic: stable code (F0xxx compile, P0xxx photonic, T0xxx
  trit, L0xxx leak, A0xxx agent-bug, V0xxx verify), file/line/col,
  severity, message, **`suggestion`** field.
- Suggestion ordering rule: the suggestion names the *repair* first
  (e.g., "check index against `len`"), and mentions the knob that would
  silence the check only as a last, labeled alternative
  (`waive via control { checks: -bounds } — loses bounds guarantee`).
- `flashc --json` emits `{code, file, line, col, severity, message,
  suggestion, waivable}`; `waivable` names the exact exemption that
  would silence it — legibility for humans and agents both.
- `docs/DIAGNOSTICS.md` — the code catalog; a drift test keeps catalog
  and emitted codes in sync.

**Files touched.**
- `selfhost/flashc1.flash`, `fsema.flash` — diagnostic record + JSON
  emitter (reuse fmt/PrintBuf).
- `docs/DIAGNOSTICS.md` (new).

**Tests gating V1.**
- `tests/verify/json_diag.sh` — schema snapshot: every diagnostic has
  code/file/line/severity; `--json` parses.
- `tests/verify/code_stable.sh` — catalog ↔ emitted-code drift test.
- `tests/verify/suggestion_order.sh` — sample diagnostics assert repair
  precedes waiver in `suggestion`.

---

### H-V2 — Auditable safety boundaries  *(P3)*

**Problem.** The draft's `control { verify: off }` switches off the whole
verifier — too broad for a language whose pitch is strong verification.
Separately: the A+ ownership model claims UAF protection *and* unrestricted
aliasing; those don't automatically coexist (an alias can outlive the
freed owner). And `AGENTS.md` documents arena `realloc` falling back to
heap — arena tests must account for allocations that escape the arena.

**Goal.**
- **Named per-check exemptions, not an off-switch.** `control` gains a
  `checks:` knob taking a *list*: `control { checks: -bounds }` disables
  only bounds checking in that scope. `-verify` as a whole-group
  exemption does **not** exist. Every waiver is named in the source.
- **Waivers are auditable data.** `--check`/`verify` output (incl. JSON)
  lists every waived check and its scope: `waived: [bounds @
  file:line-col]`. The verification report says exactly which guarantees
  hold — per the principle: safe checked, unsafe visible and limited,
  neither mislabeled.
- **Unsafe boundaries preserve information.** `raw { }`/`@unsafe`
  regions still propagate effects and record which guarantee families
  were waived at the boundary.
- **Aliasing honesty.** The ownership checker tracks owner consumption
  (landed); the docs and diagnostics stop claiming full UAF where
  aliasing defeats it — `unsafe` pointer aliases are tracked as *waived
  coverage*, reported, not silently unprotected. Region-lifetime +
  escape analysis for scope-bound arenas is a dedicated sub-deliverable
  before any "arenas prevent UAF" claim:
  - `tests/safety/arena_escape.flash` — pointer escaping an arena scope
    is caught or reported as waived; never silently accepted.
  - `tests/safety/arena_realloc_heap.flash` — realloc-fallback-to-heap
    ownership path covered (per AGENTS.md), freed-by-heap vs
    freed-by-arena tracked separately.

**Files touched.**
- `selfhost/fsema.flash` — per-check exemption mechanism, waiver log,
  escape analysis for arena scopes.
- `selfhost/femit.flash` — honor per-check waivers.
- `docs/DIAGNOSTICS.md` — waiver codes.

**Gate.** `tests/verify/` proves: representative invalid programs are
rejected; waivers appear in the report; no waiver exists that isn't
named in source.

---

### H-V3 — `flash verify`: layered, honest gates  *(P3)*

**Problem.** The draft's `flash verify` implied "compiled + emulated =
verified," but PPU32 lowers only surface routes — general-purpose Flash
has no PPU32 lowering. A green exit must not imply verification that
never occurred.

**Goal.** `flash verify <prog>` runs the applicable gates and reports
each one's status — `passed`, `failed`, or **`unsupported`/`skipped`**
— never conflating "skipped" with "passed":

| Gate | Establishes |
|------|-------------|
| Static correctness | Types, ownership, effects, dataflow (V1/V2 checks, strict) |
| Host execution | Runtime tests for general-purpose Flash on the binary host |
| PPU32 execution | Target instructions execute per `PPU32_ISA_V1.md` (when a PPU32 lowering exists; else reported `unsupported`) |
| Photonic validation | Chamber end-state + op ordering satisfy declared constraints |
| Performance evaluation | Modeled costs (E3) + benchmark comparison vs. baseline |

- A full-language program with no PPU32 lowering gets static + host
  gates and reports `ppu32: unsupported` — honest, not green.
- Verification report is machine-readable (`--json`): per-gate status,
  waivers applied, provenance of every number.

**Files touched.**
- `selfhost/flashc1.flash` — `verify` subcommand + per-gate status.
- `tests/run_all.sh` — `@verified` examples run through it.

**Tests gating V3.**
- `tests/verify/gate_status.sh` — a program with no PPU32 lowering
  reports `unsupported`, not `passed`; exit code reflects only what ran.
- `tests/verify/assert_end_state.flash` — assert-driven verification on
  the PPU32 path.

---

### H-C1 — `std.ppu32`: target package, not language pollution  *(P3)*

**Problem.** Full ISA exposure is required ("every control shown"), but
baking `trit_reg_store(3, +1)` into the core language couples every Flash
program to one processor layout — fragmentation when a second photonic
target exists.

**Goal.** Two layers:
- **Core language** — portable computations and effects (typed photonic
  API from W6/W7 stays portable: `fire_shield`, `inject_phase`, voxels,
  power, phase, polarity).
- **`std.ppu32` package** — every register, instruction, and raw device
  capability of the PPU32 target: `trit_reg_load/store`, `trit_alu_*`,
  `pcm_couple`, `raw_mode_enter/exit`, `read_voxel`, `nop`.
- **`std.target` descriptor** — a machine-readable target description
  (instructions supported, chamber dims, register count, timing
  constraints, capabilities). The compiler reads it; `control` knobs and
  `verify` consult it; a second target = a second descriptor, not a
  fork of the language.

**Files touched.**
- `selfhost/std/ppu32.flash` (new) — the target package.
- `selfhost/std/target.flash` (new) — descriptor type + PPU32 instance.
- `selfhost/fsema.flash` — typed bindings checked against the descriptor.
- `selfhost/femit.flash` — lowering per descriptor, not hardcoded.

**Tests gating C1.**
- `tests/photonic/coverage.sh` — iterate `Ppu32Class`, assert each has a
  typed binding in `std.ppu32`. 100% ISA coverage is machine-checked.
- `tests/photonic/all_controls.flash` — golden manifest per new call.
- `tests/photonic/descriptor.flash` — chamber-dim mismatch against the
  descriptor is a compile error.
- `tests/photonic/mode_contract.flash` — managed mode auto-emits
  `RawModeEnter`/`Exit`; `control { modes: manual }` requires explicit
  bracketing (forgetting = compile error).

---

### H-C2 — `control { }` directives  *(P3)*

**Problem.** Total control, per piece, user-chosen — needs a mechanism.
(Revised per review: the knob set governs compiler *behavior*, and the
safety knob is per-check, never whole-verifier.)

**Goal.** Scoped directives + file-level defaults:

```flash
control { bounds: manual, alloc: arena, routes: explicit,
          modes: manual, effects: declared, checks: -bounds } {
    // full manual scope; every waiver named; report lists them
}
```

**Knob set (v1, closed and enumerable).**
| Knob | Values | Governs |
|------|--------|---------|
| `bounds` | `managed` / `manual` | index checks on `[]`/ArrayList |
| `alloc` | `managed` / `arena` / `pool` / `manual` | default allocator for the scope |
| `routes` | `auto` / `explicit` | photonic place-and-route vs. programmer coordinates |
| `modes` | `managed` / `manual` | raw-mode bracketing (C1) |
| `effects` | `infer` / `declared` | `@photonic`/`@realtime` propagation vs. declaration |
| `checks` | `+name` / `-name` list | **per-check** waivers — auditable, named (V2) |
| `layout` | `packed` / `natural` / `explicit` | t-word storage packing |
| `opt` | `none` / `fast` / `max` | per-scope hint to the optimizer |

- File-level `control { }` sets module defaults; scopes override
  additively; conflicts and unknown knobs are compile errors naming both
  sites.
- `flashc controls --list` prints every knob, values, defaults — the
  enumerable contract agents consume.
- Inside `raw { }`, knobs are meaningless — `raw` is already manual.

**Files touched.**
- `selfhost/fparse.flash`, `fsema.flash`, `femit.flash` — parse,
  validate, honor.
- `selfhost/flashc1.flash` — `controls --list`.
- `docs/FLASH_2_0_GRAMMAR.md`, spec — syntax.

**Tests gating C2.** `control/knobs`, `conflict`, `unknown_knob`,
`nested` (three-level scope override), `list` (`--list` == spec table),
`checks_per_check` (waiving `bounds` leaves `ownership` on).

---

### H-C3 — First-class memory, honest arena semantics  *(P3)*

**Problem.** `Allocator` vtable exists (`default_allocator`, `arena_allocator`);
per-site allocator naming exists (`make_with`). Missing: scope binding,
and honest accounting for arena `realloc` falling back to heap.

**Goal.**
- `control { alloc: arena }` scopes: implicit arena create + `defer
  arena_free` — emitted as readable code, nothing hidden (still no GC).
- `control { alloc: manual }`: every site explicit; ownership tracks
  UAF/double-free *within the aliasing limits* (V2's honest bound).
- Escape analysis on arena scopes (V2) — a pointer outliving its arena
  is a named finding, not a silent hole.
- Heap-fallback ownership (per AGENTS.md) tracked as a distinct path.

**Files touched.**
- `selfhost/std/mem.flash` (new) — `arena`/`pool`/`manual` descriptors
  bound to `control { alloc: ... }` names.
- `selfhost/fsema.flash`, `femit.flash` — binding + emitted `defer`.

**Tests gating C3.** `control/arena_scope` (net-zero accounting),
`control/manual_alloc` (leak caught), `safety/arena_escape`,
`safety/arena_realloc_heap`.

---

### H-C4 — `raw { }` as the floor  *(P3)*

**Goal (unchanged in substance, audited per V2).**
- `raw { }` reaches every `std.ppu32` binding and every `_flash_raw_*`
  extern — complete register-poke surface, documented in
  `docs/RAW_SURFACE.md`.
- `raw` regions preserve type/effect info at boundaries: effects still
  propagate to the enclosing fn; waived guarantee families are recorded
  in the verification report.

**Tests gating C4.** `raw/coverage.sh` (every extern callable),
`raw/effect_leak.flash` (effects still propagate).

---

## 8. P4 — Proven optimization

*The equivalence gate precedes optimization work, not follows it.*

### H-O0 — Typed optimization IR + validator  *(P4, prerequisite)*

**Problem.** Current FIR is a route/PCM representation — no def-use, no
CFG, no effect ordering, no resource deps, ALU ops carried as name
strings. You cannot safely optimize sequences that share op names.

**Goal.** `selfhost/opt/oir.flash` (new) — a typed optimization IR over
FIR with:
- Explicit def-use edges and register dependencies.
- Control-flow regions (straight-line blocks + the photonic pipeline).
- Effect ordering constraints (chamber ops are stateful unless proven
  pure; `Raw*` sequences preserve order).
- Full operand information per node (no name-string dispatch).
- Timing/scheduling constraints for photonic resources (PCM row/col
  occupancy, route conflicts).
- A **validator** (`oir_validate`) that checks IR well-formedness
  independently of the producer — run before *and after* every
  transform.

**Tests gating O0.** `tests/opt/oir_roundtrip.flash` (FIR → OIR → FIR
lossless on the corpus), `tests/opt/oir_validate.flash` (validator
rejects malformed dependency graphs).

---

### H-O1 — Proven rewrite rules on pure regions  *(P4)*

**Problem.** The draft's trit identities were wrong under the spec'd
semantics — the exact class of bug an optimizer must never contain.

**Goal.** A rewrite engine over OIR, pure-expression regions first:

1. **Rule provenance.** Every rule is declared in
   `selfhost/opt/rules_*.flash` with its **proof obligation** — for trit
   ops: exhaustive verification against `TRIT_OPS_V1.md` (3² inputs
   checked at build/test time; multi-trit identities checked
   property-wise plus differential against `ref_interp`). A rule whose
   obligation fails **cannot enter the rule set** — the check is part of
   the build, not a review nicety.
2. **Trit/interference algebra** (frozen semantics — examples under
   consensus `@`: `a @ ~a → 0`, `a @ 0 → a`; under whichever semantics
   TRIT_OPS_V1 freezes, only identities proven under it ship).
   `trit_neg(trit_neg(x)) → x`, constant folding on the truth tables.
3. **Phase algebra** — `shift(p)`/`shift(-p)` cancellation, µrad
   normalization, same-row/col fusion — only where ordering + observable
   state are preserved.
4. **Route algebra** — zero-length/zero-power elimination, identical-
   consecutive fusion, dead-voxel routes — only where chamber end-state
   and ordering are preserved.

**The equivalence harness runs here** (not at O4): every optimized
artifact executes vs. its unoptimized form in the emulator — identical
end-state (registers, chamber, side-effects; counters may differ).

**Files touched.**
- `selfhost/opt/fir_opt.flash` + `rules_*.flash` (new).
- `selfhost/opt/equiv.flash` (new) — the differential harness.
- `selfhost/flashc1.flash` — `-O` wiring.

**Tests gating O1.**
- `tests/opt/rule_proofs.sh` — every rule's obligation executes; a
  deliberately-broken rule in the test fixture is refused.
- `tests/opt/identities.flash`, `no_fire.flash`.
- `tests/opt/equivalence.sh` — whole-corpus differential gate.

---

### H-O2 — Bounded search + measured regression comparison  *(P4)*

**Goal.** `-O{0,1,2}`: `-O0` none, `-O1` proven local rewrites, `-O2`
bounded search (time-boxed iterative deepening, O3 cost model scoring).
Determinism: same input + budget + cost-model version → byte-identical
output. Benchmark suite (`tests/opt/bench/`) records per-example cost
before/after — improvement is *demonstrated*, not asserted.

**Tests gating O2.** `tests/opt/deterministic.sh`,
`tests/opt/bench_improves.sh` (≥1 benchmark strictly cheaper, none
worse, all equivalence-green).

---

### H-O3 — Declared photonic cost model  *(P4)*

**Goal.** `cost(oir_seq) -> PhotonCost` built on E3's model —
`energy_uws` primary, `optical_latency` secondary, counted fields as
tiebreakers. The model is declared data with a version (`cost_model.flash`),
shared by rewrite scoring, `flasht stat`, and `flashd`. Reports carry
the provenance labels from E3 — modeled costs are never reported as
measured.

**Tests gating O3.** `tests/opt/cost_known.flash` (hand-computed),
`cost_monotonic.flash` (valid rewrite never raises cost),
`cost_provenance.sh` (modeled label carried end-to-end).

---

## 9. P5 — The daemon (conditional on P4 evidence)

### H-D1 — `flashd`: exploration engine with explicit promotion  *(P5)*

**Problem (review).** A background optimizer that *publishes* artifacts
creates build-identity and reproducibility hazards: which artifact did
the release pipeline verify?

**Goal.** `flashd` explores; it does not publish.

- `flasht` emits `<prog>.fir`/`.ppu32` + `<prog>.opt.state` sidecar.
- `flashd watch <dir>` searches in the background, writing **candidates**
  — each an immutable record: content hash, source hash, compiler
  version, ISA version, cost-model version, equivalence result, cost
  comparison vs. baseline.
- **Promotion is explicit**: `flasht promote <candidate>` swaps the
  candidate into the build only if (a) equivalence passed and (b) the
  declared objective improved under the exact declared model version.
  Release pipelines pin promoted hashes; a daemon discovery can't
  silently change what a release verified.
- Versioned ledger (`<prog>.opt.ledger`) records every candidate and
  every promotion — auditable optimization history.
- `-O3` semantics: "compile fast at -O1, hand the artifact to `flashd`
  for background improvement, promote explicitly later."

**Files touched.**
- `selfhost/flashd.flash` (new) — watcher + candidate store + ledger.
- `selfhost/opt/search.flash` — bounded search over proven rules.
- `flasht` CLI — `promote`.

**Tests gating D1.**
- `tests/opt/daemon_candidates.sh` — candidates appear with full
  metadata; nothing promoted without `promote`.
- `tests/opt/promote_gate.sh` — a candidate failing equivalence or
  model-version mismatch is refused.
- `tests/opt/ledger.sh` — ledger records every candidate + promotion
  deterministically.

---

### H-D2 — E-graph research track (separate, gated)  *(P5)*

**Corrected claim (review):** equality saturation finds the lowest-cost
**represented** expression under a declared model — not "the globally
optimal hardware program." Route scheduling, contention, and latency are
non-additive; the claim is bounded accordingly.

**Goal.** `docs/EQUALITY_SATURATION.md` — design for an OIR e-graph
(e-classes over typed OIR nodes, proven rules from O1, O3 cost
extraction). Prototype in `selfhost/opt/egraph.flash` only after O0's IR
can express which transformations are legal.

**Gate.** On the O2 benchmark corpus: e-graph extraction achieves cost ≤
`-O2` on every case, strictly better on ≥1, equivalence-green —
otherwise the track stays experimental.

---

## 10. P6 — Polish and certify

### H-L1 — Scalar-vs-vector: a design gate, not a decision  *(P6)*

**Problem.** The draft removed `tritvec[N]`; the review is right that
physical representation ≠ semantics. A `t9` is a scalar integer with
carry arithmetic; a 9-trit vector is lane-wise ops. Same storage,
different abstraction.

**Goal.** Resolve with evidence, not preference:
- **Default posture:** keep the semantic distinction — `t<width>` scalar
  arithmetic *and* an explicit digit-vector view (`digits(t9)` →
  lane-wise ops without a second storage representation). Zero-cost
  view, not a second type.
- Write the unified-model semantics (arithmetic, indexing, overflow,
  operator meaning) for both options; prove the chosen model has
  unambiguous semantics; only then remove `tritvec[N]` from spec or
  confirm the two-abstraction design.
- Extend remaining digit-wise ops (`~ & | ^`), per-trit indexing,
  and `of` fill per the proven model.

**Tests gating L1.** `tests/trit/vector_ops.flash`, `index_oob.flash`,
plus the semantics-proof appendix in `TRIT_OPS_V1.md`.

---

### H-L2 — Docs by control level + benchmarks  *(P6)*

- `docs/LEVELS.md` — L0 managed → L1 assisted → L2 manual → L3 raw;
  every feature lands in exactly one level, taught in order.
- `examples/` grouped by level; `full_demo` the L0 showcase,
  `raw_chamber` the L3.
- `flashc explain <code>` — plain-language diagnostic-code lookup.
- `tests/bench/` — representative benchmark suite with versioned cost
  reports (E3 provenance) — the evidence base for "demonstrated, not
  asserted."

---

### H-L3 — Spec update + reproducible conformance  *(P6)*

- Spec gains: `control { }`, the resolved vector model, V1/V2 rules,
  diagnostic catalog, `std.ppu32` split, and the emulator as the
  **definition of execution** (spec/emulator disagreement = a bug in
  both, fixed together).
- Conformance suite: static checks + host + PPU32 + photonic gates per
  `flash verify`, with a documented procedure an external party can run
  to reproduce every claim — the certification deliverable.

---

## 11. Dependency graph

```
P0: F1 (normative specs + bootstrap gate) ──> F2 (baseline hardening)
        │
P1:     └──> S1 (vertical slice: encode + independent interpreter)
        │
P2:     ├──> E1 (full machine model) ──> E2 (chamber device) ──> E3 (counters + provenance)
        │
P3:     ├──> V1 (JSON diagnostics — FIRST in P3)
        ├──> V2 (auditable boundaries) ──> V3 (layered verify)
        ├──> C1 (std.ppu32 + descriptor) ──> C2 (control{}) ──> C3 (memory) , C4 (raw)
        │         └── V2 gates C2's checks knob; C1 gates C2's modes knob
P4:     ├──> O0 (typed IR + validator) ──> O1 (proven rules + equivalence
        │         harness) ──> O2 (bounded search) , O3 (cost model ⟵ E3)
P5:     └──> D1 (flashd: candidates + explicit promotion) , D2 (e-graph)
P6:     └──> L1 (vector design gate) ──> L2 (docs/bench) ──> L3 (spec + conformance)
```

---

## 12. Non-goals (explicit)

- **No garbage collector.** Managed memory = compiler-visible arenas,
  emitted as readable `defer`. Never hidden reclamation.
- **No on-target JIT.** `flashd` is a compile-time exploration engine.
- **No borrow checker.** Affine ownership + named waivers is the model;
  aliasing stays legal for hardware access.
- **No physics-accurate simulation in v1.** Functional/ISA/unit/state
  accuracy only; modeled numbers are labeled modeled.
- **No performance superiority claims from the emulator.** Hardware
  leadership is a demonstrated result on real devices, later.
- **No global-optimality claims from the e-graph.** Lowest-cost
  represented expression under a declared model — nothing more.
- **No new concurrency model.** Photonic parallelism lives in beams.

---

## 13. Execution contract (operating rules for whoever implements)

*From the review — these bind the plan's executor, human or agent.*

1. **No silent specification changes.** A contradiction is recorded and
   resolved in the normative doc first, then in compiler behavior.
2. **No self-certified correctness.** Where compiler and emulator could
   share a bug, the check is an independent reference (`ref_interp`,
   `TRIT_OPS_V1` executable tables).
3. **No feature accepted solely because a golden test passes.** Every
   feature ships negative tests, boundary cases, property/exhaustive
   checks where tractable, and regression entries.
4. **No optimization without equivalence evidence.** Pure regions
   first; externally observable effects always preserved; equivalence
   gates every publish/promotion.
5. **No claims without measurement provenance.** Every number states
   inputs, versions, assumptions, and evaluation method
   (counted/modeled/measured/host).
6. **No large cross-pillar commits.** Reviewable per-workstream changes;
   bootstrap + conformance gates green per commit.
7. **No proceeding through failed foundational gates.** Fix the
   assumption; don't build compatibility patches around it.
8. **Repairs before waivers in diagnostics.** A suggestion names the fix
   first; the exemption is the last, labeled alternative.

---

## 14. Risks (revised)

| Risk | Mitigation |
|------|------------|
| Frozen semantics still diverge across docs later | `TRIT_OPS_V1` is executable data read by tests — the table *is* the spec; drift fails CI |
| Emulator over-claims physics | Provenance labels (E3) + non-goals; modeled ≠ measured enforced in `stat` output lint |
| `ref_interp` and encoder drift apart | Differential test between them is a P2 gate — drift *is* the signal it exists to catch |
| Always-on checks break self-host | P0 bootstrap gate + V-phase ordering; self-host `--check` clean is a P3 gate |
| Daemon mutates what a release verified | Immutable candidates, explicit promotion, hash pinning (D1) |
| E-graph balloons | Separate track, fixed gate, O1/O2 ship regardless |
| Alias-aware UAF claims overstate | Waiver-visible reporting (V2); escape analysis before arena guarantees |

---

## 15. Workstream checklist (19 items)

**P0 — Freeze**
- [ ] **H-F1** — `PPU32_ISA_V1.md` (full encoding, carry, faults,
      `RawReadVoxel`), `TRIT_OPS_V1.md` (executable truth tables; `@`
      contradiction resolved to consensus, multiplication kept as
      `trit_mul`), bootstrap reliability gate, target boundary frozen.
- [ ] **H-F2** — Negative + boundary + property layers in
      `tests/negative/`, `tests/property/`; PPU32 parser fuzz.

**P1 — Vertical slice**
- [ ] **H-S1** — `ternary_logic` end-to-end: encode per ISA spec →
      `ref_interp` → hand-computed end-state verified; fault cases.

**P2 — Emulation + measurement**
- [ ] **H-E1** — All 13 classes execute; fault tests; differential vs.
      `ref_interp`.
- [ ] **H-E2** — Parametric chamber device; interference differential;
      voxel readback.
- [ ] **H-E3** — Provenance-labeled counters + `flasht stat` + JSON +
      `cost_model.flash` table.

**P3 — Verification + agent interfaces**
- [ ] **H-V1** — Diagnostic codes + `--json` + catalog + suggestion
      ordering (repair before waiver).
- [ ] **H-V2** — Per-check exemptions, waiver audit log, boundary
      effect preservation, arena escape/realloc-heap analysis.
- [ ] **H-V3** — `flash verify` layered gates with
      passed/failed/unsupported statuses.
- [ ] **H-C1** — `std.ppu32` + `std.target` descriptor; 100% ISA
      coverage gate.
- [ ] **H-C2** — `control { }` + `controls --list` + per-check `checks:`.
- [ ] **H-C3** — Scope-bound allocators, visible `defer`, escape honesty.
- [ ] **H-C4** — `raw` full coverage + `docs/RAW_SURFACE.md` + boundary
      effects.

**P4 — Proven optimization**
- [ ] **H-O0** — Typed OIR (def-use, CFG, effects, resources, timing) +
      independent validator.
- [ ] **H-O1** — Proven rewrite rules (proof obligations in build) +
      equivalence harness.
- [ ] **H-O2** — `-O0/1/2` bounded deterministic search + benchmark
      regression suite.
- [ ] **H-O3** — Declared cost model, versioned, provenance-carried.

**P5 — Daemon (conditional)**
- [ ] **H-D1** — `flashd` candidates + ledger + explicit `promote` gate.
- [ ] **H-D2** — `docs/EQUALITY_SATURATION.md` + gated prototype.

**P6 — Polish + certify**
- [ ] **H-L1** — Scalar/vector design gate resolved with proof.
- [ ] **H-L2** — `docs/LEVELS.md`, level-grouped examples,
      `flashc explain`, benchmark suite.
- [ ] **H-L3** — Spec update; externally reproducible conformance suite.

When every box is ticked, Flash is a verifiable, hardware-aware
balanced-ternary systems language with explicit control, a reproducible
compiler, and a validated path from program semantics to photonic
execution — demonstrated, not asserted.
