#include <cstdio>
#include <cstring>

#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "RegisterInfo.h"
#include "Tracee.h"

int Tracee::executeInstruction(const std::string &instruction)
{
    unsigned char *shared = (unsigned char *) sharedMemory;
    if (instruction.size() + trapInstruction.size() >= sharedSize) {
        fprintf(stderr, "instruction too long\n");
        return 1;
    }

    memcpy(shared, instruction.c_str(), instruction.size());
    shared += instruction.size();
    memcpy(shared, trapInstruction.c_str(), trapInstruction.size());

    int wait_status;

    if (setProgramCounter(sharedMemory))
        return 1;

retry:
    if (ptrace(PTRACE_CONT, pid, NULL, 0) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not continue tracee");
        return 1;
    }

    if (waitpid(pid, &wait_status, 0) == -1) {
        perror("waitpid");
        fprintf(stderr, "could not wait for tracee");
        return 1;
    }

    if (WIFEXITED(wait_status)) {
        fprintf(stderr, "tracee exited with status %d\n",
                WEXITSTATUS(wait_status));
        return 1;
    } else if (WIFSIGNALED(wait_status)) {
        fprintf(stderr, "tracee was terminated (%s)\n",
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
                printf("tracee was stopped (%s)\n",
                       strsignal(WSTOPSIG(wait_status)));
                return 0;
        }
    } else if (WIFCONTINUED(wait_status)) {
        fprintf(stderr, "tracee continued\n");
        return 1;
    } else {
        fprintf(stderr, "tracee disappeared\n");
        return 1;
    }

    return 0;
}

static void traceeProcess() __attribute__((noreturn));
static void installTracerSignalHandlers();
Tracee *createPlatformTracee(pid_t pid, void *sharedMemory, size_t sharedSize);

std::shared_ptr<Tracee> createTracee()
{
    pid_t pid;
    void *sharedPage;
    size_t pageSize = sysconf(_SC_PAGESIZE);

    sharedPage = mmap(NULL, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (!sharedPage) {
        perror("mmap");
        fprintf(stderr, "could not create shared memory\n");
        return {nullptr};
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        fprintf(stderr, "could not fork tracee\n");
        return {nullptr};
    }

    if (pid == 0)
        traceeProcess(); /* This never returns. */

    installTracerSignalHandlers();

    return std::shared_ptr<Tracee>{createPlatformTracee(pid, sharedPage, pageSize)};
}

static void traceeProcess()
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

static void installTracerSignalHandlers()
{
    struct sigaction act = {
        .sa_handler = SIG_IGN,
        .sa_flags = 0
    };
    sigemptyset(&act.sa_mask);

    /* Ignore SIGINT so the user can break out of the tracee */
    sigaction(SIGINT, &act, NULL);
}
