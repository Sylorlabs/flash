# Flash

Flash is a **balanced-ternary-native** photonic language that compiles to Zag.
Balanced ternary is not a feature of Flash — it is Flash's home radix, the way
base 2 is C's. Surface programs (`.flash`) describe trit logic, numeric lanes,
wave carriers, optical matrix routes, and optional `raw { }` hardware blocks;
the full language is a general-purpose systems language over the same ternary
type system. The `flashc` driver lowers both to Zag shims plus a coordinate
manifest for spatial routing.

## Balanced ternary is the native radix

The numeric catalog (`flash/types/numeric/`) is organized around one rule:
**native types never pay for their own radix; foreign types pay for theirs.**

- **Native family — `trit` and `t<width>`.** `t9`, `t27`, … are balanced-ternary
  integers: width counted in **trits**, range the symmetric ±(3^width − 1)/2,
  sign intrinsic to the digits (balanced ternary has no unsigned variant —
  negation is digit-wise, not an interpretation bit). `t1` *is* the trit, and
  the t-family owns the first slots of the type catalog. On ternary hardware a
  t-unit is exactly its digits: `trit_encoding_bits == width`, an identity.
  On today's binary bootstrap hosts a t-unit lowers to a plain machine integer
  with **zero added cost** — t-units promise *range* semantics, never base-2
  wraparound, so no masking or wrap-emulation code exists to emit.
  Demo: `examples/tunit_demo.flash`.
- **Interop families — `i*`/`u*`, floats, posit, sat, fixed.** These keep their
  exact value and arithmetic contracts everywhere, but their *digit-level*
  semantics (bitwise ops, mod-2^w wraparound) are base-2 promises. Each
  descriptor prices that embedding: `trit_encoding_bits` is what the type costs
  to carry on ternary hardware (an `i8`'s mod-256 wrap needs a real modulo —
  256 is not round in base 3). Use them for interop and porting; use t-units
  when you mean a number.
- **Families are cast-only across radix.** `t9 + i32` is a radix change, not a
  widening — it requires an explicit cast, exactly like `posit as i32`.
- **The bootstrap flips, the contracts don't.** Today the binary host emulates
  the ternary family (a trit rides in a byte); on real ternary/photonic
  hardware the binary families become the emulated ones. Programs written
  against the native family inherit the hardware win with no source change.

## Bootstrap

Flash **1.0 is standalone**. Clone this repo and run:

```bash
./build.sh
```

No sibling `zag` repo, no system packages, no external toolchain. `build.sh`
bootstraps `flashc1` from the bundled native backend at `flash/toolchain/flashz`
(the Zag compiler, vendored inside Flash), installs `flashc`, and runs the full
test suite (`tests/run_all.sh`). CI uses the same single command.

Override the backend only when debugging:

```bash
FLASHZ=/path/to/flashz tests/run_all.sh
```

(`ZNC` is accepted as an alias for `FLASHZ`.)

## Certification scorecard (Wave 7)

| Pillar | Score | Notes |
|--------|------:|-------|
| Stage-0 bootstrap | 100% | Surface + full-language `flashc` path |
| GP full language | 100% | `fn`/structs/enums/control flow end-to-end |
| Stage-1 self-host | 100% | `flashc1` lexer → parser → codegen in Flash |
| Bootstrap fixpoint | 100% | `flashc1` → `flashc2` byte-identical self-compile |
| Stage-1 completeness | 100% | Manifests, raw blocks, mat3, CLI parity with stage-0 |
| Library ports + stdlib | 100% | `selfhost/lib/*`, `selfhost/std/*` seeded |
| Slim runtime shims | 100% | `flash/stdlib/flash_slim.zag` headers |
| Self-hosted driver | 100% | `build.sh` → `flashc` is `flashc1` |
| Language extensions | 100% | `pub`, `extern`, `error { }`, GPU dispatch |
| Language completion | 100% | `try`/`catch`, annotations, `group`/`defer` |
| Graphics stack | 100% | softgpu → vgpu → retained UI engine |
| Standalone 1.0 | 100% | **Bundled `flashz` backend** — no sibling zag repo required |

All pillars are at **100%** for Flash 1.0. The honest caveat: native codegen still
runs through the vendored Zag compiler (`flashz`); Flash owns and ships that
binary so consumers never clone `zag` separately.

## Test

```bash
tests/run_all.sh                 # canonical comprehensive runner
tests/critic/run_critic.sh       # adversarial critic tests only (delegates to run_all.sh)
tests/standalone_gate.sh         # Wave 7 gate: bundled flashz, no sibling zag
```

## Operating-system status

Flash is ready today for hosted systems programs and RayOS's native-host
foundation. It is not yet a freestanding kernel compiler. In particular,
`ppu32` lowers photonic surface routes, not arbitrary Flash functions, and the
compiler now rejects attempts to turn an ordinary program into an empty PPU32
image. The proposed `rayos-ternary` target also fails explicitly rather than
substituting a hosted executable.

The proven boundary and the remaining kernel-backend work are tracked in
[`docs/OS_READINESS.md`](docs/OS_READINESS.md). Flashy is the Flash UI toolkit;
it should gain a RayOS platform backend rather than being duplicated inside the
language or operating system.

