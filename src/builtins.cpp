#include <cassert>
#include <cstdio>
#include <map>
#include <sstream>

#include "Builtins/Commands.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Parser.h"

#include "builtins.h"
#include "input.h"

struct BuiltinCommand {
    BuiltinFunc func;
    std::string helpString;
};

static BUILTIN_FUNC(test)
{
    for (auto &argPtr : args) {
        Builtins::ValueAST *arg = argPtr.get();
        Builtins::ValueType type = arg->getType();
        if (type == Builtins::ValueType::IDENTIFIER) {
            auto identifier = static_cast<const Builtins::IdentifierExpr *>(arg);
            printf("identifier: %s\n", identifier->getName().c_str());
        } else if (type == Builtins::ValueType::INTEGER) {
            auto integer = static_cast<const Builtins::IntegerExpr *>(arg);
            printf("integer: %1$lld (0x%1$llx)\n", integer->getValue());
        } else if (type == Builtins::ValueType::FLOAT) {
            auto floating = static_cast<const Builtins::FloatExpr *>(arg);
            printf("floating: %f\n", floating->getValue());
        } else if (type == Builtins::ValueType::STRING) {
            auto str = static_cast<const Builtins::StringExpr *>(arg);
            printf("string: \"%s\"\n", str->getStr().c_str());
        } else
            assert(false);
    }

    return 0;
}

static BUILTIN_FUNC(quit)
{
    return -1;
}

static BUILTIN_FUNC(help);

static std::map<std::string, BuiltinCommand> commands = {
    {"test",     {builtin_test, "print evaluated arguments as a test"}},

    {"quit",     {builtin_quit, "quit the program"}},
    {"help",     {builtin_help, "print this help information"}},

    {"source",   {builtin_source, "redirect input to a given file"}},

    {"memory",   {builtin_memory,   "dump memory contents"}},
    {"register", {builtin_register, "dump register contents"}},

    {"warranty", {builtin_warranty, "show warranty information"}},
    {"copying",  {builtin_copying,  "show copying information"}},
};

static int lookupCommand(const std::string &abbrev,
                         BuiltinCommand &commandOut,
                         int commandStart,
                         Builtins::ErrorContext &errorContext)
{
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

    if (potentialMatches.size() == 0) {
        errorContext.printMessage("unknown command", commandStart);
        return 1;
    } else if (potentialMatches.size() > 1) {
        std::stringstream ss;
        ss << "ambigious command; did you mean ";
        for (size_t i = 0; i < potentialMatches.size() - 1; ++i)
            ss << '\'' << potentialMatches[i] << "', ";
        ss << "or '" << potentialMatches.back() << "'?";
        errorContext.printMessage(ss.str().c_str(), commandStart);
        return 1;
    } else {
        commandOut = commands[potentialMatches.front()];
        return 0;
    }
}

static BUILTIN_FUNC(help)
{
    std::vector<std::pair<const char *, const char *>> wantedCommands;

    int maxCommandLength = 0;
    if (args.size() == 0) {
        for (auto command : commands) {
            const char *commandName = command.first.c_str();
            const char *helpString = command.second.helpString.c_str();

            int commandLength = command.first.size();
            maxCommandLength = std::max(commandLength, maxCommandLength);

            wantedCommands.emplace_back(commandName, helpString);
        }
    } else {
        for (auto &argPtr : args) {
            Builtins::ValueAST *arg = argPtr.get();
            if (arg->getType() != Builtins::ValueType::IDENTIFIER) {
                env.errorContext.printMessage("expected command identifier",
                                              arg->getStart());
                return 1;
            }

            auto abbrevExpr =
                static_cast<const Builtins::IdentifierExpr *>(arg);
            const std::string &abbrev = abbrevExpr->getName();

            BuiltinCommand builtinCommand;
            int error = lookupCommand(abbrev, builtinCommand, arg->getStart(),
                                      env.errorContext);
            if (error)
                return 1;

            int abbrevLength = abbrev.size();
            maxCommandLength = std::max(abbrevLength, maxCommandLength);
            wantedCommands.emplace_back(abbrev.c_str(),
                                        builtinCommand.helpString.c_str());
        }
    }

    for (auto &command : wantedCommands) {
        const char *commandName = command.first;
        const char *helpString = command.second;
        printf("%-*s -- %s\n", (int) maxCommandLength, commandName, helpString);
    }

    return 0;
}

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
    std::unique_ptr<Builtins::CommandAST> command(parser.parseCommand());
    if (!command)
        return 1;

    // Evaluate the input by looping over the command arguments and evaulating
    // each one
    std::vector<std::unique_ptr<Builtins::ValueAST>> evaledArgs;
    std::vector<Builtins::ExprAST*>::iterator it;
    bool failedEval = false;
    for (auto &expr : *command) {
        Builtins::ValueAST *value = expr->eval(env);
        failedEval |= value == nullptr;
        evaledArgs.emplace_back(value);
    }

    if (failedEval)
        return 1;

    // Look up the command and execute it
    const std::string &abbrev = command->getCommand();
    BuiltinCommand builtinCommand;
    int error = lookupCommand(abbrev, builtinCommand,
                              command->getCommandStart(), errorContext);
    if (!error)
        error = builtinCommand.func(evaledArgs, env);

    return error;
}

}
