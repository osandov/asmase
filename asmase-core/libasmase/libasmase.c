/*
 * Core libasmase support.
 *
 * Copyright (C) 2016-2018 Omar Sandoval
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

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include "internal.h"

__attribute__((visibility("default")))
int libasmase_init(void)
{
	libasmase_assembler_init();
	return 0;
}

static int create_memfd(struct asmase_instance *a)
{
	a->memfd = syscall(SYS_memfd_create, "asmase", 0);
	if (a->memfd == -1)
		return -1;

	if (ftruncate(a->memfd, MEMFD_SIZE) == -1) {
		close(a->memfd);
		return -1;
	}

	return 0;
}

static pid_t fork_tracee(struct asmase_instance *a, int flags)
{
	pid_t pid = -1;

	pid = fork();
	if (pid == 0)
		tracee(a->memfd, flags); /* Doesn't return. */
	else
		return pid;
}

static int attach_to_tracee(struct asmase_instance *a)
{
	int wstatus;

	if (waitpid(a->pid, &wstatus, 0) == -1)
		return -1;

	/*
	 * If the tracee exited or was signaled before it requested to be
	 * ptrace'd, we can't attach to it. If it stopped for some reason other
	 * than trapping, then consider it dead.
	 */
	if (!WIFSTOPPED(wstatus) || WSTOPSIG(wstatus) != SIGTRAP) {
		errno = ECHILD;
		return -1;
	}

	if (ptrace(PTRACE_SETOPTIONS, a->pid, NULL, PTRACE_O_EXITKILL) == -1)
		return -1;

	if (arch_get_regs(a->pid, &a->regs) == -1)
		return -1;

	return 0;
}

static int zero_out_memfd(struct asmase_instance *a)
{
	static const char zeros[MEMFD_SIZE];
	ssize_t sret;

	sret = pwrite(a->memfd, zeros, sizeof(zeros), 0);
	if (sret == -1)
		return -1;

	return 0;
}

static int check_maps(struct asmase_instance *a)
{
	FILE *file;
	char path[50];
	int ret;

	snprintf(path, sizeof(path), "/proc/%ld/maps", (long)a->pid);

	file = fopen(path, "r");
	if (!file)
		return -1;

	for (;;) {
		unsigned long start, end;
		char *path;
		char c;

		ret = fscanf(file, "%lx-%lx %*c%*c%*c%*c %*x %*x:%*x %*u",
			     &start, &end);
		if (ret == EOF)
			break;

		ret = fscanf(file, "%*[ ]%m[^\n]", &path);
		if (ret == EOF) {
			if (!ferror(file))
				errno = EINVAL;
			goto err;
		}

		if (strstartswith(path, "/memfd:asmase")) {
			if (start != MEMFD_ADDR ||
			    (end - start) != MEMFD_SIZE) {
				errno = EADDRNOTAVAIL;
				goto err;
			}
		} else if (strcmp(path, "[vsyscall]") != 0) {
			errno = EADDRINUSE;
			goto err;
		}

		c = fgetc(file);
		if (c != '\n') {
			if (!ferror(file))
				errno = EINVAL;
			goto err;
		}
	}
	if (ferror(file))
		goto err;

	fclose(file);
	return 0;

err:
	fclose(file);
	return -1;
}

static int check_fds(struct asmase_instance *a)
{
	DIR *dir;
	char path[50];
	int ret = -1;

	snprintf(path, sizeof(path), "/proc/%ld/fd", (long)a->pid);

	dir = opendir(path);
	if (!dir)
		return -1;

	for (;;) {
		struct dirent *dirent;

		errno = 0;
		dirent = readdir(dir);
		if (!dirent) {
			if (!errno)
				ret = 0;
			break;
		}

		if (strcmp(dirent->d_name, ".") != 0 &&
		    strcmp(dirent->d_name, "..") != 0) {
			errno = EBADF;
			break;
		}
	}

	closedir(dir);
	return ret;
}

