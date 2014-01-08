/*
 * Built-in command system driver and core built-ins.
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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

/** Entry for a built-in command. */
class BuiltinCommand {
public:
    /** The function to run for the built-in. */
    BuiltinFunc func;

    /** Help string documentation. */
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
                printf("integer: 0x%1$lx (%1$ld)\n", arg->getInteger());
                break;
            case Builtins::ValueType::FLOAT:
                printf("floating: %f\n", arg->getFloat());
                break;
            case Builtins::ValueType::STRING:
                printf("string: %s\n", arg->getString().c_str());
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

/** Lookup table from full command name to built-in entry. */
static std::map<std::string, BuiltinCommand> commands = {
    {"test",      {builtin_test, "print evaluated arguments as a test"}},

    {"quit",      {builtin_quit, "quit the program"}},
    {"help",      {builtin_help, "print this help information"}},

    {"source",    {builtin_source, "redirect input to a given file"}},

    {"memory",    {builtin_memory,    "dump memory contents"}},
    {"registers", {builtin_registers, "dump register contents"}},

    {"warranty",  {builtin_warranty, "show warranty information"}},
    {"copying",   {builtin_copying,  "show copying information"}},
};

/**
 * Look up the abbreviation for a built-in command.
 * @return Zero on success, nonzero on failure (e.g., ambigious abbreviation).
 */
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

    if (potentialMatches.empty()) {
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
        for (auto &command : commands) {
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

/* See Builtins.h. */
bool isBuiltin(const std::string &str)
{
    size_t i = 0;
    while (isspace(str[i]))
        ++i;
    return str[i] == ':';
}

/* See Builtins.h. */
int runBuiltin(const std::string &line, Tracee &tracee, Inputter &inputter)
{
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
    Builtins::ErrorContext errorContext{inputter.currentFilename().c_str(),
                                        inputter.currentLineno(),
                                        line.c_str(), offset};
    Builtins::Environment env{tracee, inputter, errorContext};

    // Lex and parse the input
    Builtins::Scanner scanner{builtin};
    Builtins::Parser parser{scanner, errorContext};
    std::unique_ptr<Builtins::CommandAST> command{parser.parseCommand()};
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
        error = builtinCommand.func(evaledArgs, command->getCommand(),
                                    command->getCommandStart(),
                                    command->getCommandEnd(), env);

    return error;
}
