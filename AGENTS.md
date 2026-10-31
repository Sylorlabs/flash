# Flash Language — Build & Test Guide

## Build
```sh
./build.sh          # builds flashc1 (self-hosted compiler) + runs all tests
```

If `build.sh` segfaults during self-compilation (transient), re-run it. The
fallback path uses the cached shim from `selfhost/flashc1_shim.zag`.

## Test
```sh
tests/run_all.sh    # 95 tests: compiler, examples, critic, safety, fuzz
```

Expected output: `RESULT: 98 / 98 passed` / `== flash: all tests passed ==`

## Manual build (no build.sh)
```sh
rm -f /tmp/flash_rt_state.ptr
./flashc selfhost/flashc1.flash -o .test_bin/flashc1
cp .test_bin/flashc1 flashc && chmod +x flashc
```

## Bootstrap fixpoint verification
```sh
./flashc selfhost/flashc1.flash -o .test_bin/flashc2
cmp .test_bin/flashc1 .test_bin/flashc2 && echo "FIXPOINT OK"
```

## Key files
- `flash/toolchain/zag-poc/std/list.zag` — ArrayList[T] used by flashc compiler
- `selfhost/std/list.flash` — ArrayList[T] used by Flash user programs
- `selfhost/flashc1.flash` — self-hosted compiler entry point
- `selfhost/fsema.flash` — semantic analysis (full language)
- `selfhost/fsema_surface.flash` — semantic analysis (surface language)
- `selfhost/fparse.flash` — parser
- `selfhost/flex.flash` — lexer
- `selfhost/femit.flash` — emitter (includes monomorphization pass)
- `flash/toolchain/flashz` — vendored Zag native compiler (binary, not editable)

## Known issues and limitations

### Memory
- `_zag_malloc` panics on OOM (annotated `@panic`). No `_zag_malloc_try`
  variant exists in the vendored flashz binary.
- The Allocator abstraction uses **function pointers** for dispatch. The
  `Allocator` struct stores `alloc_fn`, `realloc_fn`, `free_fn` plus an opaque
  `state` pointer. Two built-in allocators are provided:
  - `default_allocator()`: wraps `_zag_malloc`/`_zag_realloc`/`_zag_free`
  - `arena_allocator(s)`: bump pointer over a pre-allocated buffer
- Arena `free` is a no-op (individual allocations are freed collectively via
  `arena_free`). Arena `realloc` falls back to `_zag_realloc` (cannot grow
  in-place within the arena buffer).
- Allocation size calculations include overflow guards: if `cap * sizeOf[T]`
  would overflow i32, a panic is emitted.
- Allocation results are null-checked: if `_zag_malloc` returns null, a
  diagnostic panic is emitted.

### Bounded polymorphism (concepts)
- The compiler performs **AST-level monomorphization** of user-defined generic
  functions. For each unique (fn_name, type_args) instantiation, a specialized
  copy of the function is created with type parameters substituted by concrete
  types. Call sites are rewritten to use mangled names (e.g.,
  `double_it[Counter]` → `double_it__Counter`).
- Type parameter substitution is **textual**: it replaces the type parameter
  name as a word token in all AST string fields (text, ty, names, names2,
  types). This works for method dispatch patterns like `T_doubled(x)` →
  `Counter_doubled(x)`.
- Concept constraint checking happens at call sites in `fsema.flash`. The
  checker verifies that the type argument has all required methods with
  matching signatures (parameter types and return types).
- **Limitation**: The monomorphization pass does NOT handle generic functions
  called from within other generic functions (nested generics). Only call
  sites in non-generic function bodies are processed.
- **Limitation**: The pass does NOT handle generic structs (only generic
  functions). Generic structs like `Vec[T]` and `ArrayList[T]` are handled
  natively by the Zag backend.

### Module collision detection
- The compiler detects duplicate function and type names across imported files
  and reports them as errors, including which files contain the conflicting
  declarations.
- **Limitation**: Collision detection is by name only. It does not check for
  signature compatibility (two functions with the same name but different
  signatures are still a collision).

### Bounds checking
- `ArrayList` `get`/`set`/`pop` are bounds-checked with diagnostic panics:
  `panic: ArrayList get OOB i=10 len=1`.
- Negative indices and out-of-bounds indices both trigger panics.

### Optional returns
- `flash_str_index_of` returns `?i32` (optional) instead of `i32` with `-1`
  sentinel. Use `if (expr) |val|` to capture the value.
- Boolean-returning functions use `bool` (not `i32` 0/1 sentinels):
  `flash_str_ends_with`, `flash_list_has`, `set_chamber_rotation`,
  `set_chamber_occupied`, `seat_nano_block`, `remove_nano_block`,
  `logical_offset_in_range`, `rotation_mismatch`.

### Safety
- Integer overflow in allocation size calculations is checked and panics.
- `flash_path_stem` has explicit length guards before subtracting suffix
  lengths.
- Allocation results are null-checked in `make`, `make_with`, `arena_new`,
  `lattice_new`, and `pool_new`. Null results trigger a diagnostic panic.
- Pointer dereferences in library code (`lib/*.flash`) on allocated arrays
  are safe because allocations are null-checked.

## Language features

### Bounded polymorphism (concepts)
```flash
concept Numeric {
    fn value(self: i32) i32;
    fn doubled(self: i32) i32;
}

fn double_it[T: Numeric](x: T) i32 {
    return T_doubled(x);
}
```
The compiler checks that type arguments satisfy concept constraints at call
sites, then monomorphizes the generic function body by substituting T with
the concrete type. See `examples/concept_demo.flash` and
`examples/concept_multi.flash` for working examples.

### Allocator abstraction
```flash
// Default allocator (function pointers wrap _zag_malloc etc.)
let a: Allocator = default_allocator();
let ys: ArrayList[i32] = make_with[i32](4, &a);

// Arena allocator (bump pointer over pre-allocated buffer)
let arena: ArenaState = arena_new(4096);
let arena_alloc: Allocator = arena_allocator(&arena);
let zs: ArrayList[i32] = make_with[i32](4, &arena_alloc);
// ... use zs ...
arena_free(&arena);  // frees all arena allocations at once
```
See `examples/allocator_demo.flash` for a working example.

### Module collision detection
```flash
@import("module_a.flash")
@import("module_b.flash")
// If module_a and module_b both define `foo`, the compiler reports:
// flashc1: module collision: duplicate function `foo` defined in
//   `module_a.flash` and `module_b.flash`
```

### Bounds checking
```flash
let xs: ArrayList[i32] = make[i32](4);
push[i32](&xs, 10);
let v: i32 = get[i32](xs, 5);  // panic: ArrayList get OOB i=5 len=1
```
