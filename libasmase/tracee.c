#include <dirent.h>
#include <errno.h>
#include <seccomp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>

#include "internal.h"

static void reset_signals(void)
{
	struct sigaction sa = {
		.sa_handler = SIG_DFL,
		.sa_flags = SA_RESTART,
	};
	sigset_t mask;
	int i;

	if (sigemptyset(&sa.sa_mask) == -1)
		_exit(EXIT_FAILURE);

	/*
	 * The only errors from sigaction() are EFAULT and EINVAL. EFAULT won't
	 * happen and EINVAL is expected.
	 */
	for (i = 1; i < NSIG; i++)
		sigaction(i, &sa, NULL);

	if (sigemptyset(&mask) == -1 || sigaddset(&mask, SIGWINCH) == -1)
		_exit(EXIT_FAILURE);

	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
		_exit(EXIT_FAILURE);
}

static void *mmap_memfd(int memfd, size_t size)
{
	void *addr;

	addr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED,
		    memfd, 0);
	if (addr == MAP_FAILED)
		_exit(EXIT_FAILURE);
	close(memfd);
	return addr;
}

static void close_all_fds(void)
{
	DIR *dir;

	dir = opendir("/proc/self/fd");
	if (!dir)
		_exit(EXIT_FAILURE);

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
			_exit(EXIT_FAILURE);

		if (fd != dirfd(dir))
			close(fd);
	}
	if (errno)
		_exit(EXIT_FAILURE);

	closedir(dir);
}

static void seccomp_all(void)
{
	scmp_filter_ctx ctx;

	ctx = seccomp_init(SCMP_ACT_TRAP);
	if (!ctx)
		_exit(EXIT_FAILURE);

	/* We munmap() everything after this. */
	if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0))
		_exit(EXIT_FAILURE);

	if (seccomp_load(ctx))
		_exit(EXIT_FAILURE);

	seccomp_release(ctx);
}

void tracee(int memfd, size_t memfd_size, int flags)
{
	void *memfd_addr;

	if (ptrace(PTRACE_TRACEME, -1, NULL, NULL) == -1)
		_exit(EXIT_FAILURE);

	reset_signals();

	memfd_addr = mmap_memfd(memfd, memfd_size);

	if (flags & ASMASE_SANDBOX_FDS)
		close_all_fds();

	if (flags & ASMASE_SANDBOX_SYSCALLS)
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
	_exit(EXIT_FAILURE);
}
