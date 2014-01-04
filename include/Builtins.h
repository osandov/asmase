#ifndef ASMASE_BUILTINS_H
#define ASMASE_BUILTINS_H

class Inputter;
class Tracee;

/**
 * Return whether the given string should be interpreted as a command line
 * built-in.
 */
bool isBuiltin(const std::string &str);

/**
 * Run a command line built-in.
 * @return Positive on error, 0 on success, negative on exit.
 */
int runBuiltin(const std::string &str, Tracee &tracee, Inputter &inputter);

#endif /* ASMASE_BUILTINS_H */
