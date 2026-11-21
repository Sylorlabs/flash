# Flash H — The Hardware-Native Plan

**Status:** Design proposal. Not yet implemented.
**Author:** Devin, direction set with Micah (2026-11-20).
**Scope:** The post-2.0/A+ plan. Five pillars: emulation (E), control (C),
verification (V), optimization (O), language polish (L). Thirteen workstreams
(H-E1..H-L3). This document is the contract: every change is named, motivated,
scoped, and ordered.

**Companions:** [`FLASH_2_0_PLAN.md`](FLASH_2_0_PLAN.md) (W1–W12, landed),
[`FLASH_2_0_SPEC.md`](FLASH_2_0_SPEC.md),
[`FLASH_A_PLUS_PLAN.md`](FLASH_A_PLUS_PLAN.md) (S1–S10),
[`OS_READINESS.md`](OS_READINESS.md).

---

## 0. The thesis

Flash 2.0 made it a real language. A+ made it catch its own bugs. What it is
not yet is **the fastest and best core language for photonic computing
hardware in native ternary** — because today there is no photonic-ternary
machine it can measure itself against. Every number Flash produces is a
binary-host number. The language claims ternary; the measurements lie.

Flash H fixes the foundation, then builds what the hardware story actually
needs:

1. **A photonic-ternary emulator.** `qemu_ppu32` executes 2 of the 12
   `Ppu32Class` op classes today. H completes it into a real machine model —
   full ISA, chamber physics, counters in photonic units — so Flash programs
   can be *run and measured* on emulated ternary hardware. Numbers reported
   to users come from this model. Host x86 numbers never appear in a
   Flash-facing metric.

2. **Total control, progressively unlocked.** Writing raw Flash should feel
   like writing raw C — same controllability — but the default surface stays
   simple. `raw { }` exists; it is the floor. On top of it, `control { }`
   scopes let the programmer decide, per piece, what the compiler manages
   and what they manage themselves. Every hardware control is reachable;
   nothing is hidden.

3. **Verification is part of the language, not a flag.** The A+ catches —
   null safety, optional unwrap, exhaustive switch, definite assignment,
   affine ownership, effects — become always-on. Broken code does not ship.
   Diagnostics are structured and machine-readable: the primary authors of
   Flash code will include AI agents, and an error an agent can't parse is
   an error nobody fixes.

4. **A background super-optimizer, not a garbage collector.** No GC, ever.
   Instead `flashd`: a daemon that takes compiled FIR, rewrites it toward
   mathematical optimum against the photonic cost model, and converges in
   the background — fast compiles stay fast; the optimizer spends *time*,
   not *compile time*. The research question — how to do this *properly* —
   has a known candidate answer: equality saturation. H builds the
   differential-equivalence harness that makes adopting it safe.

5. **The language finishes unifying.** `t<width>` *is* the trit-vector
   type. On balanced-ternary hardware a `t9` is physically a vector of nine
   trits; a separate `tritvec[N]` type gives developers two names for one
   machine object. We unify on `t`-width words.

---

## 1. Design principles (non-negotiable)

1. **Photonic-ternary is the only measurement standard.** Every performance
   number Flash reports — energy, latency, op counts — is expressed in
   photonic-ternary units from the emulator model. No x86 cycles, no host
   nanoseconds, no "emulation tax" numbers mixed in.

2. **Managed by default, control on request.** The default language is the
   simple one: bounds checked, effects inferred, memory pooled, routes
   auto-scheduled. A `control { }` scope opts down per knob. `raw { }` under
   `@unsafe` is the floor beneath every knob — full hardware access, always.

3. **Every control is shown.** The PPU32 ISA and the wave/chamber API get
   complete typed coverage. A test enumerates the ISA and asserts a typed
   surface exists for each class — coverage is verified, not claimed.

4. **Broken code does not ship.** Compile-time verification is always on
   and strict by default. The escape hatch is scoped (`control { verify:
   off }` / `@unsafe`), never global silence.

5. **The optimizer spends time, not compile time.** `flashd` runs in the
   background against build artifacts. `flasht -O` levels cover the
   compile-time fast path. The daemon is where the slow, deep search lives.

6. **AI agents are first-class users.** Grammar stays small and regular;
   every diagnostic has a stable code and machine-readable form; every
   knob is enumerable (`flashc controls --list`). Legibility is a feature.

7. **Balanced ternary remains the home radix.** Inherited from 1.0/2.0.
   `t<width>` words are the vector type; negation is free; foreign radices
   pay their own embedding cost.

---

## 2. Current state (what H builds on)

| Asset | State | Gap H closes |
|-------|-------|--------------|
| `flasht/isa/ppu32.flash` | 12-class ISA defined (Nop, PcmPhaseShift, PcmCouple, LatticeRoute, TritRegLoad, TritAlu, Raw×7) | Only a spec — no execution |
| `flasht/qemu_ppu32.zag` | Interpreter runs LatticeRoute + PcmPhaseShift | 10 of 12 classes unexecuted |
| `flash/runtime/chamber_sim.zag`, `raw_sim.zag` | 8×8×8 Active Lattice device model | Hardwired 8³; not wired to PPU32 execution |
| `flash/ir/flash_ir.flash` (FIR) | Post-emit pre-machine IR; `.fir` text format | No optimizer, no cost model |
| Typed photonic API (W6/W7) | `fire_shield`, `inject_phase`, `set_laser`, `target_voxel`, `manual_polarity`, mat3/wave | ISA classes `PcmCouple`, `TritRegLoad`, `TritAlu`, mode enter/exit have no typed surface |
| A+ checks (S4–S8) | Landed as tests: null, unwrap, exhaustiveness, ownership | Not all promoted to always-on errors |
| `control { }` | Does not exist | The entire progressive-control model is new |
| `flashd` | Does not exist | The entire optimizer daemon is new |

