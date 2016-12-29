#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <seccomp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "util.h"

static void *mmap_memfd(int memfd, unsigned int size)
{
	void *addr;

	addr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED,
		    memfd, 0);
	if (addr == MAP_FAILED)
		exit(EXIT_FAILURE);
	close(memfd);
	return addr;
}

static void close_all_fds(void)
{
	DIR *dir;

	dir = opendir("/proc/self/fd");
	if (!dir)
		exit(EXIT_FAILURE);

	for (;;) {
		struct dirent *dirent;
		long fd;

		errno = 0;
		dirent = readdir(dir);
		if (!dirent)
			break;

		if (strcmp(dirent->d_name, ".") == 0 ||
		    strcmp(dirent->d_name, "..") == 0)
			continue;

		errno = 0;
		fd = simple_strtoi(dirent->d_name);
		if (errno)
			exit(EXIT_FAILURE);

		if (fd != dirfd(dir))
			close(fd);
	}
	if (errno)
		exit(EXIT_FAILURE);

	closedir(dir);
}

static void set_stack_limit(unsigned long bytes)
{
	struct rlimit rlimit;

	if (getrlimit(RLIMIT_STACK, &rlimit) == -1)
		exit(EXIT_FAILURE);

	if (rlimit.rlim_max <= bytes)
		return;

	rlimit.rlim_max = bytes;
	if (rlimit.rlim_cur > bytes)
		rlimit.rlim_cur = bytes;

	if (setrlimit(RLIMIT_STACK, &rlimit) == -1)
		exit(EXIT_FAILURE);
}

static void seccomp_all(void)
{
	scmp_filter_ctx *ctx;

	ctx = seccomp_init(SCMP_ACT_TRAP);
	if (!ctx)
		exit(EXIT_FAILURE);

	if (seccomp_load(ctx))
		exit(EXIT_FAILURE);

	seccomp_release(ctx);
}

static void usage(bool error)
{
	fprintf(error ? stderr : stdout,
		"usage: asmase_tracee --memfd=FD --memfd-size=SIZE [OPTIONS]\n"
		"\n"
		"Sandbox options:\n"
		"  --close-fds      close all file descriptors\n"
		"  --nice=NICE      set (and limit) the nice value\n"
		"  --seccomp        disallow all syscalls with seccomp\n"
		"  --stack=BYTES    limit the size of the stack\n"
		"\n"
		"Miscellaneous:\n"
		"  -h, --help     display this help message and exit\n");
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	const struct option long_options[] = {
		{"memfd", required_argument, NULL, 'f'},
		{"memfd-size", required_argument, NULL, 's'},
		{"close-fds", no_argument, NULL, 'c'},
		{"nice", required_argument, NULL, 'n'},
		{"seccomp", no_argument, NULL, 'S'},
		{"stack", required_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'},
		{}
	};
	int memfd = -1;
	unsigned long memfd_size = 0;
	bool close_fds = false;
	bool nice_flag = false;
	bool use_seccomp = false;
	bool limit_stack = false;
	void *memfd_addr;
	unsigned long stack_size = 0;
	int nice = 0;

	if (ptrace(PTRACE_TRACEME, -1, NULL, NULL) == -1)
		exit(EXIT_FAILURE);

	/* Work around autogroup for nice. */
	if (setsid() == -1)
		exit(EXIT_FAILURE);

	for (;;) {
		int c;

		c = getopt_long(argc, argv, "h", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'f':
			errno = 0;
			memfd = simple_strtoi(optarg);
			if (errno)
				usage(true);
			break;
		case 's':
			errno = 0;
			memfd_size = simple_strtoul(optarg);
			if (errno)
				usage(true);
			break;
		case 'c':
			close_fds = true;
			break;
		case 'n':
			errno = 0;
			nice = simple_strtoi(optarg);
			if (errno)
				usage(true);
			nice_flag = true;
			break;
		case 'S':
			use_seccomp = true;
			break;
		case 't':
			errno = 0;
			stack_size = simple_strtoul(optarg);
			if (errno)
				usage(true);
			limit_stack = true;
			break;
		case 'h':
			usage(false);
			break;
		default:
			usage(true);
			break;
		}
	}
	if (optind != argc)
		usage(true);
	if (memfd < 0 || memfd_size == 0)
		usage(true);

	if (nice_flag) {
		if (setpriority(PRIO_PROCESS, 0, nice) == -1)
			exit(EXIT_FAILURE);
	}

	memfd_addr = mmap_memfd(memfd, memfd_size);

	if (close_fds)
		close_all_fds();

	if (limit_stack)
		set_stack_limit(stack_size);

	if (use_seccomp)
		seccomp_all();

	/*
	 * This serves two purposes: it tells the tracer where we mapped the
	 * memfd and it lets the tracer know that we're done setting up.
	 */
	*(void **)memfd_addr = memfd_addr;

	/*
	 * If seccomp is enabled, we'll get SIGSYS instead of SIGTRAP; that's
	 * okay.
	 */
	raise(SIGTRAP);

	/* We shouldn't make it here. */
	exit(EXIT_FAILURE);
}
