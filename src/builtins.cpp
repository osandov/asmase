#include <cassert>

#include "Builtins/ErrorContext.h"
#include "Builtins/Parser.h"

#include "builtins.h"
#include "input.h"

extern "C" {

/* See builtins.h. */
int is_builtin(const char *str)
{
    while (*str && isspace(*str))
        ++str;
    return str[0] == ':';
}

/* See builtins.h. */
int run_builtin(struct assembler *asmb, struct tracee_info *tracee, char *str)
{
    struct source_file *current_file = get_current_file();

    // Make sure we were really given a built-in and trim the leading colon
    const char *line = str;
    int offset = 0;
    while (*str && *str != ':') {
        ++str;
        ++offset;
    }

    assert(*str == ':');
    ++str;
    ++offset;

    // Set up the environment to work in
    Builtins::ErrorContext errorContext(current_file->filename,
                                        current_file->line, line, offset);
    Builtins::Environment env(errorContext);

    // Lex and parse the input
    Builtins::Scanner scanner(str);
    Builtins::Parser parser(scanner, errorContext);
    Builtins::CommandAST *command = parser.parseCommand();
    if (!command)
        return 1;


    // Evaluate the input by looping over the command arguments
    std::vector<Builtins::ValueAST*> values;
    std::vector<Builtins::ExprAST*>::iterator it;
    for (it = command->begin(); it != command->end(); ++it)
        values.push_back((*it)->eval(env));

    return 0;
}

}
