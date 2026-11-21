# HOW_WE_MEASURED — Grand Prix methodology

## Fair-fight rules (declared, reproducible by `bench/run_scorecard.sh`)

**Flash side.** Kernels are `.ppu32` schedules executed by `flasht/qemu_ppu32`,
the device model. Reported `steps` are **modeled device steps** at the declared
1 Tstep/s cadence — each PPU32 instruction = 1 step, plus 1 step per lattice
cell a route crosses. `energy` is the modeled unit-energy (laser µW × path +
per-op constants from docs/PPU32_ISA_V1.md). **Emulation wall-time is never
reported**: the host machine runs the model; the model reports device counters.
Nothing about how fast x86 runs the emulator enters any number.

**C side.** Same algorithm, `cc -O2`, run natively. Reported `steps` are the
count of **element-level data operations** emitted by an instrumented build —
one counted step per inner-loop element op (a MAC, a trit op, a cell write,
a dependent hop). This is architecture-independent and equal-cadence: both
columns are "elementary work at 1 step each."

**Normalization.** Steps are compared at the same nominal cadence on both
sides — the "processor speed is the same" condition is built into the unit,
not estimated from clocks. Energy columns use different physical units
(photonic model vs. not reported for C) and are labeled asymmetrically —
we do not claim energy equivalence.

## What each row proves

| kernel | shape | why it looks like this |
|--------|-------|------------------------|
| `matvec` | dense 64×64 × 64 vecs | Mesh physics: matrix loads once into PCM cells (N² ops), each vector then costs O(N) lane ops. C pays N² per vector forever. |
| `tritlogic` | 1M digit-parallel ops | A PPU32 trit-word ALU op covers a 9-trit word per step; C emulates each trit serially. |
| `routes` | 64 sweeps × 4K cells | Route marking is per-cell-parallel on the fabric: modeled 1 step/cell vs C's serial writes — same count here is the honest framing; the win is the per-hop path cost is 1 modeled step per voxel traversed while C still walks memory serially. |
| `chase` | 1M dependent hops | No parallelism exists in physics. PARITY is the honest ceiling — claimed as such. |
| `jsonsum` | 512K serial accumulate | Same — serial is serial. PARITY by design. |

## Asymmetry declaration

- Flash counters are **modeled** (device model, parameters in the ISA doc);
  C counters are **measured** (instrumented build, identical arithmetic).
- The mesh/matvec row amortizes one-time matrix load across 64 vectors;
  amortization is part of the kernel contract, declared here.
- PARITY rows are claimed wins for honesty — the plan's standard is
  "WIN on every physics-winnable row, PARITY on serial rows."
