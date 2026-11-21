# PPU32 ISA v1 — Frozen Semantics

**Status:** frozen semantics contract (FLASH_H_PLAN P0). The emulator,
emitter, reference interpreter, and every test must implement *this*
document; when code and this doc disagree, the doc wins and the code is a
bug — or the doc is amended through a spec change (never silently).

**Companion:** `TRIT_OPS_V1.md` (trit arithmetic), `FLASH_H_PLAN.md`.

---

## 1. Processor model

PPU32 is a balanced-ternary **photonic processing unit**: a route
schedule executed against an Active Lattice chamber plus a PCM
(phase-change material) grid and a trit register file. There is no
program counter, no fetch/decode loop, and no global clock: a `.ppu32`
image is an ordered **schedule** of operations, each of which completes
in a small integer number of lattice-steps (§6 latency model). The
machine is deterministic: same image, same initial state, same final
state.

### 1.1 Machine state

| Component | Contents |
|---|---|
| `Treg[0..63]` | trit register file; each cell holds one `t9` word (9 trits) |
| `Pcm` | sparse phase-cell table, addressed `(row,col)`, row 0..4095, col 0..63; cell phase in µrad, stored as s32 |
| `Chamber` | Active Lattice voxel grid, edge `N` (default 8, descriptor-settable); each voxel: polarity trit, phase µrad, amplitude µmW, occupied flag |
| `Lattice` | routing fabric, 128³ cells at 1mm pitch (128mm³); `LatticeRoute` traverses it and marks occupancy; where the route crosses the chamber volume it also writes chamber occupancy+amplitude |
| `Lasers` | `laser_mw[wall][slot]`, 4 walls × 1 slot, µW units |
| `raw_active` | mode flag: `Raw*` classes require `raw_active = 1` |
| `target` | last targeted voxel coordinate `(x,y,z)` |
| `pipe` | streaming pipeline: `(col, dep, head_z, active)` |
| `fault` | sticky fault code, 0 = none; once set, schedule halts |

### 1.2 Word layout

Every instruction is a 32-trit word: **top 8 trits = class id, low 24
trits = operands**. The text assembly (`.ppu32`) used today is the
human-readable serialization of that word; each `insn` line is one word.
Operand fields are named per class below; the emitter serializes them as
`key=value` pairs. Field widths in the packed form are given per operand;
values that overflow their field fault with `F_ENCODING` (§7).

## 2. Classes

Class ids are fixed forever by this table. Unused ids are reserved;
executing one faults `F_ENCODING`.

| id | class | mode req | summary |
|---:|---|---|---|
| 0 | `Nop` | — | no effect, advances schedule |
| 1 | `PcmPhaseShift` | — | `Pcm[row][col] += phase_a + phase_b` (µrad; stored raw) — the `result=` field in text asm is the emitter's declared semantic output and is informational only |
| 2 | `PcmCouple` | — | couples `Pcm[row][col]` into the interference path: contributes the cell's phase to the next `TritAlu`/`@` evaluation on `trit_dst` |
| 3 | `LatticeRoute` | — | optical path `from→to` in mm·1000 (µm); marks lattice cells along the segment (1mm pitch, edge 128); sets `target` to `to`; chamber-crossing voxels gain occupancy+amplitude |
| 4 | `TritRegLoad` | — | `Treg[trit_dst] = trit_a` (trit_a encodes a t9 word, see §3) |
| 5 | `TritAlu` | — | `Treg[trit_dst] = alu_op(Treg[trit_a], Treg[trit_b])`; `Neg` ignores trit_b |
| 6 | `RawFireShield` | raw | inject power pulse at voxel `target` or explicit coords: occupancy=1, amplitude += power |
| 7 | `RawInjectPhase` | raw | set voxel phase at `(x,y,z)` to `phase_micro_rad` |
| 8 | `RawSetLaserMw` | raw | `laser_mw[wall][slot] = power_micro_mw` (wall, slot in operand fields) |
| 9 | `RawTargetVoxel` | raw | `target = (x,y,z)`; faults `F_BOUNDS` if out of chamber |
| 10 | `RawSetPolarity` | raw | set voxel polarity tag at `(x,y,z)` to `polarization` (−1/0/+1) |
| 11 | `RawModeEnter` | — | `raw_active = 1`; returns 0 (faults `F_MODE`) if already active |
| 12 | `RawModeExit` | — | `raw_active = 0` |
| 13 | `RawReadVoxel` | raw | read voxel word at `(x,y,z)` into `Treg[trit_dst]` (packed §4); faults `F_BOUNDS` out of range |