---

## 3. Phase map

| Phase | Workstreams | Goal |
|-------|-------------|------|
| **P1 — Emulation foundation** | E1, E2, E3 | Full PPU32 machine model + photonic counters |
| **P2 — Control + verification** | C1, C2, C3, C4, V1, V2, V3 | Total control model; always-on verifiers; agent-grade diagnostics |
| **P3 — Super-optimizer** | O1, O2, O3, O4 | FIR rewrite engine, photonic cost model, `flashd` daemon, equivalence gate |
| **P4 — Language polish** | L1, L2, L3 | t-width vector story, control-level docs, spec + conformance |

P1 goes first — nothing else can claim to be photonic-measured without it.
C/V can run in parallel with P1 (they are mostly sema + surface work).
O needs E3's counters. L documents everything and goes last.

---

## 4. Pillar E — The photonic-ternary emulator

### H-E1 — Complete PPU32 emulator  *(P1, foundational)*

**Problem.** `qemu_ppu32` executes `LatticeRoute` and `PcmPhaseShift` only.
The other ten `Ppu32Class`es — `Nop`, `PcmCouple`, `TritRegLoad`, `TritAlu`,
`RawFireShield`, `RawInjectPhase`, `RawSetLaserMw`, `RawTargetVoxel`,
`RawSetPolarity`, `RawModeEnter`, `RawModeExit` — are defined in
`flasht/isa/ppu32.flash` but have no execution semantics. A `.ppu32` image
that uses them cannot run.

**Goal.** A complete machine model: every `Ppu32Insn` executes against an
explicit machine state.

**Machine state.**
```flash
struct Ppu32State {
    trit_regs: [32]i8,          // trit register file, -1/0/+1
    pcm_rows: i32,
    pcm_cols: i32,
    pcm_grid: *i8,              // PCM phase state, rows×cols
    chamber: ChamberState,      // E2 — beam/laser/voxel device model
    mode: i32,                  // raw mode active flag
    pc: i32,
    counters: PhotonCounters,   // E3
}
```

- `TritRegLoad` moves trit literals/values into `trit_regs`.
- `TritAlu` executes `Add`/`Mul`/`Nand`/`Neg` per the balanced-ternary truth
  tables already canonical in `flash/stdlib/trit_math.zag`.
- `PcmPhaseShift`/`PcmCouple` mutate `pcm_grid` (row/col phase state and
  coupling).
- `Raw*` ops drive the chamber device model (E2).
- `Nop` is a real instruction (timing/counter semantics), not a skip.
- `RawModeEnter`/`Exit` bracket chamber access; a `Raw*` op outside raw
  mode is a machine fault — the emulator *checks the contract*, it does not
  silently accept.

**Implementation note.** The emulator is written in Flash, not Zag —
`flasht/qemu_ppu32.zag` ports to `selfhost/qemu_ppu32.flash` (name TBD:
`fvemu`?). The toolchain is self-hosted; its emulator should be too.

**Files touched.**
- `selfhost/qemu_ppu32.flash` (new) — the emulator, ported from
  `flasht/qemu_ppu32.zag`.
- `flasht/isa/ppu32.flash` — semantics table per class (already has the
  encoding; add the contract each op enforces).
- `selfhost/lib/` — reuse `hardware_manager.flash`, `manifest.flash`,
  `spatial_probe.flash` for chamber/route state.

**Tests gating E1.**
- `tests/emulator/all_classes.flash` — a `.ppu32` exercising all 12
  classes; executes end-to-end, golden register/machine end-state.
- `tests/emulator/raw_mode_fault.ppu32` — `Raw*` outside mode-enter is a
  fault, not silent success.
- All existing `.ppu32` examples (`photonic_massive.ppu32`,
  `ternary_logic.ppu32`) execute and produce their manifests' declared end
  state.
- **Gate rule:** the test suite reports *emulator* end-state, not just
  process exit code.

**Risk.** The `.ppu32` text format is loose (key=value lines). The emulator
parser must fail closed on malformed input — a malformed instruction is a
fault, never a skip. Add `tests/emulator/malformed.ppu32` cases.

---

### H-E2 — Chamber physics device model  *(P1)*

**Problem.** `chamber_sim.zag`/`raw_sim.zag` model an 8×8×8 Active Lattice,
but hardwired, in Zag, and disconnected from PPU32 execution — `Raw*` ops
in the emulator need a device to act on.

**Goal.** The chamber model becomes the emulator's photonic device: beams,
laser power, polarity, voxel cells, phase injection, shield firing —
parametric lattice edge (default 8, not hardwired), expressed in Flash.

**Semantics to model (functional fidelity, not physics-accurate — see
decisions):**
- `fire_shield(voxel, mw)` — mark cell shielded, charge power counter.
- `inject_phase(voxel, rad)` — write phase state at coordinate.
- `set_laser_mw(beam, mw)` — set beam power.
- `target_voxel(voxel)` / `manual_polarity(voxel, pol)` — target + write
  polarity.
