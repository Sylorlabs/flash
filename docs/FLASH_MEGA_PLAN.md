# Flash Mega Plan — Beat C. Be a great language.

**Status:** Design proposal, v1. Supersedes nothing — extends
[`FLASH_H_PLAN.md`](FLASH_H_PLAN.md) (the engineering contract, rev 2) with
the competitive and language-excellence layers it deliberately didn't cover.
**Direction:** Micah, 2026-11-21 — "push Flash to be the fastest; compare it
and don't stop until it beats C and all these other languages; ignoring the
emulation tax, native photonic ternary vs native binary, ours should win
every time."

**Companions:** `FLASH_H_PLAN.md` (P0–P6 engineering), `FLASH_2_0_PLAN.md`,
`FLASH_A_PLUS_PLAN.md`, `OS_READINESS.md`.

---

## 0. The claim, stated honestly

> **Flash on native photonic-ternary hardware beats C on native binary
> hardware everywhere the physics can win — and proves it with a public,
> reproducible scorecard.** Where physics can't win (serial, pointer-chasing
> code), the target is parity, stated as parity. A claim that can't be
> measured isn't a claim — it's marketing, and the H plan's provenance rules
> (counted/modeled/measured) apply to every number in this document.

Two honesty rules inherited from H:

1. **Modeled numbers are labeled modeled.** Until a physical photonic chip
   exists, the scorecard's photonic side comes from the H-E emulator and the
   H-O3 declared cost model. The C side is *measured* on real machines. The
   comparison is published with the asymmetry labeled, not hidden.
2. **"Wins every time" is a gate, not a wish.** Every workload category in
   the scorecard has a defined victory condition. A category that doesn't
   pass is an open item in the plan, not a footnote.

---

## 1. Why photonic-ternary *should* win — the physics, mapped to language features

Each hardware advantage is only a language win if Flash exposes it. This
table is the design map for the whole plan:

| # | Physics advantage | What it means | The Flash feature that monetizes it |
|---|---|---|---|
| 1 | **Interference is compute** | A MAC happens for free at a waveguide junction — no ALU, no gate switching | `@` operator: one interference op per trit-vector, not a loop. Already in language; W-series widen it |
| 2 | **Massive spatial parallelism** | Thousands of optical paths act at once — not SIMD lanes you buy in silicon, lanes the medium gives you | `beam`/`lattice` parallel constructs; the manifest IS a parallel schedule. Needs: parallel `for` lowering to routes |
| 3 | **Wavelength multiplexing** | Independent channels in the same waveguide — a second parallelism axis with zero extra space | Channel/color-aware `wave` types; multiplexed dispatch in the scheduler |
| 4 | **Matrix ops are propagation** | A matmul through an interferometer mesh is O(1) mesh depth, energy-per-op nearly flat in size | `mat3`/larger `matN` optical ops; `matvec` as a primitive, not a library loop |
| 5 | **Diffraction = Fourier transform** | A lens computes an FFT in the time light crosses it | `fft`/transform primitives mapped to optical transforms — photonic FFT is literally free-space physics |
| 6 | **Energy per op doesn't scale with electrons moved** | Photons in waveguides dissipate almost nothing vs. electrons in copper | The win is µW·s — E3 counters quantify it; "energy" becomes a first-class optimization target |
| 7 | **Balanced ternary itself** | ~1.585 bits per trit (58% denser than binary per digit position); sign lives in the digits — negation is free, no two's complement, no signed/unsigned split, symmetric range | `t<width>` home radix: denser state, cheaper sign ops, no overflow-into-sign bugs |
| 8 | **No global clock** | Optical pipelines are throughput-bound by modulator/detector rates, not a synchronous clock | The ISA is a *route schedule*, not an instruction stream — Flash's compilation model matches it natively; C's doesn't |

C can't monetize any of these — it's an abstraction stack built for
electrons and clocked silicon. Flash's advantage is *structural*: its
compilation output is a physical routing schedule, which is what the
hardware executes.

