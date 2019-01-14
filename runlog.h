#ifndef AC_RUNLOG_H
#define AC_RUNLOG_H

#include <stdint.h>

#define RL_GB_IN_BYTE 1073741824.0

#define RL_SIMD_SSE     0x1
#define RL_SIMD_SSE2    0x2
#define RL_SIMD_SSE3    0x4
#define RL_SIMD_SSSE3   0x8
#define RL_SIMD_SSE4_1  0x10
#define RL_SIMD_SSE4_2  0x20
#define RL_SIMD_AVX     0x40
#define RL_SIMD_AVX2    0x80
#define RL_SIMD_AVX512F 0x100

#ifdef __cplusplus
extern "C" {
#endif

int64_t rl_mem_total(void);
int64_t rl_mem_avail(void);
int32_t rl_ncpu(void);
double rl_rtime(void);
double rl_utime(void);
double rl_stime(void);
int rl_simd(void);

#ifdef __cplusplus
}
#endif

#endif
