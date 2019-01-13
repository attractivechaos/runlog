#ifndef AC_RUNLOG_H
#define AC_RUNLOG_H

#include <stdint.h>

#define RL_GB_IN_BYTE 1073741824.0

#ifdef __cplusplus
extern "C" {
#endif

int64_t rl_mem_total(void);
int64_t rl_mem_avail(void);
int32_t rl_ncpu(void);
double rl_rtime(void);

#ifdef __cplusplus
}
#endif

#endif