## File extension

Flash sources use **`.flash`**. The legacy `.flash.zag` extension is still
accepted everywhere (detection is by content, not extension) — the compiler
testdata keeps a few `.flash.zag` files on purpose as back-compat coverage.

## Surface syntax vs full language

`flashc` now compiles two source flavors, auto-detected by the first token:

- **Surface syntax** — files starting with `program`, parsed by
  `flash/compiler/parse.zag` and lowered through `flash_sema_surface.zag` +
  `flash_codegen.zag`. Declarative trit/wave/numeric bindings plus `raw { }`.
- **Full language** — everything else: general-purpose Flash with `fn`, structs,
  enums, `if`/`while`/`switch`, generic calls, slices, and trit/photonic types.
  Parsed by `flash_parse.zag` and lowered near-identity to Zag by
  `flash_emit_zag.zag`. Type checking is currently delegated to `znc`
  (stage-0 bootstrap posture); `flash_sema.zag` tightens over time.

## Self-hosting roadmap

Flash is bootstrapped off Zag and is being made self-hosted in stages:

1. ✅ **Stage 0** — `flashc` written in Zag (surface + full-language paths).
2. ✅ **GP core** — the full language compiles end-to-end (`examples/gp_demo.flash`).
3. ✅ **Stage 1 pipeline in Flash** — a working compiler written in Flash,
   compiled by stage 0. `selfhost/flashc1.flash` drives
   `slex.flash` (lexer) → `sparse.flash` (parser) →
   `scodegen.flash` (Zag-shim codegen); its output is **byte-identical**
   to stage-0 shims for the supported subset (trit/wave/p32/sat + summaries),
   verified in the test suite. Full-language sources can import other Flash
   sources with `@import("x.flash")` (decls spliced into one compilation,
   deduped). No C anywhere in the stack: Flash → Zag → `znc` native binaries.
4. ✅ **Bootstrap fixpoint** — Flash is self-hosted. `flashc1` also carries a
   full-language pipeline in Flash (`flex.flash` lexer →
   `fparse.flash` pool-AST parser → `femit.flash` Zag emitter) and
   **compiles its own sources**: stage-1's self-compiled shim is
   byte-identical to stage-0's, the resulting `flashc2` recompiles the same
   sources to the same bytes (and the same binary), and still matches stage-0
   on surface programs. Verified by `run_fixpoint` in the test suite.
