# Flash operating-system readiness

Status: implementation contract, August 2026.

Flash has two distinct systems targets. They must not be conflated:

1. Hosted systems software runs as a native process over the vendored Zag
   backend and the Flash Linux syscall/runtime layer.
2. A freestanding operating-system kernel owns entry, memory, interrupts,
   scheduling, device access, and linking without a host runtime.

The first target works. The second does not yet have a compiler backend.

## Proven now

- Self-hosted compiler bootstrap and byte-identical fixpoint.
- Native x86-64 hosted executables from clean Flash source.
- Exact-width integer and balanced-ternary types, pointers, slices, structs,
  unions, enums, function values, explicit allocation, and checked collections.
- Raw Linux syscalls, files, processes, `mmap`, pipes, Unix sockets, polling,
  shared memory, and descriptor passing for hosted services and desktops.
- CPU framebuffer and vector rendering used by freshly compiled RayOS desktop
  tests.
- Photonic surface programs lower to non-empty PPU32 route instructions and run
  in the checked PPU32 interpreter.
- External packages use `FLASH_HOME` for both absolute runtime imports and the
  bundled backend, independent of their current working directory.
- Named `flash.mod` dependencies build and run through aliases even when a
  project declares multiple dependencies. Package declarations are flattened;
  duplicate top-level symbols across packages are rejected with source paths
  rather than silently shadowed.

These capabilities support compilers, compositors, emulators, services,
desktop shells, and operating-system simulation in Flash. They do not prove a
bootable Flash kernel.

## Fail-closed target boundary

`--target ppu32` is currently a photonic route backend. An ordinary full-language
program has no executable PPU32 lowering, so `flashc` rejects it instead of
writing a syntactically valid image with an empty `[insns]` section.

`--target rayos-ternary` is reserved but unavailable. It is rejected with an
explicit freestanding-backend diagnostic. Flash never silently substitutes an
`x86_64-emul` process or a generated Zag shim for that target.

`tests/os_target_contract_gate.sh` enforces both rules.

## Required before a freestanding-kernel claim

All of the following need implementation and executable tests:

- A target ABI: register roles, stack and call frame, aggregate layout,
  argument/return rules, interrupt frames, and context-switch contract.
- General full-language lowering to a real machine IR, including functions,
  branches, calls, loads/stores, statics, relocations, and startup code.
- Volatile MMIO operations with compiler ordering semantics. Ordinary pointer
  loads and stores are not a replacement.
- Atomics, memory-order operations, and interrupt-safe synchronization.
- Freestanding allocation and panic hooks with no implicit hosted runtime.
- Linker/image production with a defined entry point, sections, alignment,
  symbols, relocations, and reproducible boot artifact.
- Interrupt/exception declarations and architecture-specific save/restore.
- A boot test in an emulator or target simulator that executes Flash-produced
  kernel instructions, handles a timer interrupt, writes a framebuffer, and
  exercises an owned allocator.

Until those gates exist and pass, the accurate description is “hosted systems
language with a photonic route backend,” not “freestanding OS language.”

## RayOS and Flashy boundary

Flashy is the reusable pure-Flash application UI toolkit. RayOS should not grow
a second general UI toolkit. Its desktop should depend on Flashy through a
RayOS platform adapter that supplies:

- window/surface creation and presentation;
- pointer, keyboard, clipboard, timer, and accessibility events;
- RayOS-owned allocation and lifecycle hooks;
- compositor damage and pixel-format negotiation.

Flashy currently has headless, Linux X11, and Linux Wayland foundations. A
RayOS backend belongs in the Flashy and RayOS repositories; it is not a Flash
syntax feature. The language/runtime work in this repository should remain the
portable substrate shared by all of those backends.