- Interference: `a @ b` on the device executes as superposition in the
  lattice — the physical thing the operator models.
- Voxel readback — a `RawReadVoxel` path (or probe op) lets tests assert
  chamber end-state.

**Files touched.**
- `selfhost/lib/chamber.flash` (new, port of `chamber_sim.zag` +
  `raw_sim.zag`) — `ChamberState`, cell array, parametric edge.
- `selfhost/qemu_ppu32.flash` — `Raw*` dispatch into chamber.
- `selfhost/lib/hardware_manager.flash` — coordinate/route bookkeeping is
  already ported; wire it through.

**Tests gating E2.**
- `tests/emulator/chamber_state.flash` — `raw_chamber.flash` compiled to
  `.ppu32`, executed; voxel end-state matches `raw_sim`'s output
  (differential against the existing Zag sim).
- `tests/emulator/lattice_edge.flash` — edge 4/8/16 lattices all run;
  out-of-lattice coordinate is a machine fault.
- Interference differential: `@` on the device matches the trit truth
  tables for all 3³ input pairs.

**Decision:** fidelity level is *functional* — ISA-accurate, unit-accurate,
state-accurate. Loss, crosstalk, and thermal are explicitly deferred (see
non-goals); the model is where cost numbers live, and it is honest about
what it does and doesn't simulate.

---

### H-E3 — Photonic counters + `flasht stat`  *(P1, the measurement rule)*

**Problem.** Today nothing measures a Flash program in photonic terms. The
`trit_encoding_bits` cost model prices *types*; nothing prices *execution*.

**Goal.** The emulator accumulates counters in photonic units, and
`flasht stat <image>` reports them. This is the only sanctioned source of
Flash performance numbers.

**Counters.**
| Counter | Unit | Charged by |
|---------|------|------------|
| `energy_uws` | µW·s | every powered op (route power × duration model) |
| `route_um` | µm | `LatticeRoute` path length |
| `pcm_ops` | count | `PcmPhaseShift`, `PcmCouple` |
| `interfere_ops` | count | `TritAlu` + device `@` ops |
| `trit_reg_writes` | count | `TritRegLoad`, `TritAlu` dst |
| `optical_latency` | path units | route + PCM depth model |
| `chamber_writes` | count | `Raw*` ops |

- Counters are per-program and per-region (bracketed by `RawModeEnter` or
  by `@region` markers — cheap to add, large profiling win).
- `flasht stat` emits text and `--json`; the JSON schema is stable and
  versioned (agents consume this).
- **The measurement rule:** a test greps every counter/report name in the
  toolchain output — `ns`, `cycles`, `x86`, `mips` are banned tokens. The
  binary host is a bootstrap; its numbers are not photonic numbers.

**Files touched.**
- `selfhost/qemu_ppu32.flash` — `PhotonCounters` accumulation per class.
- `flasht/emit_ppu32.flash` — region markers if needed.
- `flasht`/driver CLI — `stat` subcommand.

**Tests gating E3.**
- `tests/emulator/counters.flash` — a program with known op counts;
  counter values asserted exactly.
- `tests/emulator/no_host_metrics.sh` — the banned-token lint over
  `flasht stat` output and test logs.
- Counter determinism: same image → same counters, byte-identical report.

---

## 5. Pillar C — Total control, progressively unlocked

### H-C1 — Hardware control inventory: every control shown  *(P2)*

**Problem.** The typed photonic API (W6/W7) covers `fire_shield`,
`inject_phase`, `set_laser`, `target_voxel`, `manual_polarity`, and
mat3/wave. It does **not** cover `PcmCouple`, `TritRegLoad`, `TritAlu`,
`RawModeEnter`/`Exit`, or voxel readback. Today those exist only as
manifest internals — invisible to the programmer, exactly the "hidden
control" H rejects.

**Goal.** Every ISA class and every chamber primitive has a typed
first-class surface:

```flash
pcm_couple(row, col, phase)                 // PcmCouple
let r: trit = trit_reg_load(3)              // TritRegLoad read
trit_reg_store(3, +1)                       // TritRegLoad write
let q: trit = trit_alu_add(a, b)            // TritAlu (typed wrappers
let q2: trit = trit_alu_nand(a, b)          //   per Ppu32TritAluOp)
raw_mode_enter() / raw_mode_exit()          // explicit chamber bracketing
let pol: polarity = read_voxel(v)           // voxel readback
```

- Typed signatures, unit literals enforced (`mw`/`rad`), `@photonic`
  effect required — same discipline as W6.
- **Coverage gate:** a test iterates `Ppu32Class` and asserts each class
  has a declared typed binding in `std/photonic`. "Every control is shown"
  is a machine-checked property.