static int check_seccomp(struct asmase_instance *a)
{
	FILE *file;
	char buf[50];
	bool no_new_privs = false, seccomp = false;
	int ret;

	snprintf(buf, sizeof(buf), "/proc/%ld/status", (long)a->pid);

	file = fopen(buf, "r");
	if (!file)
		return -1;

	for (;;) {
		int status;

		if (!fgets(buf, sizeof(buf), file))
			break;

		ret = sscanf(buf, "NoNewPrivs: %d", &status);
		if (ret == EOF)
			break;
		if (ret == 1) {
			no_new_privs = status;
			continue;
		}

		ret = sscanf(buf, "Seccomp: %d", &status);
		if (ret == EOF)
			break;
		if (ret == 1) {
			seccomp = status == 2;
			continue;
		}
	}
	if (ferror(file)) {
		ret = -1;
		goto out;
	}

	if (!no_new_privs || !seccomp) {
		errno = EPERM;
		ret = -1;
		goto out;
	}

	ret = 0;
out:
	fclose(file);
	return ret;
}

__attribute__((visibility("default")))
struct asmase_instance *asmase_create_instance(int flags)
{
	struct asmase_instance *a;
	int saved_errno;

	if (flags & ~(ASMASE_SANDBOX_ALL)) {
		errno = EINVAL;
		return NULL;
	}

	a = malloc(sizeof(*a));
	if (!a)
		return NULL;

	if (create_memfd(a) == -1) {
		free(a);
		return NULL;
	}

	a->pid = fork_tracee(a, flags);
	if (a->pid == -1) {
		close(a->memfd);
		free(a);
		return NULL;
	}

	if (attach_to_tracee(a) == -1)
		goto err;

	if (zero_out_memfd(a) == -1)
		goto err;

	if (check_maps(a) == -1)
		goto err;

	if ((flags & ASMASE_SANDBOX_FDS) && check_fds(a) == -1)
		goto err;

	if ((flags & ASMASE_SANDBOX_SYSCALLS) && check_seccomp(a) == -1)
		goto err;

	return a;

err:
	saved_errno = errno;
	asmase_destroy_instance(a);
	errno = saved_errno;
	return NULL;
}

__attribute__((visibility("default")))
void asmase_destroy_instance(struct asmase_instance *a)
{
	kill(a->pid, SIGKILL);
	waitpid(a->pid, NULL, 0);
	close(a->memfd);
	free(a);
}

__attribute__((visibility("default")))
pid_t asmase_getpid(const struct asmase_instance *a)
{
	return a->pid;
}

static int reset_tracee_program_counter(struct asmase_instance *a)
{
	union asmase_register_value value;

	assert(asmase_registers[0].set == ASMASE_REGISTERS_PROGRAM_COUNTER);
	switch (asmase_registers[0].type) {
	case ASMASE_REGISTER_U32:
		value.u32 = (uint32_t)MEMFD_ADDR;
		break;
	case ASMASE_REGISTER_U64:
		value.u64 = (uint64_t)MEMFD_ADDR;
		break;
	default:
		assert(false && "Invalid program counter type");
		errno = EINVAL;
		return -1;
	}

	return asmase_set_register(a, &asmase_registers[0], &value);
}

__attribute__((visibility("default")))
int asmase_execute_code(struct asmase_instance *a, const char *code, size_t len,
			int *wstatus)
{
	struct iovec iov[] = {
		{(void *)code, len},
		{(void *)arch_trap_instruction, arch_trap_instruction_len},
	};
	ssize_t sret;
	size_t total_len;
	bool overflow;

	overflow = __builtin_add_overflow(len, arch_trap_instruction_len,
					  &total_len);
	if (overflow || total_len > CODE_MAX_SIZE) {
		errno = E2BIG;
		return -1;
	}

	sret = pwritev(a->memfd, iov, ARRAY_SIZE(iov), 0);
	if (sret == -1)
		return -1;

	if (reset_tracee_program_counter(a) == -1) {
		if (errno == ESRCH)
			goto wait;
		return -1;
	}
	if (ptrace(PTRACE_CONT, a->pid, NULL, NULL) == -1 && errno != ESRCH)
		return -1;

wait:
	if (waitpid(a->pid, wstatus, 0) == -1)
		return -1;

	if (arch_get_regs(a->pid, &a->regs) == -1 && errno != ESRCH)
		return -1;

	return 0;
}