## 2. Where the fight is real — the workload scorecard

Not everything is a win, and pretending otherwise destroys credibility.
Scorecard by workload class (to be instantiated as real benchmarks in P-B):

| Workload | C on binary hardware | Flash on photonic ternary | Expected | Why |
|---|---|---|---|---|
| Dense matmul / conv / tensor kernels | GFLOP-bound, memory wall | O(1) mesh depth + wavelength parallelism | **WIN** | Physics, not codegen |
| Fourier/spectral transforms | O(n log n) butterfly ops | Optical FFT via propagation | **WIN** | Physics literally computes it |
| Wide logic evaluation (many lanes of boolean/trit ops) | bit-packing SIMD, per-gate delay | Interference lanes in one pass | **WIN** | @ op + lane count |
| Search / associative lookup / correlation | index + scan | Optical correlation/associative match | **WIN** | Pattern match is interference |
| AI inference kernels | MAC arrays, huge power | Photonic MACs (what photonic startups sell) | **WIN** | Same physics industry is betting on |
| Sorting / comparison networks | O(n log n) swaps | Optical sorting/routing networks | **WIN/PUSH** | Route networks exist; gate it |
| Parsing / string / pointer-chasing / JSON | tight scalar loops, cache-bound | no photonic advantage; ternary ALU is a draw | **PARITY** | Honest draw — target: never *slower* |
| Crypto-style bit mixing | serial dependency chains | serial too | **PARITY** | Bar: parity, stated as parity |
| OS/driver-style control code | native | native (same story) | **PARITY** | |
| Irregular graph algorithms | cache+branch bound | waveguide routing helps some patterns | **PUSH** | Research item |

The design consequence of the parity rows: Flash's GP side (the part that
looks like C) has to be *at least as good* as C on hosted execution —
codegen quality, no gratuitous overhead, lean runtime. "We win where the
hardware wins and never lose where it doesn't" is the actual victory
condition.

## 3. What makes a language *great* — the axes C still owns today

Hardware wins the benchmarks it wins. A great language also wins on axes
that have nothing to do with the chip:

| Axis | C's position | Flash's play |
|---|---|---|
| Compile speed | gcc/clang on large code: seconds to minutes | Self-hosted, small, single-pass — target: **Flash compiles faster than clang -O0 on equivalent-size code**, measured as a *host* metric, labeled host |
| Diagnostics | cryptic, unstructured | Stable codes + JSON + repair-first suggestions (H-V1) — structurally better than any 1970s compiler |
| Safety | none — UB everywhere | Compile-time verifiers at zero runtime cost: defined semantics where C has UB. Flash wins "time-to-correct-program" even on hosted targets |
| Memory model | raw malloc | No GC, but allocators + affine ownership + visible `defer` — C's control with Rust's catches |
| Portability | hardware everywhere | Bootstrap on any binary host via Zag; the target descriptor model (H-C1) means new photonic targets are *data*, not forks |
| Tooling | gdb/valgrind/sanitizer patchwork | Built-in: emulator, verifier, profiler (photonic counters), formatter, REPL, debugger that can *pause light* — step a route schedule and inspect the chamber |
| Package/build | makefiles, header hell | `flash.mod` + named deps (exists) → full package story |
| AI-agent authorship | noisy, macro-heavy | Small regular grammar, enumerable knobs, machine-readable everything — Flash is designed to be *written by agents correctly* |

Now the mega plan's workstreams. H (P0–P6) stays the engineering core;
this adds five pillars around it:

```
            ┌─────────────────────────────────────────────┐
            │              FLASH H (P0–P6)                 │
            │  semantics → slice → emulation → verify →    │
            │  control → proven optimization → daemon      │
            └─────────────────────────────────────────────┘
     ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐
     │ B — Bench  │  │ X — Perf   │  │ T — Tooling│  │ S — Stdlib │  │ G — Great  │
     │ the fight  │  │ engineering│  │            │  │ + interop  │  │ DX/ecosystem│
     └────────────┘  └────────────┘  └────────────┘  └────────────┘  └────────────┘
```