**Files touched.**
- `selfhost/std/photonic.flash` (new; W6's surface, consolidated) —
  full inventory.
- `selfhost/fsema.flash` — effect + unit checking on new calls.
- `selfhost/femit.flash` — lowering to manifest/PPU32 for each.

**Tests gating C1.**
- `tests/photonic/coverage.sh` — ISA-coverage assertion above.
- `tests/photonic/all_controls.flash` — golden manifest for every new call.
- `tests/photonic/mode_contract.flash` — `Raw*` without explicit
  mode-enter emits `RawModeEnter`/`Exit` automatically in managed mode;
  in `control { modes: manual }` (C2) the programmer writes them and
  forgetting one is a compile error.

---

### H-C2 — `control { }` directives: the progressive-control model  *(P2, the centerpiece)*

**Problem.** The user wants "every control shown" *and* a language that
feels simple by default — control chosen per piece, not per language. No
current mechanism exists.

**Goal.** A scoped directive that sets compiler-behavior knobs for a
region, plus a file-level default form:

```flash
control { bounds: manual, alloc: arena, routes: explicit,
          modes: manual, effects: declared } {
    // this scope: full manual. The programmer owns what the
    // compiler would otherwise do — and the verifier still checks.
}

control { verify: off } {
    // scoped escape hatch for code the checks can't see through.
    // The knob, not the bug, is what gets hidden.
}
```

**Knob set (v1).** Each knob: `managed` (default) or `manual`/`off` as noted.

| Knob | Values | What it governs |
|------|--------|-----------------|
| `bounds` | `managed` / `manual` | index checks on `[]`/ArrayList — `manual` = C-style unchecked |
| `alloc` | `managed` / `arena` / `pool` / `manual` | default allocator for the scope; `manual` = you malloc/free |
| `routes` | `auto` / `explicit` | photonic route scheduling — auto place-and-route vs. programmer coordinates |
| `modes` | `managed` / `manual` | raw-mode bracketing auto-emission (see C1) |
| `effects` | `infer` / `declared` | whether `@photonic`/`@realtime` propagate automatically or must be declared |
| `verify` | `on` / `off` | all V1 checks for the scope — the scoped escape hatch |
| `layout` | `packed` / `natural` / `explicit` | t-word storage packing (trits packed vs. byte-per-trit) |
| `opt` | `none` / `fast` / `max` | per-scope optimization hint to the daemon |

- **Semantics.** File-level `control { ... }` at top sets module defaults;
  nested scopes override additively. Conflicting knobs on the same scope
  are a compile error with both sites named. Unknown knobs are a compile
  error — the knob list is closed and enumerable.
- **Enumeration.** `flashc controls --list` prints every knob, its values,
  and its default — the machine-readable contract. This is the
  "AI-agent ergonomics" surface: an agent can ask the compiler what it is
  allowed to control.
- **Composition.** `control { }` composes with `raw { }`: `control`
  changes *what the compiler does*; `raw` changes *what the code does*.
  Inside `raw`, `control` knobs are meaningless — `raw` is already manual.

**Files touched.**
- `selfhost/fparse.flash` — `control` block + file-level form.
- `selfhost/fsema.flash` — knob validation, scope propagation, conflict
  errors.
- `selfhost/femit.flash` — honor `bounds`, `routes`, `modes`, `layout`.
- `selfhost/flashc1.flash` — `controls --list` CLI.
- `docs/FLASH_2_0_GRAMMAR.md`, `docs/FLASH_2_0_SPEC.md` — grammar + spec.

**Tests gating C2.**
- `tests/control/knobs.flash` — each knob exercised, behavior verified
  (e.g., `bounds: manual` compiles the same OOB write that `managed`
  rejects at compile time or traps at runtime).
- `tests/control/conflict.flash` — same knob twice in one scope = error.
- `tests/control/unknown_knob.flash` — typo'd knob name = error.
- `tests/control/nested.flash` — outer managed, inner manual, innermost
  managed — three-way scoping verified.
- `tests/control/list.sh` — `controls --list` output is complete and
  matches the spec table.

**Risk.** A closed knob list ossifies. Mitigation: adding a knob is a
spec-level change, deliberate — that is a feature, not a bug. The
inventory is the contract.

---

### H-C3 — First-class memory: managed pools, manual everywhere  *(P2)*

**Problem.** `Allocator` is already a vtable (`alloc_fn`/`realloc_fn`/
`free_fn`/`state`) with `default_allocator()` and `arena_allocator()`. But
allocators can't be *named per scope*, and ownership tracking (A+ S8) is
not tied to which allocator owns a value.

**Goal.**
- `control { alloc: arena } { ... }` scopes create and free an arena
  implicitly — managed memory with manual performance characteristics,
  no GC.
- `control { alloc: manual }` makes every allocation site explicit;
  affine ownership still tracks use-after-free/double-free.
- Allocation sites may name their allocator explicitly regardless of
  scope: `make_with[T](cap, &my_alloc)` (already exists — promote to the
  documented pattern).

**Files touched.**
- `selfhost/std/mem.flash` (new) — `arena`, `pool`, `manual` allocator
  descriptors bound to `control { alloc: ... }` names.
- `selfhost/fsema.flash` — scope → allocator binding; ownership tracking
  already landed (S8), extend to per-allocator domains.
- `selfhost/femit.flash` — implicit arena create/free at scope edges
  (emitted as the same `defer arena_free` a user would write — nothing
  hidden).

**Tests gating C3.**
- `tests/control/arena_scope.flash` — arena scope frees everything at
  exit; counter test (alloc bytes before/after == 0 net).
- `tests/control/manual_alloc.flash` — manual scope: leak detected by V1,
  correct free passes.
- `tests/safety/cross_allocator_uaf.flash` — arena pointer used after
  scope exit is caught (scope-exit free is visible to the ownership
  checker).

**Rule.** No garbage collector, no hidden reclamation, ever. `control
{ alloc: arena }` emits the free as an explicit `defer` — readable in
`--emit-zag` output. Managed means *the compiler writes what you would*,
never *the compiler does what you can't see*.

---

### H-C4 — `raw { }` as the floor  *(P2)*

**Problem.** `raw { }` is already demoted to `@unsafe` escape hatch (W6),
but its extern coverage is implicit — whatever happened to be wired. For
total control, `raw` must guarantee reachability of *everything*.

**Goal.**
- `raw { }` may name every `_flash_raw_*` extern and every PPU32 class
  binding — a documented, complete register-poke surface.
- `raw` blocks are exempt from `control` knobs and V1 checks *except*
  effects propagation (a `raw` block's effects still mark the enclosing
  fn — you can drop to raw, you cannot hide that you did).
- Documentation: `docs/RAW_SURFACE.md` — the complete `raw` API table,
  the "C mode" contract.

**Files touched.**
- `selfhost/fsema.flash` — `raw`-block symbol scope: externs legal,
  typed-API magic names illegal (they're managed-mode constructs).
- `docs/RAW_SURFACE.md` (new).

**Tests gating C4.**
- `tests/raw/coverage.sh` — every `_flash_raw_*` extern callable inside
  `raw { }`.
- `tests/raw/effect_leak.flash` — `raw` photonic op still propagates
  `@photonic` to the enclosing fn.

---

## 6. Pillar V — Verification is part of the language

"Making sure broken code doesn't ship. Strong verifiers built into the
language, strong catchers. Unlike C, you can actually see what's
breaking — every little issue found."

### H-V1 — Always-on checks: A+ becomes the default  *(P2)*

**Problem.** The A+ checks landed as tests, but promotion to always-on
errors is incomplete: `--check`/`--strict` are opt-in, and normal
compilation still lets through what strict mode rejects.

**Goal.** The zero-false-positive checks become compile errors in normal
compilation:

| Check | Class | Source |
|-------|-------|--------|
| F0003 photonic effect violation | error | W8 landed |
| F0006 non-exhaustive switch | error | S6 |
| F0007 uninitialized use | error | S7 |
| F0008 use of moved value | error | S8 |
| Null outside `?T` | error | S4 |
| `?T` deref without unwrap | error | S5 |
| L0012 leak (owned value dropped) | error in `--check`, warning in build | S2 |
| A0101 off-by-one, A0102 ignored return | error in `--check`, warning in build | S3 |

- `--check` = "verify this program fully" — the shipping gate.
- `control { verify: off }` is the only escape, scoped, visible in the
  source. `--unsafe` file flag does **not** exist (per-file silence is
  exactly the C failure mode).

**Files touched.**
- `selfhost/fsema.flash` — check promotion table.
- `selfhost/flashc1.flash` — `--check` semantics (implies strict).
- `tests/safety/*` — expect-reject cases become *default-mode* rejects.

**Tests gating V1.**
- `tests/verify/default_rejects.flash` — every error class above rejected
  in normal compile.
- `tests/verify/scoped_escape.flash` — `control { verify: off }` accepts
  the same code, scoped.
- Self-host gate: `flashc1 --check` passes on every `.flash` in
  `selfhost/` (A+ S1/C1 — the compiler eats its own food).

**Risk.** Promoting warnings to errors can break the self-host compile
(96 known F0001s in `--check`). Ordering: fix self-host `--check` clean
first (A+ S1 work, prerequisite task listed in C2's dependency edge).

---

### H-V2 — `flash verify`: the one-command ship gate  *(P2)*

**Problem.** A program that "compiles" hasn't been proven. Shipping needs
one command that means *verified*.

**Goal.** `flash verify <prog>` runs, in order:
1. `flashc1 --check` — all V1 checks.
2. Compile to PPU32 and execute in the emulator (E1/E2) — end-state
   assertions (`assert` calls become emulator-state checks).
3. Manifest verification — declared outputs match produced chamber
   state.
4. Counter report (E3) — the photonic cost of the verified program.

Exit 0 means: checks clean, emulator ran to completion, declared state
matches produced state. This is "broken code doesn't ship" as a tool.

**Files touched.**
- `selfhost/flashc1.flash` — `verify` subcommand orchestrating the above.
- `tests/run_all.sh` — `verify` becomes the gate for examples marked
  `@verified`.

**Tests gating V2.** `tests/verify/gate.flash` — a program that compiles
but faults in the emulator is rejected by `verify`. `tests/verify/
assert_end_state.flash` — assert-driven verification.

---

### H-V3 — Structured diagnostics: agent-grade errors  *(P2)*

**Problem.** Diagnostics are human strings. AI agents (the primary
Flash-code authors going forward) need to parse errors mechanically:
code, location, severity, and what to do.

**Goal.**
- Every diagnostic has a stable code (F0xxx compile, P0xxx photonic,
  T0xxx trit, L0xxx leak, A0xxx agent-bug, V0xxx verify).
- `flashc --json` emits `{code, file, line, col, severity, message,
  suggestion}` per diagnostic. Stable schema, versioned.
- Every error message names the fix or the relevant knob — e.g. a bounds
  error says `control { bounds: manual }` disables this check, not just
  "index out of bounds".

**Files touched.**
- `selfhost/flashc1.flash`, `selfhost/fsema.flash` — diagnostic record
  struct + JSON emitter (reusing the fmt/PrintBuf infrastructure).
- `docs/DIAGNOSTICS.md` (new) — the code catalog.

**Tests gating V3.**
- `tests/verify/json_diag.sh` — `--json` output parses, every diagnostic
  has code/file/line/severity; schema snapshot test.
- `tests/verify/code_stable.sh` — the DIAGNOSTICS.md catalog and the
  emitted codes stay in sync (a drift test).

---

## 7. Pillar O — The super-optimizer ("daemon")

"Not a GC — a daemon running in the background, constantly optimizing the
code to make sure it's the most mathematically efficient as possible.
Research how to do that properly."

### H-O1 — FIR rewrite engine  *(P3)*

**Problem.** FIR (`flash/ir/flash_ir.flash`) is the post-emit,
pre-machine IR — the right altitude for optimization — but nothing
rewrites it.

**Goal.** A rewrite engine over FIR sequences with three rule families:

1. **Trit/interference algebra** (on `TritAlu` chains and `@`):
   `a @ ~a → 0`, `a @ 0 → a`, `~(a @ b) → ~a @ ~b` where the law holds,
   `trit_neg(trit_neg(x)) → x`, constant folding over balanced-ternary
   truth tables.
2. **Phase algebra** (on `PcmShift` nodes): `shift(p)` then `shift(-p)`
   cancels; µrad normalization; consecutive shifts on the same
   row/col fuse.
3. **Route algebra** (on `LatticeRoute` nodes): zero-length and
   zero-power routes eliminated; identical consecutive routes fused;
   routes feeding provably-dead voxels removed.

**Design.** Rules are *local and confluent* in v1 — ordered rewrite
passes over the FIR list, each preserving a declared invariant. This is
the cheap 80%: it needs no search, runs in compile time, and its output
is deterministic.

**Files touched.**
- `selfhost/opt/fir_opt.flash` (new) — the rewrite passes.
- `selfhost/opt/rules_*.flash` — one file per rule family (keeps rules
  reviewable; the rule list is the auditable surface).
- `selfhost/flashc1.flash` — `-O` flag wiring.

**Tests gating O1.**
- `tests/opt/identities.flash` — each rule fires on a golden case.
- `tests/opt/no_fire.flash` — rules do *not* fire where the precondition
  fails (the negative cases matter more).
- `tests/opt/photonic_massive_opt.ppu32` — optimized manifest has fewer
  insns than baseline, same emulator end-state.

---

### H-O2 — Photonic cost model  *(P3, depends on E3)*

**Problem.** A rewrite is only "better" against a cost function. The cost
function must be photonic — energy and latency in the emulator's units,
never host cycles.

**Goal.** `cost(fir_seq) -> PhotonCost` — a pure function over a FIR
sequence using E3's counter model: `energy_uws` primary, `optical_latency`
secondary, `pcm_ops`/`route_um`/`interfere_ops` as tiebreakers and as
per-dimension reporting.

- The same model drives: rewrite scoring, `flasht stat` reporting, and
  `flashd` convergence — one source of truth.
- The model is *declared*: a table mapping each `FirOpKind`/`Ppu32Class`
  to its unit costs, in `selfhost/opt/cost_model.flash`. When real
  hardware arrives, the table is updated from measurements — the
  structure doesn't change.

**Tests gating O2.**
- `tests/opt/cost_known.flash` — a program with hand-computed photonic
  cost; model output asserted exactly.
- `tests/opt/cost_monotonic.flash` — applying a valid rewrite never
  increases cost (the engine's correctness invariant).

---

### H-O3 — `flashd`: the background optimizer  *(P3, the daemon)*

**Problem.** Deep optimization is search, and search is slow. Putting it
in the compile path makes compiles slow. The answer isn't to optimize
less — it's to move the slow part out of the critical path.

**Goal.** `flashd` — a daemon that optimizes *artifacts*, not compiles:

1. `flasht` emits `<prog>.fir` + `<prog>.ppu32` (plus a sidecar
   `<prog>.opt.state` for daemon bookkeeping).
2. `flashd watch <dir>` — a long-running process that watches for
   `.fir` artifacts, applies rewrite passes + guided search in the
   background, re-emits improved `.ppu32` when cost strictly decreases.
3. Convergence ledger: the daemon records `(generation, cost, rules
   applied)` per artifact; `flashd status` reports per-program cost
   trajectories in photonic units.
4. Determinism: given the same artifact + generation limit + cost model
   version, the daemon produces byte-identical output. Optimization is
   reproducible — non-negotiable for a systems language.
5. `flasht -O{0,1,2,3}` — compile-time levels map to: `-O0` none, `-O1`
   O1's local passes, `-O2` O1 + bounded search budget (time-boxed),
   `-O3` "emit then hand to flashd" (compile stays fast; the daemon
   finishes the job in the background). Default is `-O1`.

- Search strategy v1: iterative deepening over rule application with
  cost-model scoring — deterministic, bounded, incrementally improvable.
- The daemon runs as a hosted process on the dev machine (Linux today);
  on ternary hardware it becomes a resident optimizer — same code.

**Files touched.**
- `selfhost/flashd.flash` (new) — daemon entry point: watcher loop,
  artifact store, ledger.
- `selfhost/opt/search.flash` (new) — bounded search over O1 rules +
  O2 scoring.
- `selfhost/std/process.flash`, `io.flash` — file watching via
  existing runtime (poll-based is fine for v1).

**Tests gating O3.**
- `tests/opt/daemon_converges.sh` — drop a `.fir` in a watched dir; the
  daemon emits a `.ppu32` with cost ≤ baseline within N seconds.
- `tests/opt/daemon_deterministic.sh` — same input twice → byte-identical
  output + identical ledger.
- `tests/opt/no_regress.sh` — daemon never emits worse cost than the
  input artifact (it only publishes strict improvements).

**Risk.** A background daemon mutating build artifacts could surprise.
Mitigation: `flashd` never rewrites in place — it writes
`<prog>.opt.ppu32` alongside and only *publishes* (renames) on strict
improvement, with the ledger recording every publish.

---

### H-O4 — Equivalence gate + the research track  *(P3)*

**Problem.** "Optimize" is meaningless if the optimized program computes
something else. And "the most mathematically efficient" needs a technique
stronger than local rewrites eventually.

**Two deliverables.**

1. **The equivalence gate.** For every optimized artifact: run original
   and optimized in the emulator; chamber end-state, trit registers, and
   emitted side-effects must be identical (counters may differ — that's
   the point). `tests/opt/equivalence.sh` runs this over the whole
   examples corpus. A rule that breaks equivalence is pulled, not
   patched quietly.

2. **The research track: equality saturation.** The known-correct
   technique for "most mathematically efficient" rewriting is
   e-graphs/egg-style equality saturation: represent *all* equivalent
   forms of the program simultaneously, saturate with rewrite rules,
   then extract the minimum-cost form. It fits `flashd` perfectly:
   saturation is incremental, backgroundable, and cost-exact once it
   converges.
   - Deliverable: `docs/EQUALITY_SATURATION.md` — design for a FIR
     e-graph in Flash (e-classes over `FirNode`, rules from O1, O2 cost
     extraction), plus a prototype in `selfhost/opt/egraph.flash`.
   - Gate: on the O1 benchmark corpus, e-graph extraction finds cost ≤
     O1 on every case, strictly better on at least one, with the
     equivalence gate green.

**Why this is "doing it properly":** local rewrites get stuck in local
optima; equality saturation explores the equivalence space and extracts
the global minimum under the cost model — that is what "most
mathematically efficient" actually means, and the emulator gives it a
cost model that is honest.

**Tests gating O4.**
- `tests/opt/equivalence.sh` — whole-corpus differential gate (per above).
- `tests/opt/egraph_beats_local.sh` — the research gate above.

---

## 8. Pillar L — Language completion and polish

### H-L1 — `t<width>` is the trit-vector type  *(P4, decision made)*

**Decision.** No separate `tritvec[N]` type. On balanced-ternary hardware
a `t9` is physically a vector of 9 trits — the spec's `tritvec[N]` would
be a second name for the same machine object. Two spellings for one
thing is exactly the kind of choice that makes languages worse, not
better.

**What ships instead.**
- `@`, `~`, `&`, `|`, `^` operate digit-wise on `t<width>` (already true
  for `@`; extend the set per W5's table).
- Per-trit indexing: `a[i]` reads trit `i` of a `t<width>`; `a[i] = t`
  writes it (bounds per `control { bounds: ... }`).
- Fill sugar: `t9 of +1` → all-`+1` word (the `of` construct survives,
  applied to the t-type).
- `tritvec[N]` removed from the spec; W5's semantics map onto `t<width>`
  with `N` = width. `vec3` is `t3` (or stays a struct if it needs named
  accessors — resolve in spec update).

**Files touched.**
- `docs/FLASH_2_0_SPEC.md`, `FLASH_2_0_GRAMMAR.md` — replace `tritvec[N]`
  with the unified model.
- `selfhost/fparse.flash`, `fsema.flash`, `femit.flash` — indexing +
  `of` + remaining digit-wise ops.
- `selfhost/std/fmt.flash` — `fmt("{t}", v)` prints digit strings.

**Tests gating L1.**
- `tests/trit/vector_ops.flash` — `@ ~ & | ^` + indexing + `of` on
  `t9`/`t27`, golden outputs (interference test already covers `@`).
- `tests/trit/index_oob.flash` — `a[i]` OOB behavior per bounds knob.

---

### H-L2 — Managed-mode ergonomics + control-level docs  *(P4)*

**Problem.** "Python-ish" default means the *docs and examples* must be
organized by control level, or users will meet `raw { }` before
`println`.

**Goal.**
- `docs/LEVELS.md` — the four-level tour: **L0 managed** (no knobs, no
  effects declarations, everything inferred), **L1 assisted** (typed
  photonic API, declared effects), **L2 manual** (`control { }` knobs),
  **L3 raw** (`raw { }`, register pokes). Every feature lands in
  exactly one level; the doc teaches them in order.
- Rewrite `examples/` README-table grouping by level; `full_demo` becomes
  the L0 showcase, `raw_chamber` the L3 showcase.
- `flashc explain <code>` — prints the plain-language meaning of any
  diagnostic code (agents and humans share this).

**Tests gating L2.** Docs tests: every example compiles at its claimed
level (a script asserts `full_demo` uses no `control`/`raw`, etc.).

---

### H-L3 — Spec, grammar, conformance on the emulator  *(P4)*

**Goal.**
- `docs/FLASH_2_0_SPEC.md` → add `control { }`, the vector-type decision,
  the verification rules (V1), the diagnostic catalog reference, and the
  emulator as the *definition of execution*: where spec and emulator
  disagree, the emulator is a bug, the spec is truth — and both get
  fixed.
- `docs/FLASH_2_0_GRAMMAR.md` — `control` block syntax, `of` fill,
  indexing.
- The conformance requirement list gains: "execute under the PPU32
  emulator" and "pass `flash verify`".

---

## 9. Dependency graph

```
E1 ──> E2 ──> E3 ──> O2 ──> O3 ──> O4
 │      │             ^
 │      └── (device model feeds emulator Raw* ops)
 │
C1 ──> C2 ──> C3     (control model)
C4 ──┘
│
V1 ──> V2 ──> V3     (verification)
 ▲
 └── prerequisite: selfhost --check clean (A+ S1 residue)

O1 independent of E (rewrites need only FIR);
O3 needs O1+O2; O4 needs everything.
L1–L3 last (document the shipped reality).
```

---

## 10. Non-goals (explicit)

- **No garbage collector.** Managed memory = compiler-visible arenas and
  pools with explicit `defer` emission. Never hidden reclamation.
- **No on-target JIT.** `flashd` is a compile-time daemon. If on-device
  optimization becomes a goal, it is a separate design.
- **No borrow checker.** Affine ownership (landed, S8) is the memory-
  safety model; aliasing stays legal because hardware register access
  needs it.
- **No physics-accurate simulation in v1.** The emulator is
  functional/ISA/unit accurate. Loss, crosstalk, thermal are a later
  fidelity bump (and would land inside E2's device model, not a redesign).
- **No native ternary backend yet.** Flash still lowers through Zag for
  the binary host and emits PPU32 for the photonic target. A native
  backend is a separate project; the emulator is the interim target of
  truth.
- **No new concurrency model.** Photonic parallelism lives in the
  hardware (beams), matching the A+ posture.

---

## 11. Risks

| Risk | Mitigation |
|------|-----------|
| Emulator semantics drift from "real" hardware whenever it exists | The cost/semantic tables are *declared data* (`cost_model.flash`, `ppu32.flash`), swappable against hardware measurements |
| `control` knob list ossifies | Closed list is intentional; additions are spec changes. `controls --list` keeps the contract honest |
| Always-on checks break self-host compile | Order V1 behind the self-check cleanup (A+ S1); `--check` clean on `selfhost/` is V1's gate |
| Daemon nondeterminism poisons builds | Deterministic search + byte-identical ledger + publish-only-on-improvement; equivalence gate on every publish |
| E-graph work balloons | It's a research track with a fixed gate (O4); local rules (O1) ship regardless |
| "Total control" vs. "simple default" collides in UX | `docs/LEVELS.md` orders the teaching; defaults stay managed; every managed default is named and overrideable |

---

## 12. Workstream checklist

- [ ] **H-E1** — Full PPU32 emulator (all 12 classes, trit regs, PCM grid,
      mode contract). Tests: `emulator/all_classes`, `raw_mode_fault`,
      existing `.ppu32` corpus end-state.
- [ ] **H-E2** — Chamber device model (parametric lattice, all `Raw*`,
      interference superposition, voxel readback). Tests:
      `emulator/chamber_state`, `lattice_edge`, interference differential.
- [ ] **H-E3** — Photonic counters + `flasht stat` (+ JSON). Tests:
      `emulator/counters`, `no_host_metrics`, determinism.
- [ ] **H-C1** — Typed surface for 100% of `Ppu32Class`. Test:
      `photonic/coverage.sh`, `all_controls`, `mode_contract`.
- [ ] **H-C2** — `control { }` directives + `controls --list`. Tests:
      `control/knobs`, `conflict`, `unknown_knob`, `nested`, `list`.
- [ ] **H-C3** — Scope-bound allocators; managed = visible `defer`.
      Tests: `control/arena_scope`, `manual_alloc`,
      `safety/cross_allocator_uaf`.
- [ ] **H-C4** — `raw { }` complete coverage + `docs/RAW_SURFACE.md`.
      Tests: `raw/coverage`, `raw/effect_leak`.
- [ ] **H-V1** — Always-on error checks; `control { verify: off }` scoped
      escape; self-host `--check` clean. Tests: `verify/default_rejects`,
      `scoped_escape`, self-host gate.
- [ ] **H-V2** — `flash verify` ship gate. Tests: `verify/gate`,
      `assert_end_state`.
- [ ] **H-V3** — Stable diagnostic codes + `--json` + `docs/DIAGNOSTICS.md`.
      Tests: `verify/json_diag`, `code_stable`.
- [ ] **H-O1** — FIR rewrite engine (trit/phase/route rule families).
      Tests: `opt/identities`, `no_fire`, `photonic_massive_opt`.
- [ ] **H-O2** — Declared photonic cost model. Tests: `opt/cost_known`,
      `cost_monotonic`.
- [ ] **H-O3** — `flashd` background daemon + `-O` levels + ledger.
      Tests: `opt/daemon_converges`, `daemon_deterministic`,
      `no_regress`.
- [ ] **H-O4** — Equivalence gate + `docs/EQUALITY_SATURATION.md` +
      e-graph prototype. Tests: `opt/equivalence`,
      `egraph_beats_local`.
- [ ] **H-L1** — `t<width>` as the vector type (indexing, `of`, full
      digit-wise op set; `tritvec[N]` removed from spec). Tests:
      `trit/vector_ops`, `index_oob`.
- [ ] **H-L2** — `docs/LEVELS.md`, examples grouped by control level,
      `flashc explain`. Tests: docs-level assertions.
- [ ] **H-L3** — Spec/grammar update; conformance requires emulator
      execution + `flash verify`.

When every box is ticked, Flash is the language where balanced ternary
is measured, every control is shown, the broken never ships, and a
daemon keeps making the code more mathematically perfect in the
background.
