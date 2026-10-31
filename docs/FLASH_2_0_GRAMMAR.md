# Flash 2.0 — Unified Grammar (EBNF)

**Status:** Active. This is the contract for the unified parser.
**Scope:** Full language grammar. Replaces the two-dialect split (surface vs full).

---

## 1. Lexical structure

```
comment       = "//" { non-newline } | "/*" { any } "*/"
ident         = letter { letter | digit | "_" }
type_ident    = ( "i" | "u" | "t" | "f" | "p" ) digit-string  -- i8, u16, t9, f32, p32...
              | ident                                        -- user types

keyword       = "fn" | "let" | "const" | "struct" | "enum" | "union"
              | "if" | "else" | "while" | "for" | "switch" | "break" | "continue"
              | "return" | "defer" | "try" | "catch" | "orelse" | "as"
              | "pub" | "extern" | "error" | "program" | "raw"
              | "true" | "false" | "null" | "in"

int_lit       = digit-string | "0x" hex-digit-string
float_lit     = digit-string "." digit-string
trit_lit      = "T" | "t"                                    -- single trit -1 (legacy)
tritvec_lit   = "0t" trit-digit-string                       -- balanced-ternary digits
                trit-digit = "+" | "0" | "-" | "T" | "t"
str_lit       = '"' { any } '"'
char_lit      = "'" any "'"

annot         = "@" ident                                    -- @photonic, @realtime, @import
```

## 2. Top-level grammar

```
file          = [ "program" ident ] { top-decl } ;

top-decl      = annot* ( fn-decl | struct-decl | enum-decl | union-decl
              | error-set-decl | import-decl | top-let | const-decl
              | surface-trit | surface-wave | surface-p32
              | surface-sat-i16 | optical-decl | raw-block
              | "pub" fn-decl | "extern" fn-decl ) ;

import-decl   = "@import" "(" str_lit ")" [ "as" ident ] ;

const-decl    = "const" ident ":" type "=" expr ";" ;           -- W11

fn-decl       = "fn" ident [ generic-params ] "(" param-list ")"
                [ type ] [ "!" type ] block ;

param-list    = [ param { "," param } ] ;
param         = ident ":" type ;

struct-decl   = "struct" ident [ generic-params ] "{" field-list "}" ;
field-list    = { field "," } ;
field         = ident ":" type ;

enum-decl     = "enum" ident "{" ident-list "}" ;
union-decl    = "union" ident [ generic-params ] "{" field-list "}" ;
error-set-decl= "error" ident "{" ident-list "}" ;

-- Surface DSL constructs (now legal at top level and inside fn bodies)
surface-trit  = "trit" ident "=" trit-expr ;
surface-wave  = "wave" ident "=" wave-expr ;
surface-p32   = "p32" ident "=" expr ;
surface-sat-i16= "sat_i16" ident "=" expr ;
optical-decl  = "optical" ident "(" param-list ")" "=" ident ;
raw-block     = "raw" "{" { raw-call ";" } "}" ;
raw-call      = ident "(" arg-list ")" ;

-- Equation-style function sugar: let f(a, b) = out
-- desugars to: fn f(a: trit, b: trit) trit { return <primitive>f(a, b); }
top-let       = "let" ident "(" ident-list ")" "=" ident     -- equation fn
              | "let" ident ":" type "=" expr ";"            -- top-level binding
              | "let" ident "=" expr ";" ;
```

## 3. Statements

```
block         = "{" { stmt } "}" ;

stmt          = "let" ident [ ":" type ] [ "=" expr ] ";"
              | "return" [ expr ] ";"
              | "if" "(" expr ")" [ "|" ident "|" ] block [ "else" ( if-stmt | block ) ]
              | "while" "(" expr ")" block
              | "switch" "(" expr ")" "{" switch-arms "}"
              | "break" ";" | "continue" ";"
              | "defer" stmt
              | block                                        -- bare scoped block
              | "raw" "{" { raw-call ";" } "}"               -- photonic escape
              | surface-trit | surface-wave                  -- surface in fn body
              | lvalue "=" expr ";"
              | expr ";" ;

switch-arms   = { arm } [ "else" "=>" block ] ;
arm           = ( tag { "," tag } | "else" ) "=>" block ;
tag           = ident | int-lit ;

lvalue        = expr ( "." ident | "[" expr "]" ) ;
```

## 4. Expressions (precedence low → high)

```
ident-list    = ident { "," ident } ;

expr          = or-expr ( "orelse" bin-expr | "catch" [ "|" ident "|" ] bin-expr ) ;
or-expr       = and-expr { "||" and-expr } ;
and-expr      = eq-expr { "&&" eq-expr } ;
eq-expr       = cmp-expr { ( "==" | "!=" ) cmp-expr } ;
cmp-expr      = add-expr { ( "<" | ">" | "<=" | ">=" ) add-expr } ;
add-expr      = mul-expr { ( "+" | "-" ) mul-expr } ;
mul-expr      = interfere-expr { ( "*" | "/" | "%" ) interfere-expr } ;
interfere-expr= unary-expr { "@" unary-expr } ;               -- trit interference
unary-expr    = ( "-" | "!" | "~" ) unary-expr | try-expr ;   -- ~ is trit-wise negation
try-expr      = "try" unary-expr | postfix-expr ;
postfix-expr  = primary-expr { postfix } ;
postfix       = "." ident | "(" arg-list ")" | "[" expr "]"
              | "as" type | ".(" "(" arg-list ")" ;           -- method call (W9)
primary-expr  = int-lit | float-lit | tritvec-lit | str-lit | char-lit
              | "true" | "false" | "null" | "error"
              | ident | type-ident | "(" expr ")"
              | struct-lit | fn-expr | call-expr ;
struct-lit    = type-ident "{" { "." ident "=" expr "," } "}" ;
fn-expr       = "fn" "(" param-list ")" [ type ] block ;
call-expr     = ident [ "[" type-list "]" ] "(" arg-list ")" ;
arg-list      = [ expr { "," expr } ] ;
```