5. ✅ **Stage-1 completeness — stage-0 can retire.** The Flash-written
   compiler covers everything the suite exercises: raw `{ }` blocks with the
   voxel probe, mat3 optical ops, the full coordinate-manifest emitter
   (linking the Zag spatial libraries — `HardwareManager`, path routing,
   `manifest_write` — the same way it links `znc`'s runtime), surface
   semantic validation, enums, switch, and a flashc-compatible CLI
   (`-o`, `--emit-manifest`, `--emit-zag`, `--check`, `--run`) that invokes
   `znc` itself. Shims **and manifests** are byte-identical to stage-0 for
   every example. The entire test suite passes with the stage-1 compiler
   substituted for stage-0:

   ```bash
   ./flashc selfhost/flashc1.flash -o .test_bin/flashc1
   cp .test_bin/flashc1 .test_bin/flashc1_ret
   FLASHC="$PWD/.test_bin/flashc1_ret" tests/run_all.sh   # 42/42, no stage-0
   ```

6. ✅ **Widened subset + library ports + stdlib seed.**
   - Full language now covers unions, switch captures, methods (mangled to
     `Type_name`), generic fns/structs, optionals (`?T`/`null`/`orelse`), and
     if-captures — in both compilers, byte-identically
     (`examples/widen_demo.flash`).
   - The spatial/manifest libraries are ported to Flash
     (`selfhost/lib/*.flash`, staged `_fl` symbols) and proven equivalent
     by a differential probe against the Zag originals (byte-identical output
     incl. full manifests). Generated shims import `flash/stdlib/flash_slim.zag`
     (runtime types without the session/spatial originals), so the Flash
     ports can claim those names when wired in.
   - Flash stdlib seed: `selfhost/std/str.flash` (strings + paths),
     already used by `flashc1`.
   - Upstream Zag gained a native `_zag_getenv` runtime routine (envp walk in
     znc's x86 backend); `flashc`/`flashc1` honor `ZNC` env, `--znc`,
     `flash.mod`, and probe fallbacks, in that order.

7. ✅ **Slim-runtime shim header** — generated shims import
   `flash/stdlib/flash_slim.zag` (runtime types without photonic session /
   Zag spatial originals), freeing names for the Flash lib ports.
8. ✅ **Self-hosted bootstrap + stdlib seed + compiler port swap.**
   - `build.sh` bootstraps via `flashc1`; `flashc` is the self-hosted driver.
   - Stage-1 surface codegen (`scodegen.flash`) links `selfhost/lib/*.flash`
     ports; shims and manifests remain byte-identical to stage-0.
   - `selfhost/std/` grew: `str.flash`, `list.flash`, `io.flash`.
   - Full-language `--check` runs `flash_sema.zag` (stage-0); stage-1 parses on
     `--check` (sema port pending).
   - `ui_engine_demo` added to the test suite (42/42).
9. ✅ **Decoupling + language extensions.**
   - `selfhost/std/process.flash` and `zagmod.flash` replace Zag CLI deps in
     `flashc1`; `fsema.flash` + `fsema_surface.flash` power stage-1 `--check`.
   - `engine_dispatch_kernel()` routes GPU paths (CPU fallback + dispatch log).
   - Full language: `pub`, `extern fn`, `error { }` sets (`lang_ext_demo.flash`).
   - Shim headers still use Zag `list.zag` (Flash `list.flash` is spliced only
     when deduped); bootstrap seeds via `.test_bin/flashc1_old` when stage-0
     cannot compile the full closure. **43/43** tests.
10. ✅ **Language completion — catch, annotations, group, stdlib.**
   - Full language: `try`/`catch`/`orelse`/`!T`/`?T` sugar in stage-1
     (`fparse.flash` + `femit.flash`); `error_catch_demo.flash`.
   - `@photonic` / `@realtime` annotations on fn/struct decls preserved in
     emitted shims (`annot_demo.flash` + stage-1 annot emit test).
   - `group { }` / `group name { }` blocks (lowered to scoped `if (1)`),
     `defer`, and numeric underscore literals (`grouping_demo.flash`).
   - Stdlib seed: `selfhost/std/fmt.flash` (int format + join/println) and
     `vec.flash` (ergonomic `Vec[T]` over `ArrayList`); `stdlib_demo.flash`.
   - `f1_collect` skips `list.flash` imports; shim headers keep Zag
     `list.zag` (no duplicate `ArrayList` defs). Bootstrap via
     `.test_bin/flashc1_old` when stage-0 cannot compile the closure.
     **49/49** tests.
11. ✅ **Graphics + UI engine** — softgpu CPU fallback through vgpu v6
    (faces, damage tracking) and retained `ui_engine_demo`; full graphics
    pillar at 100%.
12. ✅ **Standalone Flash 1.0** — `flash/toolchain/flashz` ships in-repo;
    `build.sh` is the only bootstrap step (no sibling `zag`, no external deps).
    `flashc1.flash` resolves `flash/toolchain/flashz` before legacy
    `../zag` fallbacks. Verified by `tests/standalone_gate.sh`.

**Post-1.0** — emit Flash `list.flash` instead of Zag `list.zag`; native GPU
present path.

## Graphics: softgpu fallback driver

Flash targets ternary systems with **no GPU driver stack**. Until native
drivers exist for the real hardware, `flash/driver/softgpu.flash`
(written in Flash) provides general graphical compute on the CPU: an RGB
framebuffer, raster primitives, per-pixel compute kernels (including a
balanced-ternary interference kernel; trit colors follow manifest
polarization − H / split / V), and a present-device abstraction — portable
PPM files by default, best-effort `/dev/fb0` blit, with future native
ternary/photonic display drivers registering as new device kinds behind the
same `softgpu_present()`. Demo: `examples/gfx_demo.flash`.

## Remaining architecture

- **Full parser (`flash/compiler/flash_parse.zag`)** — available for tooling and
  regression tests (`flash_parse_test.zag`, integration parity checks). `flashc`
  uses the lighter surface parser in `parse.zag` by default.
- **Ternary math (`flash/stdlib/trit_math.zag`)** — canonical znc-safe balanced-
  ternary arithmetic and truth tables; prefer this over `flash/ternary/trit.zag`
  when linking with `znc`.
- **Mat3 (`flash/types/mat3.zag`)** — canonical 3×3 struct and f32 ops (`mat3_identity`,
  `mat3_mul`, `mat3_near`). `matrix_rt.zag` adds wave/optical routing;
  `matrix_shim.zag` and `matrix_codegen.zag` import the shared type.
- **Raw sim (`flash/runtime/raw_sim.zag`)** — znc-safe raw-mode chamber driver for
  tests and software simulation (used by critic `harsh_raw`).

## Example

Compile, emit the coordinate manifest, build with `znc`, and run:

```bash
./flashc examples/full_demo.flash --emit-manifest --run
```

Other examples live under `examples/`:

- `full_demo.flash` — trits, posit, sat lanes, and raw chamber probes
- `ternary_logic.flash` — balanced-trit arithmetic and logic
- `numeric_spectrum.flash` — heterogeneous numeric families
- `raw_chamber.flash` — raw hardware block + voxel readback
- `optical_mat.flash` — `mat3` optical transforms (nine path segments)
- `error_catch_demo.flash` — `try`/`catch`/`orelse`, error sets, `!T`/`?T`
- `annot_demo.flash` — `@photonic` / `@realtime` annotation emit
- `grouping_demo.flash` — `group` blocks, `defer`, numeric `_` literals
- `stdlib_demo.flash` — `fmt.flash` + `vec.flash` stdlib modules

Generated shims (`*_shim.zag`) and binaries (`*.out`) are build artifacts and are
gitignored.
