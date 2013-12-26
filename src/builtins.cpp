#include <cassert>
#include <cstdio>
#include <map>
#include <sstream>

#include "Builtins/Commands.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Parser.h"

#include "builtins.h"
#include "input.h"

static BUILTIN_FUNC(test)
{
    for (const Builtins::ValueAST *value : args) {
        Builtins::ValueType type = value->getType();
        if (type == Builtins::ValueType::IDENTIFIER) {
            auto identifier = static_cast<const Builtins::IdentifierExpr *>(value);
            printf("%s\n", identifier->getName().c_str());
        } else if (type == Builtins::ValueType::INTEGER) {
            auto integer = static_cast<const Builtins::IntegerExpr *>(value);
            printf("0x%016llx\n", integer->getValue());
        } else if (type == Builtins::ValueType::FLOAT) {
            auto floating = static_cast<const Builtins::FloatExpr *>(value);
            printf("%f\n", floating->getValue());
        } else if (type == Builtins::ValueType::STRING) {
            auto str = static_cast<const Builtins::StringExpr *>(value);
            printf("\"%s\"\n", str->getStr().c_str());
        } else
            assert(false);
    }

    return 0;
}

static std::map<std::string, BuiltinCommand> commands = {
    {"test", {builtin_test}},
    {"memory", {builtin_memory}},
    {"register", {builtin_register}},
};

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
    Builtins::Environment env(asmb, tracee, errorContext);

    // Lex and parse the input
    Builtins::Scanner scanner(str);
    Builtins::Parser parser(scanner, errorContext);
    Builtins::CommandAST *command = parser.parseCommand();
    if (!command)
        return 1;

    // Evaluate the input by looping over the command arguments and evaulating
    // each one
    std::vector<Builtins::ValueAST*> values;
    std::vector<Builtins::ExprAST*>::iterator it;
    bool failedEval = false;
    for (Builtins::ExprAST *expr : *command) {
        Builtins::ValueAST *value = expr->eval(env);
        failedEval |= value == NULL;
        values.push_back(value);
    }

    if (failedEval) {
        delete command;
        return 1;
    }

    const std::string &abbrev = command->getCommand();

    // Find the given command. 
    std::vector<std::string> potentialMatches;
    for (const std::pair<std::string, BuiltinCommand> &command : commands) {
        if (command.first.compare(0, abbrev.size(), abbrev) == 0) {
            if (abbrev.size() == command.first.size()) {
                // Allow full matches to bypass the ambiguity check
                potentialMatches.clear();
                potentialMatches.push_back(command.first);
                break;
            } else
                potentialMatches.push_back(command.first);
        }
    }

    int error = 0;
    if (potentialMatches.size() == 0) {
        errorContext.printMessage("unknown command", command->getCommandStart());
        error = 1;
    } else if (potentialMatches.size() > 1) {
        std::stringstream ss;
        ss << "ambigious command; did you mean ";
        for (size_t i = 0; i < potentialMatches.size() - 1; ++i)
            ss << '\'' << potentialMatches[i] << "', ";
        ss << "or '" << potentialMatches.back() << "'?";
        errorContext.printMessage(ss.str().c_str(), command->getCommandStart());
        error = 1;
    } else {
        BuiltinFunc func = commands[potentialMatches.front()].func;
        error = func(values, env);
    }

    delete command;
    return error;
}

}
