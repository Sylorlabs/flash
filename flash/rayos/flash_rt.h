/* flash/rayos/flash_rt.h — RayOS ↔ Flash hardware runtime contract (v1).
 *
 * Flash binaries compiled with --emit-fimg ship a .fimg sidecar or an ELF
 * section ".flash.manifest". RayOS firmware loads the manifest, programs PCM
 * cells, and registers HAL symbols before main().
 *
 * Environment (set by boot firmware):
 *   FLASH_PLATFORM=engine|conventional|emulation
 *   FLASH_RT=sim|hw|qemu
 *   FLASH_GPU=0|1
 */

#ifndef FLASH_RT_H
#define FLASH_RT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FlashCoord {
    float x, y, z;
} FlashCoord;

typedef struct FlashPathSegment {
    FlashCoord from;
    FlashCoord to;
    float laser_power_mw;
    int32_t polarization;
    const char *op_tag;
    int32_t pcm_row;
    int32_t pcm_col;
    float phase_a_rad;
    float phase_b_rad;
    float phase_result_rad;
    int32_t lattice_op;
} FlashPathSegment;

typedef struct FlashManifest {
    const char *name;
    const char *version;
    uint32_t segment_count;
    const FlashPathSegment *segments;
    float peak_power_mw;
    int32_t dominant_pol;
} FlashManifest;

typedef struct FlashEngineBoot {
    const char *platform;
    int32_t gpu;
    int32_t compute_route;
    const char *boot_abi;
} FlashEngineBoot;

/* Manifest loader — parse FIMG v1 or mmap ELF .flash.manifest */
int flash_manifest_load(const char *path, FlashManifest *out);
void flash_manifest_unload(FlashManifest *out);

/* Engine boot loader — parse engine-v1 image ([engine] + nested FIMG) */
int flash_engine_load(const char *path, FlashManifest *out, FlashEngineBoot *boot);

/* PCM programmer — apply manifest segments to Active Lattice */
int flash_pcm_program(const FlashManifest *m);

/* HAL registration — silicon implements these; sim uses photonic_rt */
int flash_hal_register_sim(void);
int flash_hal_register_hw(void);

/* Raw chamber ABI (stable across sim/hw) */
int _flash_raw_mode_enter(void);
int _flash_raw_mode_exit(void);
int _flash_raw_fire_shield(int x, int y, int z, float mw);
int _flash_raw_inject_phase(int x, int y, int z, float phase);
int _flash_raw_set_laser_mw(int wall, int slot, float mw);
int _flash_raw_target_voxel(int x, int y, int z);
int _flash_raw_set_polarity(int x, int y, int z, int pol);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_RT_H */