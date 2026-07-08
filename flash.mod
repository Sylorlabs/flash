name    = "flash"
version = "1.0.0"
edition = "2026"

# Flash is standalone: compiler backend ships as flash/toolchain/flashz.
# No sibling zag repo required.

[target]
default = "x86_64-emul"
# x86_64-emul — Zag/flashz binary host (default)
# fimg        — manifest + HAL only (.fimg sidecar)
# ppu32       — balanced-ternary photonic processor ISA
# engine      — unified light-based SoC firmware ABI

[runtime]
default = "sim"
# sim  — photonic_rt.zag + chamber_sim (reference)
# hw   — RayOS HAL (flash/rayos/flash_rt.h)
# qemu — ppu32 emulator (bring-up)