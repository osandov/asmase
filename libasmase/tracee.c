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

static void reset_signals(void)
{
	struct sigaction sa = {
		.sa_handler = SIG_DFL,
		.sa_flags = SA_RESTART,
	};
	sigset_t mask;
	int i;

	if (sigemptyset(&sa.sa_mask) == -1)
		exit(EXIT_FAILURE);

	/*
	 * The only errors from sigaction() are EFAULT and EINVAL. EFAULT won't
	 * happen and EINVAL is expected.
	 */
	for (i = 1; i < NSIG; i++)
		sigaction(i, &sa, NULL);

	if (sigemptyset(&mask) == -1)
		exit(EXIT_FAILURE);

	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
		exit(EXIT_FAILURE);
}

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
		"  --seccomp        disallow all syscalls with seccomp\n"
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
		{"seccomp", no_argument, NULL, 'S'},
		{"help", no_argument, NULL, 'h'},
		{}
	};
	int memfd = -1;
	unsigned long memfd_size = 0;
	bool close_fds = false;
	bool use_seccomp = false;
	void *memfd_addr;

	if (ptrace(PTRACE_TRACEME, -1, NULL, NULL) == -1)
		exit(EXIT_FAILURE);

	reset_signals();

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
		case 'S':
			use_seccomp = true;
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

	memfd_addr = mmap_memfd(memfd, memfd_size);

	if (close_fds)
		close_all_fds();

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
