# Flash 1.0 → 2.0 Migration Guide

**Status:** Active. This guide helps migrate Flash 1.0 code to Flash 2.0.

---

## 1. Overview

Flash 2.0 is a significant evolution of the language:

- **Unified grammar**: The surface/full dialect split is gone. One parser
  handles everything.
- **Typed photonic API**: `raw { }` blocks are replaced with type-safe
  Flash-native functions.
- **Ternary-native**: `tritvec[N]`, `0t` literals, and the `@` interference
  operator are first-class.
- **Effects system**: `@photonic` is enforced at compile time.
- **Type inference + methods**: `c.doubled()` and `push(&xs, 9)` work.
- **`const`**: Compile-time constants for array sizes and type params.
- **Flash-native stdlib**: No more direct `_zag_*` calls in user code.

---

## 2. Mechanical translation table

| 1.0 | 2.0 | Workstream |
|-----|-----|------------|
| `_zag_print("x="); _zag_println(_zag_i64_to_str(x as i64));` | `flash_println_str_int("x=", x as i64);` | W2 |
| `_zag_print(s)` | `flash_print(s)` | W2 |
| `_zag_println(s)` | `flash_println(s)` | W2 |
| `let b: t9 = 0 - 9841;` | `let b: t9 = -9841;` | W3 |
| `group { ... }` | `{ ... }` | W3 |
| `push[i32](&xs, 9)` | `push(&xs, 9)` (inferred) | W9 |
| `Counter_doubled(c)` | `c.doubled()` | W9 |
| `raw { fire_shield(0,0,0,2.5); }` | `fire_shield(voxel(0,0,0), mw(2.5));` | W6 |
| `raw { _zag_fire_shield(...); }` | `fire_shield(FlashVoxel, FlashPower)` | W6 |
| `let trit_add(a,b) = sum` | `fn trit_add(a:trit,b:trit) trit { ... }` or keep sugar | W1 |
| 64-line hand-unrolled trit pipeline | `tritvec[64]` + `for` + `@` | W5 |
| `let N = 9; let arr = make[i32](N);` | `const N: i32 = 9; let arr = make[i32](N);` | W11 |
| (no effect annotation) | `@photonic fn fire(...) { ... }` | W8 |

---

## 3. Step-by-step migration

### 3.1 Replace _zag_* calls with Flash-native stdlib

**Before:**
```flash
_zag_print("result: ");
_zag_println(_zag_i64_to_str(x as i64));
```

**After:**
```flash
@import("std/flash_std.flash")
// ...
flash_println_str_int("result: ", x as i64);
```

Add `@import("std/flash_std.flash")` at the top of the file. Replace all
`_zag_*` calls with their `flash_*` equivalents.

### 3.2 Replace raw { } blocks with typed photonic API

**Before:**
```flash
raw {
    fire_shield(0, 0, 0, 2.5);
    set_phase(0, 0, 0, 0.785);
}
```

**After:**
```flash
@import("std/photonic.flash")

@photonic
fn main() i32 {
    let v: FlashVoxel = voxel(0, 0, 0);
    let p: FlashPower = mw(2.5);
    fire_shield(v, p);

    let ph: FlashPhase = rad(0.785);
    set_phase(v, ph);
    return 0;
}
```

Note the `@photonic` annotation — it's required for any function that
performs photonic operations (W8).

### 3.3 Use tritvec and @ instead of hand-unrolled pipelines

**Before:**
```flash
let a0: trit = +1;
let b0: trit = -1;
let r0: trit = trit_add(a0, b0);
// ... repeat 64 times
```

**After:**
```flash
let a: tritvec[64] = 0t+++0--+...;
let b: tritvec[64] = 0t-0+0+...;
let r: tritvec[64] = a @ b;
```

### 3.4 Use method syntax and type inference

**Before:**
```flash
push[i32](&xs, 9);
let d: i32 = Counter_doubled(c);
```

**After:**
```flash
push(&xs, 9);           // type inferred
let d: i32 = c.doubled();  // method syntax
```

### 3.5 Use const for compile-time constants

**Before:**
```flash
fn process(arr: ArrayList[i32]) void {
    let N: i32 = 64;
    // N must be redefined in every function
}
```

**After:**
```flash
const N: i32 = 64;

fn process(arr: ArrayList[i32]) void {
    // N is available everywhere, evaluated at compile time
    let buf: ArrayList[i32] = make[i32](N);
}
```

### 3.6 Use PrintBuf for multi-argument printing

**Before:**
```flash
_zag_print("x=");
_zag_print(_zag_i64_to_str(x));
_zag_print(" y=");
_zag_println(_zag_i64_to_str(y));
```

**After (overload):**
```flash
flash_println_4sisi("x=", x, " y=", y);
```

**After (PrintBuf, for arbitrary counts):**
```flash
let pb: PrintBuf = pb_new();
pb_str(&pb, "x=");
pb_int(&pb, x);
pb_str(&pb, " y=");
pb_int(&pb, y);
pb_println(pb);
```

---

## 4. Effect annotations

Any function that performs photonic operations must be annotated
`@photonic`:

```flash
@photonic
fn fire_shield(v: FlashVoxel, p: FlashPower) void {
    // photonic operations here
}
```

If a function calls a `@photonic` function, it must also be `@photonic`.
The compiler will report F0003 if this is violated.

---

## 5. What stays the same

These Flash 1.0 constructs are unchanged in 2.0:

- `fn name(params) ReturnType { }` function syntax
- `let name: Type = expr;` bindings
- `struct`, `enum`, `union`, `switch` with `|capture|`
- `if`, `while`, `for`, `break`, `continue`
- `try`, `catch`, `orelse` error handling
- `?T` optionals, `!T` error unions
- Generic functions with `[T]` syntax
- `@import("path")` module imports

## 5a. New reserved keywords

Flash 2.0 adds `const` as a reserved keyword. If your 1.0 code uses "const"
as a variable or function name, rename it before migrating.

---

## 6. Automated migration

A `flashc migrate` command is planned to perform mechanical substitutions
automatically. It will produce a diff for review. Until then, migration
is manual following the steps above.

---

## 7. Testing your migration

After migrating, verify with:

```bash
flashc your_file.flash --check    # semantic check
flashc your_file.flash -o out --run  # compile and run
```

The test suite in `tests/` provides examples of all 2.0 constructs.
