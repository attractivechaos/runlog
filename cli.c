#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include "runlog.h"
#include "ketopt.h"

typedef struct {
	double rtime, utime, stime;
	int64_t peak_mem;
	int ret, sig;
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

int main(int argc, char *argv[])
{
	ketopt_t o = KETOPT_INIT;
	int i, c;
	int64_t avail_mem_st;
	rl_res_t res;

	while ((c = ketopt(&o, argc, argv, 0, "", NULL)) >= 0) {
	}

	if (argc == o.ind) {
		fprintf(stderr, "Usage: runlog <prog> [arguments]\n");
		return 1;
	}

	avail_mem_st = rl_mem_avail();
	fprintf(stderr, "runlog_number_of_cpus\t%d\n", rl_ncpu());
	fprintf(stderr, "runlog_total_ram_in_gb\t%.6f\n", rl_mem_total() / RL_GB_IN_BYTE);
	if (avail_mem_st >= 0) // if not, rl_mem_avail() is not implemented
		fprintf(stderr, "runlog_available_ram_in_gb_start\t%.6f\n", avail_mem_st / RL_GB_IN_BYTE);
	fprintf(stderr, "runlog_command_line\t");
	for (i = o.ind; i < argc; ++i) {
		if (i != o.ind) fputc(' ', stderr);
		fprintf(stderr, "%s", argv[i]);
	}
	fputc('\n', stderr);
	fprintf(stderr, "runlog_start =====>\n");

	launch_cmd(argc - o.ind, argv + o.ind, &res);

	fprintf(stderr, "runlog_end <=====\n");
	if (avail_mem_st >= 0)
		fprintf(stderr, "runlog_available_ram_in_gb_end\t%.6f\n", (long)rl_mem_avail() / RL_GB_IN_BYTE);
	fprintf(stderr, "runlog_signal\t%d\n", res.sig);
	fprintf(stderr, "runlog_return_value\t%d\n", res.ret);
	fprintf(stderr, "runlog_real_time_in_sec\t%.3f\n", res.rtime);
	fprintf(stderr, "runlog_user_time_in_sec\t%.3f\n", res.utime);
	fprintf(stderr, "runlog_sys_time_in_sec\t%.3f\n", res.stime);
	fprintf(stderr, "runlog_peak_rss_in_mb\t%.6f\n", res.peak_mem / 1024.0 / 1024.0);
	return res.sig > 0? 2 : res.ret;
}
