#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "runlog.h"

int32_t rl_page_size(void)
{
	return (int32_t)sysconf(_SC_PAGESIZE);
}

int64_t rl_mem_total(void)
{
	return (int64_t)sysconf(_SC_PHYS_PAGES) * rl_page_size();
}

int32_t rl_ncpu(void)
{
	return (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
}

double rl_rtime(void)
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec + tp.tv_usec * 1e-6;
}

double rl_stime(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_stime.tv_sec + 1e-6 * r.ru_stime.tv_usec;
}

double rl_utime(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_utime.tv_sec + 1e-6 * r.ru_utime.tv_usec;
}

int64_t rl_mem_peak(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
#ifdef __linux__
	return r.ru_maxrss * 1024;
#else
	return r.ru_maxrss;
#endif
}

#if defined(__APPLE__)
#include <mach/mach.h>

/* In Activity Monitor on Mac, memory is *probably* classified in the following way:
 *
 * - Wired: kernel memory, given by .wire_count
 * - App memory: given by (.internal_page_count - .purgeable_count)
 * - Memory used by compressor: computed by .compressor_page_count (on recent OSX only)
 * - Used memory = wired + app + compressor
 * - Free memory: computed by (.free_count - .speculative_count). The "top" command counts
 *   speculative memory towards free, but vm_stat and Activity Monitor doesn't.
 * - File cache: computed by (.external_page_count + .purgeable_count). Speculative pages
 *   are *probably* part of external pages.
 *
 * Total memory = .internal_page_count + .external_page_count + .wire_count + .compressor_page_count + (.free_count - .speculative_count) ?
 * .internal_page_count = .active_count + (.inactive_count - (.external_page_count - .speculative_count))
 *
 * In the following function, available memory is computed as free+cahce. References:
 *
 * - https://opensource.apple.com/source/system_cmds/system_cmds-805.220.1/vm_stat.tproj/vm_stat.c.auto.html
 * - https://opensource.apple.com/source/xnu/xnu-4903.221.2/osfmk/mach/vm_statistics.h.auto.html
 * - https://stackoverflow.com/questions/31469355/how-to-calculate-app-and-cache-memory-like-activity-monitor-in-objective-c-in-ma
 * - https://apple.stackexchange.com/questions/258166/what-is-app-memory-calculated-from
 */
int64_t rl_mem_avail(void)
{
	natural_t c = HOST_VM_INFO64_COUNT;
	struct vm_statistics64 v;
	if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&v, &c) != KERN_SUCCESS)
		return -1;
	return (int64_t)(v.external_page_count + v.purgeable_count + v.free_count - v.speculative_count) * rl_page_size();
}
#elif defined(__linux__)
#include <string.h>
#include <stdio.h>
#include <assert.h>

static int read_key(const char *s, int len, const char *key, int64_t *x)
{
	int i, off;
	off = strlen(key);
	if (strncmp(s, key, off) != 0) return 0;
	for (i = off; i < len && s[i] != 0; ++i)
		if (s[i] >= '0' && s[i] <= '9')
			break;
	assert(i < len && s[i] != 0);
	*x = atol(&s[i]);
	return 1;
}

static int64_t read_num_from_file(const char *fn)
{
	int64_t x;
	FILE *fp;
	char buf[256];
	if ((fp = fopen(fn, "r")) == NULL) return -1;
	fgets(buf, 256, fp);
	x = atol(buf);
	fclose(fp);
	return x;
}

int64_t rl_mem_avail(void)
{
	FILE *fp;
	int64_t mem_avail = -1, mem_free = -1, inact_file = -1, act_file = -1, sreclaim = -1;
	char buf[256];
	if ((fp = fopen("/proc/meminfo", "r")) == NULL) return -1;
	while (fgets(buf, 255, fp) != NULL) {
		int64_t x;
		if (read_key(buf, 256, "MemAvailable:", &x)) mem_avail = x;
		else if (read_key(buf, 256, "MemFree:", &x)) mem_free = x;
		else if (read_key(buf, 256, "Inactive(file):", &x)) inact_file = x;
		else if (read_key(buf, 256, "Active(file):", &x)) act_file = x;
		else if (read_key(buf, 256, "SReclaimable:", &x)) sreclaim = x;
	}
	fclose(fp);
	if (mem_avail < 0) { // see procps/free.c
		int64_t min_free, low;
		min_free = read_num_from_file("/proc/sys/vm/min_free_kbytes");
		low = min_free * 5 / 4;
		mem_avail = mem_free - low
			+ inact_file + act_file - ((inact_file + act_file) / 2 < low? (inact_file + act_file) / 2 : low)
			+ sreclaim - (sreclaim / 2 < low? sreclaim / 2 : low);
		if (mem_avail < 0) mem_avail = 0;
	}
	return mem_avail * 1024;
}
#else
int64_t rl_mem_avail(void) { return -1; }
#endif // ~__linux__
