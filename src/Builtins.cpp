#include <cassert>
#include <cstdio>
#include <map>
#include <sstream>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Parser.h"
#include "Builtins/Support.h"

#include "Builtins.h"
#include "Inputter.h"

struct BuiltinCommand {
    BuiltinFunc func;
    std::string helpString;
};

static BUILTIN_FUNC(test)
{
    for (auto &arg : args) {
        switch (arg->getType()) {
            case Builtins::ValueType::IDENTIFIER:
                printf("identifier: %s\n", arg->getIdentifier().c_str());
                break;
            case Builtins::ValueType::INTEGER:
                printf("integer: %1$ld (0x%1$lx)\n", arg->getInteger());
                break;
            case Builtins::ValueType::FLOAT:
                printf("floating: %F\n", arg->getFloat());
                break;
            case Builtins::ValueType::STRING:
                printf("string: \"%s\"\n", arg->getString().c_str());
                break;
        }
    }

    return 0;
}

static BUILTIN_FUNC(quit)
{
    return -1;
}

static BUILTIN_FUNC(help);

static std::map<std::string, BuiltinCommand> commands = {
    {"test",      {builtin_test, "print evaluated arguments as a test"}},

    {"quit",      {builtin_quit, "quit the program"}},
    {"help",      {builtin_help, "print this help information"}},

    // {"source",    {builtin_source, "redirect input to a given file"}},

    {"memory",    {builtin_memory,    "dump memory contents"}},
    // {"registers", {builtin_registers, "dump register contents"}},

    {"warranty",  {builtin_warranty, "show warranty information"}},
    {"copying",   {builtin_copying,  "show copying information"}},
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
            if (checkValueType(*arg, Builtins::ValueType::IDENTIFIER,
                            "expected command identifier", env.errorContext))
                return 1;

            const std::string &abbrev = arg->getIdentifier();

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
        printf("  %-*s -- %s\n", (int) maxCommandLength, commandName, helpString);
    }

    return 0;
}

/* See builtins.h. */
bool isBuiltin(const std::string &str)
{
    size_t i = 0;
    while (isspace(str[i]))
        ++i;
    return str[i] == ':';
}

/* See builtins.h. */
int runBuiltin(const std::string &str, Tracee &tracee, const Inputter &inputter)
{
    std::string line = str.substr(0, str.size() - 1); // Don't want the newline

    // Make sure we were really given a built-in and trim the leading colon
    const char *builtin = line.c_str();
    int offset = 0;
    while (*builtin && *builtin != ':') {
        ++builtin;
        ++offset;
    }
    assert(*builtin == ':');
    ++builtin;
    ++offset;

    // Set up the environment to work in
    Builtins::ErrorContext errorContext(inputter.currentFilename().c_str(),
                                        inputter.currentLineno(),
                                        line.c_str(), offset);
    Builtins::Environment env(tracee, errorContext);

    // Lex and parse the input
    Builtins::Scanner scanner(builtin);
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
    if (!error) {
        error = builtinCommand.func(evaledArgs, command->getCommand(),
                                    command->getCommandStart(),
                                    command->getCommandEnd(), env);
    }

    return error;
}