## 3. Operand field encodings

Each class uses a fixed subset of the operand fields. Fields not listed
for a class **must be zero** in packed form; the text form omits them.

| field | used by | encoding |
|---|---|---|
| `row`, `col` | PcmPhaseShift, PcmCouple | u24 each; row ≤ 4095, col ≤ 63 valid; else `F_BOUNDS` |
| `phase_micro_rad` | PcmPhaseShift (as phase_a/phase_b pair), RawInjectPhase | s24 µrad; PcmPhaseShift carries two fields `phase_a`, `phase_b` plus `result` (the emitter's predicted sum — the emulator recomputes and faults `F_ENCODING` on mismatch, keeping text asm self-checking) |
| `from_x/y/z_mm`, `to_x/y/z_mm` | LatticeRoute, Raw* (as `x`,`y`,`z` keys) | s24 µm each |
| `power_micro_mw` | LatticeRoute, RawFireShield, RawSetLaserMw | u24 µW·10^-3 (i.e., µmW); must be ≥0 else `F_ENCODING` |
| `polarization` | LatticeRoute, RawSetPolarity | s2 trit: −1, 0, +1; anything else faults `F_ENCODING` |
| `trit_a`, `trit_b`, `trit_dst` | TritRegLoad, TritAlu, PcmCouple, RawReadVoxel | u6 register index 0..63 for a/b/dst; for `TritRegLoad`, `trit_a` field carries a packed t9 immediate (§3.1) |
| `alu_op` | TritAlu | u4: 0=Add, 1=Mul, 2=Nand, 3=Neg; else `F_ENCODING` |
| `wall`, `slot` | RawSetLaserMw | u4 each; slot must be 0, wall 0..3, else `F_BOUNDS` |

### 3.1 Packed t9 immediates

A `t9` word packs 9 trits, 2 trits-worth of information each but stored as
3 values per trit position: positions 0..8, coefficient `3^i`, each digit
in {−1,0,+1} mapped to bits as (−1→0b10, 0→0b00, +1→0b01) in 18 bits.
`TritRegLoad` carries this in `trit_a`. Range ±(3^9−1)/2 = ±9841.

## 4. Voxel word (RawReadVoxel result)

The 64-trit-wide voxel word (matches `chamber_sim.zag::encode_voxel_word`):

| bits | field |
|---|---|
| 0..1 | polarity (balanced-trit encoded: 00=0, 01=+1, 10=−1) |
| 2..25 | phase µrad (s24) |
| 26..49 | amplitude µmW (u24) |
| 50..57 | occupancy / payload byte |
| 58..63 | reserved, must read 0 |

Out-of-chamber reads fault `F_BOUNDS`, do not write `trit_dst`.

## 5. Instruction semantics (per class, precise)

```
Nop:                — no state change.
PcmPhaseShift:      Pcm[row][col] ← Pcm[row][col] + phase_a + phase_b.
PcmCouple:          pending_couple[trit_dst] += Pcm[row][col].
LatticeRoute:       for each voxel on segment from→to (Bresenham-3D on
                    chamber grid): occupied=1, amplitude += power.
                    target ← to. power ≥ 0, pol ∈ {−1,0,+1} required.
TritRegLoad:        Treg[dst] ← immediate trit_a.
TritAlu:            Treg[dst] ← alu_op(Treg[a], Treg[b]) per TRIT_OPS_V1
                    (Add = balanced ternary add w/ carry out dropped —
                    modular 3^9; Mul = ternary multiply mod 3^9;
                    Nand = trit-wise NAND; Neg = digit-wise negation).
                    If pending_couple[dst] ≠ 0, the coupled phase rotates
                    the result's trit 0 by consensus (§TRIT_OPS_V1 §6).
RawFireShield:      voxel at (x,y,z) or target: occupied=1,
                    amplitude += power_micro_mw. Requires raw_active.
RawInjectPhase:     voxel(x,y,z).phase ← phase_micro_rad. raw_active.
RawSetLaserMw:      laser_mw[wall][slot] ← power. raw_active.
RawTargetVoxel:     target ← (x,y,z); F_BOUNDS if outside. raw_active.
RawSetPolarity:     voxel(x,y,z).polarity ← pol ∈ {−1,0,+1}. raw_active.
RawModeEnter:       F_MODE if raw_active=1, else raw_active ← 1.
RawModeExit:        raw_active ← 0.
RawReadVoxel:       Treg[dst] ← voxel word at (x,y,z). raw_active.
```

`raw_active` violations on classes 6–10, 13 fault `F_MODE`.

## 6. Latency & energy model (cost model v1, declared parameters)

The schedule advances in **lattice-steps** — the time light crosses one
voxel edge. This is the unit `flasht stat` reports; it is *modeled*, never
host-measured. Declared parameters (v1):

| parameter | value | unit |
|---|---|---|
| `STEP` | 1 | lattice-step per voxel traversal |
| `T_ALU` | 1 | lattice-steps per TritAlu/TritRegLoad |
| `T_PCM` | 2 | lattice-steps per Pcm op |
| `T_RAW` | 1 | lattice-steps per Raw* op |
| `ROUTE_LEN` | segment voxel count | lattice-steps per LatticeRoute |
| `E_LASER` | power µmW × steps | energy per route = Σ power×len |
| `E_ALU` | 1 | unit-energy per trit op |
| `E_PCM` | 3 | unit-energy per PCM write |

Interference ops (`@` via TritAlu + PcmCouple) evaluate in `T_ALU` — this
is the photonic-parallel win: a 9-trit op is one step, not nine.

Concurrent instructions on disjoint lanes share a step (see scheduler,
H-X1); the v1 cost model counts sequential sum **and** lane-packed
latency — the scorecard uses packed latency, labeled.

## 7. Fault model

| code | name | raised when |
|---|---|---|
| 0 | none | — |
| 1 | `F_ENCODING` | bad class id, reserved fields nonzero, illegal enum/pol value, negative power, emitter `result` field ≠ a+b |
| 2 | `F_BOUNDS` | row > 4095, col > 63, voxel out of chamber, lattice coord out of range, reg index > 63, bad wall/slot |
| 3 | `F_MODE` | Raw* op without `raw_active`; `RawModeEnter` twice |

Faults halt the schedule; there is no recovery in v1.

Faults are sticky and halt the schedule at the faulting instruction;
the emulator prints `fault <code> at insn <n>` and exits nonzero.
There is no trap handler in v1 — faults are machine stops, matching
"broken code doesn't ship": a faulting image is invalid.

## 8. Text assembly (.ppu32)

```
# ppu32 assembly v1
target = "ppu32"
name = "example"
format = "ppu32-v1"
[insns]
insn 0 TritRegLoad dst=0 imm=0t+++
insn 1 TritRegLoad dst=1 imm=0t+-0
insn 2 TritAlu op=add a=0 b=1 dst=2
insn 3 LatticeRoute from_x=0 from_y=0 from_z=0 to_x=2000 to_y=0 to_z=0 power=500 pol=1
insn 4 RawReadVoxel x=0 y=0 z=0 dst=3
```

Parser rules: `insn <idx> <Class> key=value…`; unknown class or key →
`F_ENCODING`; indices need not be contiguous but execute in order.

## 9. Non-goals

- No packed-binary image format yet — text asm is v1 canonical (packed
  word layout defined in §1.2/§3 for the eventual binary form).
- No interrupts, traps, or branches — a schedule is straight-line by
  design in v1 (control flow lives in the compiler's schedule emission).
- No floating point — all values are fixed-point µ-units.
