# Flash 2.0 — Language Specification

**Status:** Normative. This document defines the Flash 2.0 language.
**Companion:** [`FLASH_2_0_GRAMMAR.md`](FLASH_2_0_GRAMMAR.md) for the full EBNF.

---

## 1. Program model

A Flash program is a set of modules, each in a `.flash` file. A module
consists of top-level declarations: functions, structs, enums, unions,
error sets, imports, and constants. An optional `program name` header
declares the module name.

Every program has a `fn main() i32` entry point. The return value is the
process exit code.

### Compilation

Flash compiles to Zag (`.zag`) source, which is then compiled to native
code by the Zag compiler (`znc`). The self-hosted Flash compiler
(`flashc1`) is itself written in Flash and bootstraps through this path.

---

## 2. Types

### 2.1 Primitive types

| Type | Description |
|------|-------------|
| `i8`, `i16`, `i32`, `i64` | Signed integers |
| `u8`, `u16`, `u32`, `u64` | Unsigned integers |
| `f32`, `f64` | Floating-point |
| `bool` | Boolean (`true` / `false`) |
| `[]u8` | Byte string / string slice |
| `void` | Unit type (function returns nothing) |

### 2.2 Balanced-ternary types (t-family)

| Type | Description |
|------|-------------|
| `trit` | Single balanced-ternary digit (-1, 0, +1) |
| `t3`, `t9`, `t27`, ... | Fixed-width ternary words (3^n values) |
| `tritvec[N]` | Vector of N trits (W5) |

Ternary literals: `0t+++0--+0+` (balanced-ternary digit string, W4).
The `@` operator computes trit-wise interference (W5).

### 2.3 Photonic types (W6, W7)

| Type | Description |
|------|-------------|
| `FlashVoxel` | 3D coordinate (x, y, z) |
| `FlashBeam` | Photonic beam descriptor |
| `FlashPhase` | Phase angle in radians |
| `FlashPower` | Power in milliwatts |
| `FlashPolarity` | Beam polarity |
| `Mat3` | 3x3 ternary matrix |
| `Wave` | Wave function descriptor |
| `TritTriple` | Triple of trits |
| `Vec3` | 3D vector |

Unit literals: `mw(2.5)` → `FlashPower`, `rad(0.785)` → `FlashPhase`.

### 2.4 Composite types

```
ArrayList[T]    -- growable list of T
*T              -- pointer to T
?T              -- optional T (null or value)
!T              -- error-union (error or T)
fn(A, B) R      -- function pointer
```

### 2.5 User-defined types

- `struct Name { field: Type, ... }` — record
- `enum Name { Variant, ... }` — enumeration
- `union Name { field: Type, ... }` — tagged union
- `error Name { Code, ... }` — error set

---

## 3. Bindings and control flow

### 3.1 Bindings

```
let name: Type = expr;       -- typed binding
let name = expr;             -- inferred binding
const NAME: Type = expr;     -- compile-time constant (W11)
```

`const` bindings are evaluated at compile time and persist across all
scopes. They can be used as array sizes, type parameters, and expression
values.

### 3.2 Control flow

```
if (cond) { ... } else { ... }
while (cond) { ... }
switch (val) { Tag => { ... }, else => { ... } }
for item in collection { ... }       -- collection iteration
break;
continue;
defer stmt;                          -- deferred execution
```

Bare `{ }` blocks create a new scope (W3).

---

## 4. Functions, methods, and generics

### 4.1 Function declarations

```
fn name[T, U](param: Type, ...) ReturnType { body }
```

Generic type parameters are listed in `[...]`. Parameters are typed.
The return type is optional (defaults to `void`).

### 4.2 Method syntax (W9)

```
c.doubled()    -- desugars to Counter_doubled(c)
```

Method calls are resolved by looking up `Type_method` in the function
table, where `Type` is the receiver's type.

### 4.3 Type inference (W9)

Generic functions called without explicit type args have their type args
inferred from the argument types:

```
fn first_or[T](l: ArrayList[T], fallback: T) T { ... }
first_or(xs, -1)    -- infers T = i32 from xs: ArrayList[i32]
```

Inference prefers concrete types over `int_literal`. If the mono table
has a registered instantiation, it is used; otherwise the return type is
computed by substituting type params in the declared return type.

### 4.4 Equation-style sugar

```
let f(a, b) = out    -- desugars to fn f(a: trit, b: trit) trit { ... }
```

---

## 5. Optionals and errors

### 5.1 Optionals

```
?T    -- either null or a value of T
```

Use `orelse` to provide a default: `val orelse fallback`.