---

## 4. Pillar B — The benchmark war (prove the win)

### M-B1 — The Flash Grand Prix suite  *(foundation)*

**Goal.** A benchmark harness that is itself honest: `tests/bench/` holds
kernel programs in *matched pairs* — a Flash kernel and a C kernel doing
the same computation, checked for semantic equivalence on inputs.

**Kernel set (v1), mapped to the scorecard.**
- `matmul_64`, `conv2d_3x3`, `matvec_256` — tensor lane (WIN expected)
- `fft_1024`, `fir_filter` — spectral lane (WIN)
- `logic_eval_wide` (10k trit-lane boolean net) — logic lane (WIN)
- `assoc_search` (dictionary match) — (WIN)
- `sort_10k` — (WIN/PUSH)
- `json_parse`, `string_interp`, `ptr_chase` — serial lane (PARITY target)
- `sha_mix` — serial crypto-ish (PARITY)
- `nn_infer_mlp` — photonic MAC showcase (WIN showcase)

**Rules.** Flash side reports E3 counters + modeled cost (labeled);
C side compiled `gcc -O2`/`clang -O2` and *measured* (host section).
Equivalence: both sides produce identical outputs on the same inputs
(differential-checked) — no winning by computing less.

**Files.** `tests/bench/kernels/`, `tests/bench/run_gp.sh`,
`tests/bench/SCORECARD.md` (generated).

**Gate.** Suite runs end-to-end on emulator + host; scorecard emits with
provenance labels on every row.

### M-B2 — The scorecard: measured vs modeled, printed

**Goal.** A generated `docs/SCORECARD.md` (updated by the harness) showing
per-kernel: Flash counted ops, Flash modeled energy/latency (+model
version), C measured time/energy-estimate (declared method), and the
verdict column: **WIN / PARITY / PUSH / OPEN**.

- The honesty rule is executable: a kernel marked WIN only when its
  declared victory condition is met under the declared model; anything
  else is OPEN, listed in the plan until it flips.
- Reproducibility: the scorecard regenerates from one command
  (`tests/bench/run_gp.sh`) — an external party can rerun it.

**Gate.** Scorecard exists, regenerates deterministically, every row
labeled. (Content goal: WIN everywhere physics allows — that's the
campaign, executed per-kernel below.)

### M-B3 — Per-kernel victory campaigns

**Goal.** Each scorecard row is a campaign with its own work: write the
kernel, emit it, run it, profile it (E3 counters), find what the modeled
cost says is the bottleneck, fix the language or the scheduler — not the
benchmark — until the row flips to WIN.

- This is the "don't stop until it beats C" loop, mechanized: a kernel
  that stays OPEN gets its own investigation — is it a codegen gap, a
  scheduler gap, a missing primitive (`matvec`? muxing?), or an honest
  parity workload?
- New insight path: profiling data flows back into H-O1 rules and the
  scheduler (X1) — the benchmark suite *drives* the optimizer's rule set.

### M-B4 — The honesty appendix

`docs/HOW_WE_MEASURED.md` — the methods doc: emulator model parameters,
versions, C measurement rig (machine, governor, flags, warmup), the full
provenance of every number on the scorecard. This is what makes the win
*trustworthy* — the differentiator the review demanded.

---

## 5. Pillar X — Performance engineering (make the hardware win real)

### M-X1 — Photonic place-and-route scheduler

**Problem.** Today routes are emitted in source order. Real photonic
execution needs a scheduler: which operations run simultaneously on
different lanes, which share PCM rows, which serialize on chamber state.
This is the photonic equivalent of instruction scheduling — and it's
where most of the win lives.

**Goal.** `selfhost/sched/` — a scheduler pass between OIR and PPU32
emission:
- Lane allocation: independent trit/interference ops packed onto parallel
  optical lanes (width from the target descriptor, C1).
