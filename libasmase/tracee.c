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
#include <sys/stat.h>

#include "util.h"

static void mmap_memfd(int pipefd, int memfd, unsigned int size)
{
	void *addr;

	addr = mmap(NULL, size, PROT_READ | PROT_EXEC, MAP_SHARED, memfd, 0);
	if (addr == MAP_FAILED)
		exit(EXIT_FAILURE);
	close(memfd);

	write(pipefd, &addr, sizeof(addr));
	close(pipefd);
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
		"usage: asmase_tracee --pipefd=FD --memfd=FD --memfd-size=SIZE [OPTIONS]\n"
		"\n"
		"Sandbox options:\n"
		"  --close-fds    close all file descriptors\n"
		"  --seccomp      disallow all syscalls with seccomp\n"
		"\n"
		"Miscellaneous:\n"
		"  -h, --help     display this help message and exit\n");
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	const struct option long_options[] = {
		{"pipefd", required_argument, NULL, 'p'},
		{"memfd", required_argument, NULL, 'f'},
		{"memfd-size", required_argument, NULL, 's'},
		{"close-fds", no_argument, NULL, 'c'},
		{"seccomp", no_argument, NULL, 'S'},
		{"help", no_argument, NULL, 'h'},
		{}
	};
	int pipefd = -1, memfd = -1;
	unsigned long memfd_size = 0;
	bool close_fds = false, use_seccomp = false;

	for (;;) {
		int c;

		c = getopt_long(argc, argv, "h", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'p':
			errno = 0;
			pipefd = simple_strtoi(optarg);
			if (errno)
				usage(true);
			break;
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
	if (pipefd < 0 || memfd < 0 || memfd_size == 0)
		usage(true);

	mmap_memfd(pipefd, memfd, memfd_size);
	if (close_fds)
		close_all_fds();

	ptrace(PTRACE_TRACEME, -1, NULL, NULL);
	if (use_seccomp)
		seccomp_all();
	/*
	 * If seccomp is enabled, we'll get SIGSYS instead of SIGTRAP; that's
	 * okay.
	 */
	raise(SIGTRAP);

	/* We shouldn't make it here. */
	return EXIT_FAILURE;
}