### 5.2 Error unions

```
!T    -- either an error code or a value of T
```

Use `try` to propagate errors: `try risky_call()`.
Use `catch` to handle: `risky_call() catch |e| { ... }`.

Error sets are declared with `error Name { Code1, Code2 }`.

---

## 6. Photonic API (W6, W7)

### 6.1 Typed photonic functions

The photonic API is accessed through Flash-native functions in
`std/photonic.flash`:

```
voxel(x, y, z)           -> FlashVoxel
fire_shield(v, power)    -> void    (requires @photonic)
mw(value)                -> FlashPower
rad(value)               -> FlashPhase
```

These replace the untyped `raw { }` blocks from Flash 1.0.

### 6.2 Mat3 / Wave / Vec3 (W7)

The `std/mat3.flash` module provides typed matrix, wave, and vector
operations:

```
mat3_identity()          -> Mat3
mat3_rotation_xy_90()    -> Mat3
mat3_mul(a, b)           -> Mat3
mat3_apply(m, t)         -> TritTriple
flash_wave_default()     -> Wave
```

---

## 7. Effects system (W8)

### 7.1 Effect annotations

| Annotation | Meaning |
|------------|---------|
| `@photonic` | Function accesses photonic hardware |
| `@realtime` | Function has realtime constraints |
| `@io` | Function performs I/O |
| `@total` | Function is pure (no side effects) |
| `@unsafe` | Function uses unsafe operations |

### 7.2 Effect checking

The semantic analyzer tracks effects through function bodies:
- If a function's body contains photonic effects (directly or through
  transitive calls), the function **must** declare `@photonic`.
- Effects propagate through function calls.
- The effect bits are OR'd together across all sub-expressions.

A function that calls a `@photonic` function without itself being
`@photonic` is a compile error (F0003).

---

## 8. Compile-time (W11)

### 8.1 Const declarations

```
const WIDTH: i32 = 9;
const MAX: t9 = t+++++++++;
```

`const` bindings are:
- Evaluated at compile time (must be literal or const-expression).
- Usable as `tritvec[N]`, array sizes, and generic type args.
- Persistent across all scopes (unlike `let` which is scope-local).

### 8.2 What is NOT included

- No `comptime` blocks.
- No compile-time functions.
- No reflection.
- No macros.

This is **constant folding with type-level use**, not Zig's full comptime.

---

## 9. Standard library

### 9.1 std/flash_std.flash

Core I/O and printing:
- `flash_print(s)`, `flash_println(s)` — string printing
- `flash_print_int(v)`, `flash_println_int(v)` — integer printing
- `flash_println_str_int(s, v)` — combined string + int
- `flash_newline()` — print newline
- Extended overloads (W10): `flash_println_2ss`, `flash_println_3sis`,
  `flash_println_4sisi`, `flash_println_5sisiv`
- `PrintBuf` builder: `pb_new()`, `pb_str()`, `pb_int()`, `pb_println()`

### 9.2 std/photonic.flash

Typed photonic API (W6): `FlashVoxel`, `FlashBeam`, `FlashPower`,
`FlashPhase`, `FlashPolarity` types and their operations.

### 9.3 std/mat3.flash

Typed matrix/wave/vector operations (W7): `Mat3`, `Wave`, `TritTriple`,
`Vec3` types and their operations.

### 9.4 std/fmt.flash

Formatting helpers: `flash_fmt_int(v)` — format integer as `[]u8`.

---

## 10. Explicitly-unsupported list

The following are **not** part of Flash 2.0:

- `comptime` blocks or compile-time code execution
- Macros or metaprogramming
- Reflection or runtime type information
- Variadic `...` syntax (W10 uses overloads + PrintBuf instead)
- Bitwise operators (`|`, `^`, `&`, `<<`, `>>`) — planned for future
- Async/await
- Classes or inheritance (Flash uses structs + methods)
- Garbage collection (manual memory management via `_zag_str_free`)

---

## 11. Conformance requirements

A conforming Flash 2.0 implementation must:

1. **Parse** the full grammar in `FLASH_2_0_GRAMMAR.md`.
2. **Type-check** all constructs including generics, optionals, and errors.
3. **Enforce effects**: `@photonic` required for photonic operations.
4. **Support const** declarations with type-level use.
5. **Resolve method calls** via `Type_method` mangling.
6. **Infer generic type args** from argument types.
7. **Compile** to Zag source that `znc` can build to native code.
8. **Provide** the stdlib modules: `flash_std`, `photonic`, `mat3`, `fmt`.
