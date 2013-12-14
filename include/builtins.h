#ifndef ASIP_BUILTINS_H
#define ASIP_BUILTINS_H

#include "assembler.h"
#include "tracing.h"

/** Return whether the given string is a command line builtin. */
int is_builtin(const char *str);

/**
 * Run a command line builtin.
 * @return -1 on error, 0 on succes, 1 on exit.
 */
int run_builtin(struct assembler *asmb, struct tracee_info *tracee, char *str);

#endif /* ASIP_BUILTINS_H */