- Resource model: PCM row/col occupancy, chamber voxel conflicts,
  beam-power budgets.
- Parallelism extraction: `for` over independent lanes → concurrent
  routes; `control { routes: manual }` lets the programmer take over.
- Output: a *schedule* (per-cycle op sets), not a stream — matching how
  the hardware actually runs.

**Gate.** `photonic_massive` scheduled vs. unscheduled: modeled latency
(strictly lower), end-state identical (equivalence gate).

### M-X2 — Wavelength multiplexing in the type system

**Goal.** `wave` types carry channel identity; the scheduler packs
independent channels into shared waveguide time. Surface: `wave chan(n)`
or a `chan` field on beam ops — modeled per the target descriptor's
muxing capability. First language-level expression of advantage #3.

**Gate.** A kernel written with 2 channels reports ≥1.8× modeled
throughput vs. single-channel on the same lattice.

### M-X3 — Zero-cost abstraction audit

**Goal.** The GP side must not tax itself out of parity. Audit pass over
codegen: `println`/`fmt`/`ArrayList`/optionals/generics must emit code as
lean as the C equivalent — measured by host-side perf on the PARITY
kernels (B3 drives this). Findings are bugs, filed and fixed.

### M-X4 — Photonic `matN`/`matvec`/transform primitives

