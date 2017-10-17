/*
 * Tracee process: sandboxes itself and waits to be traced.
 *
 * Copyright (C) 2016-2017 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * asmase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * asmase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asmase.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>
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

static void close_fds(int memfd)
{
	DIR *dir;

	dir = opendir("/proc/self/fd");
	if (!dir)
		_exit(EXIT_FAILURE);

	for (;;) {
		struct dirent *dirent;
		int fd;

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

		if (fd != memfd && fd != dirfd(dir))
			close(fd);
	}
	if (errno)
		_exit(EXIT_FAILURE);

	closedir(dir);
}

void tracee(int memfd, int flags)
{
	static const char *name = "asmase_tracee";
	void *temp;

	if (ptrace(PTRACE_TRACEME, -1, NULL, NULL) == -1)
		_exit(EXIT_FAILURE);

	reset_signals();

	if (prctl(PR_SET_NAME, (long)name, 0, 0, 0) == -1)
		_exit(EXIT_FAILURE);

	if (flags & ASMASE_SANDBOX_FDS)
		close_fds(memfd);

	BUILD_BUG_ON(MEMFD_ADDR & (MEMFD_SIZE - 1));

	/*
	 * The temporary mapping must be aligned to MEMFD_SIZE so that it either
	 * doesn't overlap the final mapping at all or is exactly the same
	 * mapping.
	 */
	temp = mmap(NULL, 2 * MEMFD_SIZE, PROT_NONE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (temp == MAP_FAILED)
		_exit(EXIT_FAILURE);
	temp = (void *)(round_up((uintptr_t)temp, MEMFD_SIZE));

	temp = mmap(temp, MEMFD_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_SHARED | MAP_FIXED, memfd, 0);
	if (temp == MAP_FAILED)
		_exit(EXIT_FAILURE);

	memcpy(temp, arch_bootstrap_code, arch_bootstrap_code_len);
	((arch_bootstrap_func)temp)(memfd, flags & ASMASE_SANDBOX_SYSCALLS);
}
