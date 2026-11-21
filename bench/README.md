# Flash Grand Prix — benchmark suite

Matched kernels: `.ppu32` (Flash → PPU32 device steps, modeled by qemu_ppu32)
vs `.c` (binary host, `-O2`, dynamic instructions measured by `perf`).

Fairness: see docs/HOW_WE_MEASURED.md. Flash numbers are modeled device steps
at the declared cadence — emulation wall-time is never reported. C numbers are
dynamic instruction counts converted to device steps at the same cadence.