Note: Bitwise operators (`|`, `^`, `&`) and shift operators (`<<`, `>>`) are
planned for a future workstream but not yet implemented in the parsers. They
will be inserted between `&&` and `==` in the precedence chain when added.

## 5. Types

```
type          = type-ident [ "[" int-lit "]" ]                -- array / generic
              | "*" type                                      -- pointer
              | "?" type                                      -- optional
              | "!" type                                      -- error-union
              | "fn" "(" type-list ")" [ type ] ;             -- function pointer
type-list     = [ type { "," type } ] ;
```

## 6. Annotations

```
annot         = "@photonic"     -- marks a fn as requiring photonic hardware (W8)
              | "@realtime"     -- marks a struct/fn as realtime-constrained
              | "@total"        -- marks a fn as total (no side effects)
              | "@io"           -- marks a fn as performing I/O (W8)
              | "@unsafe"       -- marks a fn as unsafe (allows raw blocks)
              | "@import"       -- module import
              | "@cabi"         -- C ABI interop
              | "@cabi_export"  -- C ABI export
```

## 6a. Photonic types (W6, W7)

```
photonic-type = "FlashVoxel" | "FlashBeam" | "FlashPhase"
              | "FlashPower" | "FlashPolarity"
              | "Mat3" | "Wave" | "TritTriple" | "Vec3"

unit-lit      = unit-fn "(" float-lit ")"          -- typed unit literal
unit-fn       = "mw" | "rad" | "phase" | "pol"     -- unit constructors (W6)
```

Unit literals are functions that return typed struct values:
- `mw(2.5)` returns `FlashPower`
- `rad(0.785)` returns `FlashPhase`
- `phase(0.5)` returns `FlashPhase`
- `pol(1)` returns `FlashPolarity`

## 6b. Effects system (W8)

Functions that perform photonic operations must be annotated `@photonic`.
The effect system tracks these capabilities:
- `@photonic` — photonic hardware access
- `@realtime` — realtime constraints
- `@io` — I/O side effects
- `@total` — no side effects (pure)

A function's body effects are checked against its declared annotations.
If a function body contains photonic effects, the function must declare
`@photonic`. Effects propagate transitively through calls.

## 6c. Compile-time constants (W11)

```
const-decl    = "const" ident ":" type "=" expr ";" ;
```

`const` bindings are evaluated at compile time and are usable:
- As values in expressions (e.g., `let x = WIDTH + 1;`)
- As array sizes (e.g., `make[i32](SIZE)`)
- As generic type arguments

No `comptime` blocks, no compile-time functions, no reflection.
This is constant folding with type-level use, not full comptime.

## 6d. Type inference and method syntax (W9)

```
method-call   = expr "." ident "(" arg-list ")" ;  -- desugars to Type_method(expr, args)
```

Method calls `c.doubled()` are resolved by looking up `Type_method` in the
function table, where `Type` is the type of `c`.

Generic functions called without explicit type args have their type args
inferred from the argument types:
```
first_or(xs, -1)   -- infers T = i32 from xs: ArrayList[i32]
```

## 6e. Variadic-like printing (W10)

Since Flash 2.0 does not yet have `...` variadic syntax, the stdlib provides:
- Fixed-arity overloads: `flash_println_str_int`, `flash_println_2ss`,
  `flash_println_3sis`, `flash_println_4sisi`, `flash_println_5sisiv`, etc.
- `PrintBuf` builder pattern for arbitrary arg counts:
  ```
  let pb: PrintBuf = pb_new();
  pb_str(&pb, "x=");
  pb_int(&pb, 42);
  pb_println(pb);
  ```

## 7. What changed from 1.0

1. **`program` is optional.** A file may start with `program name` as a module
   header. Without it, the file is a library module. Either way, the same
   parser handles it.

2. **Surface constructs are legal everywhere.** `trit a = +1`, `wave carrier =
   default`, `raw { }` are legal at top level AND inside `fn` bodies.

3. **`let f(a, b) = out` is sugar for `fn`.** The equation form desugars to a
   function declaration with a primitive body.

4. **Bare `{ }` blocks.** Scoped blocks without `group` keyword (W3).

5. **`0t` tritvec literals.** `0t+++0--+0+` is a balanced-ternary digit literal
   (W4).

6. **`@` interference operator.** Binary `@` between trit expressions computes
   trit-wise consensus (W5). Precedence is between `*` and unary.

7. **Unary minus on literals.** `let b: t9 = -9841;` (W3).

8. **Typed photonic API (W6).** `mw(2.5)`, `rad(0.785)`, `voxel(0,0,0)`,
   `fire_shield(voxel, power)` replace untyped `raw { }` blocks with
   type-safe Flash-native functions returning `FlashVoxel`, `FlashPower`,
   etc.

9. **Typed mat3/wave/vec3 (W7).** `Mat3`, `Wave`, `TritTriple`, `Vec3`
   types are available as Flash-native stdlib modules.

10. **Photonic effect system (W8).** Functions performing photonic
    operations must be annotated `@photonic`. The effect system enforces
    this at compile time with transitive propagation.

11. **Type inference + method syntax (W9).** `c.doubled()` method calls
    and generic type inference (`push(&xs, 9)` infers `T = i32`).

12. **Variadic-like printing (W10).** Extended print overloads and
    `PrintBuf` builder pattern for arbitrary argument counts.

13. **`const` declarations (W11).** Compile-time constant bindings
    usable as array sizes, type parameters, and expression values.