static inline size_t asmase_register_type_size(enum asmase_register_type type)
{
	switch (type) {
	case ASMASE_REGISTER_U8:
		return sizeof(uint8_t);
	case ASMASE_REGISTER_U16:
		return sizeof(uint16_t);
	case ASMASE_REGISTER_U32:
		return sizeof(uint32_t);
	case ASMASE_REGISTER_U64:
		return sizeof(uint64_t);
	case ASMASE_REGISTER_U128:
		return 2 * sizeof(uint64_t);
	case ASMASE_REGISTER_FLOAT80:
		return sizeof(long double);
	default:
		assert(false && "unknown register type");
		return 0;
	}
}

void default_copy_register(const struct asmase_register_descriptor *desc,
			   void *dst, const struct arch_regs *src)
{
	memcpy(dst, (char *)src + desc->offset,
	       asmase_register_type_size(desc->type));
}

__attribute__((visibility("default")))
void asmase_get_register(const struct asmase_instance *a,
			 const struct asmase_register_descriptor *reg,
			 union asmase_register_value *value)
{
	reg->copy_register_fn(reg, value, &a->regs);
}

__attribute__((visibility("default")))
int asmase_set_register(struct asmase_instance *a,
			const struct asmase_register_descriptor *reg,
			const union asmase_register_value *value)
{
	if (reg->copy_register_fn != default_copy_register) {
		errno = EOPNOTSUPP;
		return -1;
	}

	memcpy((char *)&a->regs + reg->offset, value,
	       asmase_register_type_size(reg->type));
	return arch_set_regs(a->pid, &a->regs);
}


__attribute__((visibility("default")))
char *asmase_status_register_format(const struct asmase_register_descriptor *reg,
				    const struct asmase_status_register_bits *bits,
				    const union asmase_register_value *value)
{
	uint8_t bits_value;
	char *str;
	int ret;

#define BITS_VALUE(value, bits) ((value >> bits->shift) & bits->mask)
	switch (reg->type) {
	case ASMASE_REGISTER_U8:
		bits_value = BITS_VALUE(value->u8, bits);
		break;
	case ASMASE_REGISTER_U16:
		bits_value = BITS_VALUE(value->u16, bits);
		break;
	case ASMASE_REGISTER_U32:
		bits_value = BITS_VALUE(value->u32, bits);
		break;
	case ASMASE_REGISTER_U64:
		bits_value = BITS_VALUE(value->u64, bits);
		break;
	case ASMASE_REGISTER_U128:
		bits_value = BITS_VALUE(value->u128, bits);
		break;
	default:
		assert(false && "Invalid status register type");
		errno = EINVAL;
		return NULL;
	}
#undef BITS_VALUE

	if (bits->mask == 0x1) {
		if (bits_value)
			return strdup(bits->name);
		else
			return strdup("");
	}

	if (bits->values)
		ret = asprintf(&str, "%s=%s", bits->name, bits->values[bits_value]);
	else
		ret = asprintf(&str, "%s=0x%x", bits->name, bits_value);
	if (ret == -1)
		return NULL;

	return str;
}

__attribute__((visibility("default")))
ssize_t asmase_readv_memory(const struct asmase_instance *a,
			    const struct iovec *local_iov, size_t liovcnt,
			    const struct iovec *remote_iov, size_t riovcnt)
{
	return process_vm_readv(a->pid, local_iov, liovcnt, remote_iov, riovcnt,
				0);
}

__attribute__((visibility("default")))
ssize_t asmase_read_memory(const struct asmase_instance *a, void *buf,
			   void *addr, size_t len)
{
	struct iovec local_iov = {
		.iov_base = buf,
		.iov_len = len,
	};
	struct iovec remote_iov = {
		.iov_base = addr,
		.iov_len = len,
	};

	return process_vm_readv(a->pid, &local_iov, 1, &remote_iov, 1, 0);
}
