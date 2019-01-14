#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <signal.h>
#include <time.h>
#include "runlog.h"
#include "ketopt.h"

typedef struct {
	double rtime, utime, stime;
	int64_t peak_mem;
	int ret, sig;
	long inblock, outblock, minflt, majflt;
} rl_res_t;

typedef void (*rl_sighandler_t)(int);

int launch_cmd(int argc, char *argv[], rl_res_t *res)
{
	pid_t pid;
	res->rtime = rl_rtime();
	pid = fork();
	if (pid < 0) { // failed to fork
		perror(NULL);
		return -1;
	}
	if (pid != 0) { // parent process
		int status;
		struct rusage r;
		rl_sighandler_t sigint_func, sigquit_func;
		pid_t ret;

		sigint_func  = signal(SIGINT,  SIG_IGN); // acquire the default signal handler and ignore in the parent process
		sigquit_func = signal(SIGQUIT, SIG_IGN);
		while ((ret = wait3(&status, 0, &r)) != pid) {
			if (ret == -1) {
				perror(NULL);
				return -1;
			}
		}
		signal(SIGINT,  sigint_func);
		signal(SIGQUIT, sigquit_func);
		res->inblock  = r.ru_inblock;
		res->outblock = r.ru_oublock;
		res->minflt = r.ru_minflt;
		res->majflt = r.ru_majflt;
		res->rtime = rl_rtime() - res->rtime;
		res->utime = r.ru_utime.tv_sec + 1e-6 * r.ru_utime.tv_usec;
		res->stime = r.ru_stime.tv_sec + 1e-6 * r.ru_stime.tv_usec;
#ifdef __linux__
		res->peak_mem = r.ru_maxrss * 1024;
#else
		res->peak_mem = r.ru_maxrss;
#endif
		if (WIFEXITED(status)) res->ret = WEXITSTATUS(status), res->sig = 0;
		else if (WIFSIGNALED(status)) res->ret = -1, res->sig = WTERMSIG(status);
		else res->ret = -1, res->sig = 0;
	} else { // child process
		execvp(argv[0], argv);
		perror(NULL); // launch failed
	}
	return 0;
}

static void write_simd_str(FILE *fp, int flag, const char *str, int *is_first)
{
	if (flag) {
		if (!*is_first) fputc(' ', fp);
		*is_first = 0;
		fputs(str, fp);
	}
}

int main(int argc, char *argv[])
{
	ketopt_t o = KETOPT_INIT;
	int i, c, simd_flags, has_uname, print_sys = 0;
	int64_t avail_mem_st;
	char host_name[256], *fn_log = 0;
	struct utsname un;
	rl_res_t res;
	time_t cur_time;
	FILE *fp = stderr;

	while ((c = ketopt(&o, argc, argv, 0, "so:", NULL)) >= 0) {
		if (c == 's') print_sys = 1, fp = stdout;
		else if (c == 'o') fn_log = o.arg;
	}

	if (argc == o.ind && !print_sys) {
		fprintf(stderr, "Usage: runlog [options] <command> [arguments]\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "  -s         print system information to stdout\n");
		fprintf(stderr, "  -o FILE    save logs to FILE [stderr]\n");
		return 1;
	}

	if (fn_log) {
		fp = fopen(fn_log, "w");
		if (fp == 0) {
			perror("Failed to write");
			return 1;
		}
	}

	if (uname(&un) == 0) has_uname = 1;
	else has_uname = 0;

	// CPU information
	if (has_uname) fprintf(fp, "runlog_cpu_type\t%s\n", un.machine);
	fprintf(fp, "runlog_n_cpus\t%d\n", rl_ncpu());
	simd_flags = rl_simd();
	if (simd_flags >= 0) {
		int is_first = 1;
		fprintf(fp, "runlog_cpu_simd\t");
		write_simd_str(fp, simd_flags & RL_SIMD_SSE,     "sse",     &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_SSE2,    "sse2",    &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_SSE3,    "sse3",    &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_SSSE3,   "ssse3",   &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_SSE4_1,  "sse4.1",  &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_SSE4_2,  "sse4.2",  &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_AVX,     "avx",     &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_AVX2,    "avx2",    &is_first);
		write_simd_str(fp, simd_flags & RL_SIMD_AVX512F, "avx512f", &is_first);
		fputc('\n', fp);
	}
	fprintf(fp, "runlog_total_ram_in_gb\t%.6f\n", rl_mem_total() / RL_GB_IN_BYTE);

	if (has_uname) fprintf(fp, "runlog_os\t%s %s\n", un.sysname, un.release);
	if (gethostname(host_name, 255) == 0)
		fprintf(fp, "runlog_hostname\t%s\n", host_name);
	avail_mem_st = rl_mem_avail();
	if (avail_mem_st >= 0) // if not, rl_mem_avail() is not implemented
		fprintf(fp, "runlog_avail_ram_in_gb\t%.6f\n", avail_mem_st / RL_GB_IN_BYTE);

	if (print_sys) return 0;

	fprintf(fp, "runlog_command_line\t");
	for (i = o.ind; i < argc; ++i) {
		if (i != o.ind) fputc(' ', fp);
		fprintf(fp, "%s", argv[i]);
	}
	fputc('\n', fp);
	time(&cur_time);
	fprintf(fp, "runlog_time_start ===>\t%s", ctime(&cur_time));

	launch_cmd(argc - o.ind, argv + o.ind, &res);

	time(&cur_time);
	fprintf(fp, "runlog_time_end <===\t%s", ctime(&cur_time));
	fprintf(fp, "runlog_term_signal\t%d\n", res.sig);
	fprintf(fp, "runlog_return_value\t%d\n", res.ret);
	fprintf(fp, "runlog_real_time_in_sec\t%.3f\n", res.rtime);
	fprintf(fp, "runlog_user_time_in_sec\t%.3f\n", res.utime);
	fprintf(fp, "runlog_sys_time_in_sec\t%.3f\n", res.stime);
	fprintf(fp, "runlog_percent_cpu\t%.2f\n", 100.0 * (res.utime+res.stime+1e-6) / (res.rtime+1e-6));
	fprintf(fp, "runlog_peak_rss_in_mb\t%.6f\n", res.peak_mem / 1024.0 / 1024.0);
	fprintf(fp, "runlog_hard_page_faults\t%ld\n", res.majflt);
	fprintf(fp, "runlog_soft_page_faults\t%ld\n", res.minflt);
	fprintf(fp, "runlog_disk_inputs\t%ld\n", res.inblock);
	fprintf(fp, "runlog_disk_outputs\t%ld\n", res.outblock);

	if (fp != stdout || fp != stderr) fclose(fp);
	return res.sig > 0? 2 : res.ret;
}
