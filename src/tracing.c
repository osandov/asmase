/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "tracing.h"

/**
 * The child process' (a.k.a. the tracee) main function. All it does is ask to
 * be traced.
 */
static void tracee_process()
{
    if (ptrace(PTRACE_TRACEME, -1, NULL, NULL) == -1) {
        perror("ptrace");
        abort();
    }

    if (raise(SIGTRAP)) {
        perror("raise");
        abort();
    }

    /* We shouldn't make it here, but if we do... */
    abort();
}

/** Set up any signal handlers needed by the parent (a.k.a. the tracer). */
static void install_tracer_signal_handlers()
{
    struct sigaction act = {
        .sa_handler = SIG_IGN,
        .sa_flags = 0
    };
    sigemptyset(&act.sa_mask);

    /* Ignore SIGINT so the user can break out of the tracee */
    sigaction(SIGINT, &act, NULL);
}

/* See tracing.h. */
int create_tracee(struct tracee_info *tracee)
{
    pid_t pid;
    void *shared_page;
    size_t page_size = sysconf(_SC_PAGESIZE);

    shared_page = mmap(NULL, page_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (!shared_page) {
        perror("mmap");
        return 1;
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0)
        tracee_process(); /* This never returns. */

    tracee->pid = pid;
    tracee->shared_page = shared_page;

    install_tracer_signal_handlers();

    return 0;
}

/* See tracing.h. */
void cleanup_tracing(struct tracee_info *tracee)
{
    size_t page_size = sysconf(_SC_PAGESIZE);

    kill(tracee->pid, SIGKILL);

    if (munmap(tracee->shared_page, page_size) == -1)
        perror("munmap");
}

/* See tracing.h. */
int execute_instruction(struct tracee_info *tracee, unsigned char *mc_buffer,
                        size_t mc_length)
{
    pid_t pid = tracee->pid;
    unsigned char *pc = (unsigned char *) tracee->shared_page;
    size_t page_size = sysconf(_SC_PAGESIZE);
    int wait_status;

    if (mc_length > page_size) {
        fprintf(stderr, "Instruction too long.\n");
        return 1;
    }

    memcpy(pc, mc_buffer, mc_length);
    if (generate_sigtrap(pc + mc_length, page_size - mc_length))
        return 1;
    set_program_counter(pid, pc);

retry:
    if (ptrace(PTRACE_CONT, pid, NULL, 0) == -1) {
        perror("ptrace");
        fprintf(stderr, "Could not continue tracee.\n");
        return 1;
    }

    if (waitpid(pid, &wait_status, 0) == -1) {
        perror("waitpid");
        fprintf(stderr, "Could not wait for tracee.\n");
        return 1;
    }

    if (WIFEXITED(wait_status)) {
        fprintf(stderr, "Tracee exited with status %d.\n",
                WEXITSTATUS(wait_status));
        return 1;
    } else if (WIFSIGNALED(wait_status)) {
        fprintf(stderr, "Tracee was terminated (%s).\n",
                strsignal(WTERMSIG(wait_status)));
        return 1;
    } else if (WIFSTOPPED(wait_status)) {
        int signal = WSTOPSIG(wait_status);
        switch (signal) {
            case SIGTRAP:
                break;
            case SIGWINCH:
                /*
                 * We don't want to be interrupted if the window changes size,
                 * so continue the process and keep waiting.
                 */
                goto retry;
            default:
                printf("Tracee was stopped (%s).\n",
                       strsignal(WSTOPSIG(wait_status)));
                return 0;
        }
    } else if (WIFCONTINUED(wait_status)) {
        fprintf(stderr, "Tracee continued.\n");
        return 1;
    } else {
        fprintf(stderr, "Tracee disappeared.\n");
        return 1;
    }

    return 0;
}
