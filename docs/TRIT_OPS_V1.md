# Trit Ops v1 — Frozen Semantics

**Status:** frozen semantics contract (FLASH_H_PLAN P0). Every trit
operation in the language, the emulator, the reference interpreter, and
the optimizer's rewrite rules is defined by *this document's truth
tables*. When code disagrees, the doc wins — or the doc is amended
through a spec change, never silently.

**Companion:** `PPU32_ISA_V1.md`, `FLASH_2_0_SPEC.md`.

---

## 0. Resolution recorded here

The 2.0 plan (W5) spec'd `@` as *digit-wise trit multiplication*; the
shipped tests (`tests/trit/interfere.flash`) implement *consensus*.
**Resolved to consensus** — it matches the tests, matches physical
superposition (equal phases reinforce, opposite phases cancel, zero
phase passes through), and keeps numeric multiply available separately
as `*` / `trit_mul`. `FLASH_2_0_PLAN.md` W5 and `FLASH_2_0_SPEC.md` are
amended by this document.

## 1. The trit

A trit `t ∈ {−1, 0, +1}`, written `-`, `0`, `+`. A `tW` word is W trits,
value `Σ digit_i · 3^i`, range `±(3^W − 1)/2`. Widths: `t3`, `t9`, `t27`,
`t81`. Encoding per `PPU32_ISA_V1 §3.1`.

## 2. Digit operations — complete truth tables

### 2.1 `neg` — digit-wise negation (free in balanced ternary)

| a | −a |
|---|---|
| − | + |
| 0 | 0 |
| + | − |

Digit-wise on every position; no carry, no special case. `-x` for `x=MIN`
is `MAX` — symmetric range means **no overflow**.

### 2.2 `cons` — consensus / interference (`@`) — CANONICAL

| a \ b | − | 0 | + |
|---|---|---|---|
| **−** | − | − | 0 |
| **0** | − | 0 | + |
| **+** | 0 | + | + |

Physical reading: equal phases reinforce (`+@+=+`, `-@-=-`), opposite
phases cancel (`+@-=0`), zero passes through (`0@x=x`, `x@0=x`).
Applied digit-wise across the word: `c[i] = cons(a[i], b[i])`.

Properties (used by the optimizer's proof obligations):
- Commutative: `a @ b = b @ a` ✓ (table symmetric)
- Associative: `(a @ b) @ c = a @ (b @ c)` — **false**. Example:
  `+ @ (+ @ −) = + @ 0 = +`, but `(+ @ +) @ − = + @ − = 0`. The emitter
  and every rewrite rule must respect left-associativity as defined by
  the language, never re-associate.
- Identity: `a @ 0 = a` ✓
- Idempotent: `a @ a = a` ✓
- Self-annihilation: `a @ neg(a) = 0` ✓ (every position cancels)
- Absorption: none beyond identity — `+ @ − = 0`, `0` is *not* absorbing:
  `0 @ x = x`. Note `cons` has no zero element that annihilates.

### 2.3 `trit_mul` — numeric ternary multiply digit (for `*` / TritAlu Mul)

| a \ b | − | 0 | + |
|---|---|---|---|
| **−** | + | 0 | − |
| **0** | 0 | 0 | 0 |
| **+** | − | 0 | + |

This is the multiply **digit** used inside `TritAlu Mul` (whole-word
ternary multiplication mod 3^W uses this per-position with carry
propagation per §3.2). It is *not* what `@` does — `@` is `cons`.

### 2.4 `nand` — trit-wise NAND (TritAlu Nand)

`nand(a,b) = neg(min(a,b))` — universal gate in trit logic:

| a \ b | − | 0 | + |
|---|---|---|---|
| **−** | + | + | + |
| **0** | + | + | 0 |
| **+** | + | 0 | − |

### 2.5 `min` / `max` — trit comparison digits

`min`: `min(−,*)=−`; `min(0,+)=0`; `min(+,+)=+`.
`max`: `max(+,*)=+`; `max(0,−)=0`; `max(−,−)=−`.

### 2.6 `txor` — digit-wise ternary XOR (`^`), sum mod 3 mapped to balanced

| a \ b | − | 0 | + |
|---|---|---|---|
| **−** | + | − | 0 |
| **0** | − | 0 | + |
| **+** | 0 | + | − |

i.e. `txor(a,b) = ((a+b+1) mod 3) − 1` over {−1,0,+1}. Self-inverse:
`txor(a, txor(a,b)) = b`.

## 3. Word operations

### 3.1 Addition (`+`, TritAlu Add)

Balanced-ternary add with per-position carry. Digit add of `a_i + b_i +
carry` ∈ {−3..3}: result digit = value mod 3 mapped to balanced
{−1,0,+1}, carry = value div 3 (round-toward-zero: −3→carry −1,
+3→carry +1, −2→digit +1 carry −1, +2→digit −1 carry +1).

| sum | digit | carry |
|---:|---:|---:|
| −3 | 0 | −1 |
| −2 | +1 | −1 |
| −1 | −1 | 0 |
| 0 | 0 | 0 |
| +1 | +1 | 0 |
| +2 | −1 | +1 |
| +3 | 0 | +1 |

Overflow past the top trit is **dropped** (mod 3^W) — ternary words wrap
modularly, defined semantics (unlike C signed overflow UB).

### 3.2 Multiplication (`*`, TritAlu Mul)

Shift-add over trit digits using §2.3 digit multiply, modular mod 3^W.
`a * −b = −(a * b)` holds exactly (symmetric range — no MIN asymmetry).

### 3.3 `@` word interference

`c = a @ b`: `c[i] = cons(a[i], b[i])` for all positions — **one
interference step on hardware**, no carry, no loop. This is the
photonic-parallel primitive: evaluated in `T_ALU` on PPU32.

### 3.4 `^` word txor — digit-wise, no carry.

### 3.5 Shifts — `<<` / `>>` shift the *digit vector* (multiply/divide by
3^k, mod 3^W). This is the ternary analog of binary shifts ×2 — on
ternary hardware shifting left one position is ×3.

## 4. Literals

`0t<digits>` — balanced-ternary literal, `+`=+1 `0`=0 `-`=−1, MSB first.
`0t++` = 4, `0t+-` = 2, `0t-` = −1. Width inferred from context (`t9`
etc.); literal must fit the target width else `F_ENCODING`/compile error.

## 5. Comparisons

Ordered by numeric value: `−1 < 0 < +1` per trit; words compare
numerically. `==` is digit-wise equality.

## 6. PCM coupling rule (PPU32 `PcmCouple` interaction)

When `pending_couple[dst] ≠ 0` at a `TritAlu` write, the result's trit 0
is rotated by consensus with `sign(phase)`:
`Treg[dst][0] = cons(result[0], sign(phase µrad))` — a phase-shifted
beam steers the trit. `pending_couple` clears after the write.

## 7. Proof obligations for the optimizer (O1)

Rewrite rules must preserve these tables exactly:
- `a @ a → a` (idempotent) — proven by §2.2 diagonal.
- `a @ 0 → a` — proven by §2.2 row/col.
- `a @ neg(a) → 0` — proven per-position.
- `neg(neg(a)) → a`.
- `a + 0 → a`; `a * 1 → a`; `a * 0 → 0`; `a * −1 → neg(a)`.
- NOT valid: any reassociation of `@`; any rule assuming `cons` absorbs.
- NOT valid: `a @ b → a*b` — consensus ≠ multiplication (the frozen fix).