**Goal.** Extend `mat3` upward: `matN[N,N]`/`matvec` primitives that lower
to mesh operations (advantage #4), and the optical-transform primitives
(`fft`) mapped to propagation models in the emulator (advantage #5).
These are the showcase ops for the WIN rows.

**Gate.** `matvec_256` modeled cost in the scorecard vs. the C measured
number — the flagship WIN row.

### M-X5 — Parallel `for` on lanes

**Goal.** `for i in lanes(N)` (or `parallel for`) semantics: independent
iterations lower to concurrent optical lanes through the scheduler.
`photonic_massive` becomes `parallel for` + `@`, not 64 statements.

**Gate.** Same-manifest-or-better output vs. hand-unrolled baseline;
scheduling determinism.

---

## 6. Pillar T — Tooling (a great language ships its toolchain)

### M-T1 — `flash fmt`

Canonical formatter, zero-config, CI-checkable (`--check` mode). Every
example and test reformatted; idempotence test (fmt(fmt(x))==fmt(x)).

### M-T2 — `flash repl` on the emulator

Interactive Flash: expressions evaluate on the PPU32 emulator — trit ops,
`@` interference, typed photonic calls against the chamber model, counter
readout per line. The learning/discovery tool; also the fastest "what does
this do on the hardware" answer an agent can get.

### M-T3 — `flash debug`: the emulator debugger

The genuinely differentiated tool: step a route schedule, inspect trit
registers, PCM grid, chamber state, counters — *pause light*. Breakpoints
on op classes and chamber writes. Runs on `.ppu32` images; source-mapped
back to `.flash` lines via the manifest.

### M-T4 — `flash test` built-in test framework

`@test` blocks, `flash test <file>` runs them (emulator mode for
photonic). Fixtures, asserts, and the property/exhaustive helpers from
H-F2 as library functions. A great language doesn't bolt testing on.

### M-T5 — `flash profile`

Counter-driven profiler: per-region/per-fn photonic cost breakdown from
E3; flame-of-light report (routes × latency × energy). Feeds B3
campaigns.

### M-T6 — `flash lsp`

Language server: parse/sema reuse = diagnostics in-editor for free;
completion over stdlib + `std.ppu32`; hover shows frozen semantics
(TRIT_OPS_V1). Agents use the CLI; humans use the LSP; both get the same
diagnostic codes.

### M-T7 — `flash pkg`

Package story on top of `flash.mod`: named deps, lockfile, integrity
hashes. Small-scope v1 — a real package story without pretending to be
crates.io.

---

## 7. Pillar S — Stdlib and interop (the language people actually write)

### M-S1 — Stdlib expansion to working breadth

Ordered additions to `selfhost/std/`:
1. `math_t` — balanced-ternary math: add/mul/div on `t<width>` with
   carry, conversions, min/max, saturating ops.
2. `str`/`fmt` completion — full format spec coverage (`{t}` digit
   forms), builder APIs.
3. `mem` — allocator zoo: arena, pool, fixed-block, stack; all
   ownership-tracked (A+).
4. `collections` — map/set/queue/ring on `ArrayList` foundations.
5. `io`/`fs`/`net` — hosted runtime surface (exists partially).
6. `sort`, `search`, `hash` — algorithms tuned for ternary.
7. `json` — needed by the benchmark kernels anyway.
8. `time`, `rand`, `log`.

Each module: spec section, tests, and a `control`-level designation
(which level of user needs it).

### M-S2 — C interop (FFI)

`extern "C"` declarations + calling-convention shim on hosted targets:
Flash can call libc and be called by C. The practical-parity story — and
the porting story (existing C code wraps in, then gets rewritten in
Flash).

### M-S3 — `std.photonic` ergonomic layer

The managed-mode photonic API (above `std.ppu32`'s raw surface):
chamber-handle objects, `with` patterns for mode bracketing,
polarity/power newtypes, safe defaults — the L0/L1 developer never
touches register mechanics.

### M-S4 — The porting demos

Three flagship ports proving breadth: a JSON tool, a neural-net inference
demo (the photonic showcase), and a compiler-in-Flash check (selfhost
already exists — exercise it as a demo target). Each is a scorecard
kernel too.

---

## 8. Pillar G — Great-developer-experience & ecosystem

### M-G1 — Tutorial + reference built for agents and humans

- `docs/TUTORIAL.md` — from `println` to `raw { }` in LEVELS order.
- `docs/REFERENCE.md` — generated from the spec + stdlib signatures;
  a drift test keeps it in sync with the compiler's decl tables.
- Machine-readable spec export: the grammar (FLASH_2_0_GRAMMAR.md)
  emitted as machine-parseable EBNF + the diagnostic catalog as JSON —
  agents consume Flash's spec as data.

### M-G2 — Error-message quality program

Beyond codes (V1): the *writing* — every diagnostic reviewed against a
style guide (states what happened, what was expected, the repair, the
waivable knob). A curated "worst 20 messages" cleanup pass; new messages
must pass the guide (lint).

### M-G3 — Reproducible builds + versioning

Deterministic toolchain output (already largely true — byte-identical
fixpoint); versioned spec/ISA/cost-model compatibility matrix
(`docs/COMPAT.md`); semver discipline on spec changes.

### M-G4 — The "agents write Flash" program

The differentiator nobody else is optimizing for: evals where an agent
writes Flash from spec alone — measure error rate, iterate on grammar
irregularities that trip models, fix diagnostics that don't lead to
repairs. Agent-authorship evals in the test suite.

### M-G5 — Showcase repo polish

README rewrite around the win: scorecard front-page, emulator walkthrough,
`flash debug` demo, comparison table vs C claims with provenance. First
impressions for humans and agents alike.

---

## 9. Master ordering (mega view)

```
H-P0  freeze semantics ───────────────────────────────> everything depends
H-P1  vertical slice
H-P2  emulation + measurement ──────────────> B1/B2 (benchmarks need counters)
H-P3  verification + control + diagnostics ──> T-series tooling reuses sema
H-P4  proven optimization ──────────────────> X1 scheduler feeds B3 campaigns
H-P5  daemon (conditional)
B1–B4 benchmark war ──────────────────────── starts after H-E3, runs forever
X1–X5 perf engineering ───────────────────── interleaved with B3 campaigns
T1–T7 tooling ────────────────────────────── parallel, after H-P3 sema stable
S1–S4 stdlib/interop ─────────────────────── parallel, S1 early (bench needs it)
G1–G5 ecosystem ──────────────────────────── continuous
H-P6  polish + certify ───────────────────── last, publishes the scorecard
```

**Critical path:** H-P0 → H-P1 → H-P2 → B1/B2 → the WIN campaigns (B3×X1–X5)
→ certification (H-P6 + scorecard publication).

---

## 10. Non-goals (updated)

Everything in H §12, plus:

- **No claim of beating C on hosted/binary execution** — parity is the
  target there; the win is on photonic hardware, labeled as modeled until
  measured.
- **No benchmark rigging** — matched kernels, semantic equivalence
  required, C side gets best-effort -O2. Winning by computing less is a
  failed gate, not a clever one.
- **No pretending the emulator is hardware** — modeled numbers carry
  model parameters; the scorecard labels every row.

---

## 11. Mega checklist

**Benchmark war (B).**
- [ ] **M-B1** Grand Prix suite: matched Flash/C kernels, differential
      equivalence, harness.
- [ ] **M-B2** `docs/SCORECARD.md` generated, provenance-labeled,
      deterministic.
- [ ] **M-B3** Per-kernel campaigns: every WIN row either flipped or an
      open investigation with a named blocker.
- [ ] **M-B4** `docs/HOW_WE_MEASURED.md` methods doc.

**Performance engineering (X).**
- [ ] **M-X1** Place-and-route scheduler (lanes, PCM occupancy, chamber
      conflicts, schedule output).
- [ ] **M-X2** Wavelength multiplexing: channel-aware wave + scheduler
      packing (≥1.8× modeled throughput on 2 channels).
- [ ] **M-X3** Zero-cost audit: GP codegen ≈ C on parity kernels.
- [ ] **M-X4** `matN`/`matvec`/`fft` primitives → mesh/transform lowering.
- [ ] **M-X5** `parallel for` lane semantics.

**Tooling (T).**
- [ ] **M-T1** `flash fmt` (idempotent, `--check`).
- [ ] **M-T2** `flash repl` on the emulator.
- [ ] **M-T3** `flash debug` — step the schedule, inspect chamber/trit
      regs, source-mapped.
- [ ] **M-T4** `flash test` framework (`@test`, property/exhaustive
      helpers).
- [ ] **M-T5** `flash profile` counter profiler.
- [ ] **M-T6** `flash lsp`.
- [ ] **M-T7** `flash pkg` (deps, lockfile, hashes).

**Stdlib + interop (S).**
- [ ] **M-S1** Stdlib expansion: `math_t`, `fmt` full, `mem` zoo,
      collections, io/fs/net, sort/search/hash, json, time/rand/log.
- [ ] **M-S2** C FFI on hosted targets.
- [ ] **M-S3** `std.photonic` ergonomic layer over `std.ppu32`.
- [ ] **M-S4** Flagship ports: JSON tool, NN inference demo, selfhost as
      demo target.

**Ecosystem (G).**
- [ ] **M-G1** `docs/TUTORIAL.md`, generated `docs/REFERENCE.md`,
      machine-readable spec export.
- [ ] **M-G2** Error-message quality program + style-guide lint.
- [ ] **M-G3** Reproducible builds + `docs/COMPAT.md` version matrix.
- [ ] **M-G4** Agent-authorship evals in CI.
- [ ] **M-G5** README/showcase rewrite around the scorecard.

**Victory definition (the campaign's finish line):** `docs/SCORECARD.md`
shows WIN on every physics-winnable row, PARITY on serial rows, zero OPEN
rows — all under declared models, reproducible by command.

---

## 12. The honest finish

C took 50 years, the whole software industry, and every binary fab on
Earth to get where it is. Flash's edge is that its home turf is a physics
C was never built for — light as compute. The plan's job is to make that
edge *provable*: semantics frozen, machine modeled honestly, benchmarks
matched fairly, every number labeled, every win demonstrated. The
scorecard, not the slogan, is the victory.
