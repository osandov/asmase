#ifndef ASMASE_BUILTINS_H
#define ASMASE_BUILTINS_H

class Tracee;
class Inputter;

/** Return whether the given string is a command line built-in. */
bool isBuiltin(const std::string &str);

/**
 * Run a command line built-in.
 * @return Positive on error, 0 on success, negative on exit.
 * @return 1 on error, 0 on success, -1 on exit.
 */
int runBuiltin(const std::string &str, Tracee &tracee, Inputter &inputter);

#endif /* ASMASE_BUILTINS_H */
