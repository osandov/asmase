/*
 * registers built-in command for inspecting tracee registers.
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

#include <cstdio>
#include <sstream>
#include <unordered_map>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

#include "RegisterCategory.h"
#include "Tracee.h"

using Builtins::findWithDefault;

/** Lookup table for register categories. */
static std::unordered_map<std::string, RegisterCategory> categoryMap = {
    {"general-purpose", RegisterCategory::GENERAL_PURPOSE},
    {"general",         RegisterCategory::GENERAL_PURPOSE},
    {"gp",              RegisterCategory::GENERAL_PURPOSE},
    {"g",               RegisterCategory::GENERAL_PURPOSE},

    {"condition-codes", RegisterCategory::CONDITION_CODE},
    {"condition",       RegisterCategory::CONDITION_CODE},
    {"status",          RegisterCategory::CONDITION_CODE},
    {"flags",           RegisterCategory::CONDITION_CODE},
    {"cc",              RegisterCategory::CONDITION_CODE},

    {"floating-point", RegisterCategory::FLOATING_POINT},
    {"floating",       RegisterCategory::FLOATING_POINT},
    {"fp",             RegisterCategory::FLOATING_POINT},
    {"f",              RegisterCategory::FLOATING_POINT},

    {"extra", RegisterCategory::EXTRA},
    {"xr",    RegisterCategory::EXTRA},
    {"x",     RegisterCategory::EXTRA},

    {"segment", RegisterCategory::SEGMENTATION},
    {"seg",     RegisterCategory::SEGMENTATION},
    {"s",       RegisterCategory::SEGMENTATION},
};

static std::string getUsage(const std::string &commandName)
{
    std::stringstream ss;
    ss << "usage: " << commandName << " [CATEGORY...]";
    return ss.str();
}

BUILTIN_FUNC(registers)
{
    static RegisterCategory defaultCategories = 
        RegisterCategory::GENERAL_PURPOSE | RegisterCategory::PROGRAM_COUNTER |
        RegisterCategory::CONDITION_CODE;

    RegisterCategory categories = RegisterCategory::NONE;

    if (wantsHelp(args)) {
        std::string usage = getUsage(commandName);
        printf("%s\n", usage.c_str());
        printf(
            "Categories:\n"
            "  gp  -- general purpose registers\n"
            "  cc  -- condition code/status flag registers\n"
            "  fp  -- floating point registers\n"
            "  x   -- extra registers\n"
            "  seg -- segment registers\n");
        return 0;
    }

    for (auto &arg : args) {
        if (checkValueType(*arg, Builtins::ValueType::IDENTIFIER,
                    "expected register category", env.errorContext))
            return 1;

        const std::string &category = arg->getIdentifier();

        RegisterCategory regCat =
            findWithDefault(categoryMap, category, RegisterCategory::NONE);

        if (!any(regCat)) {
            env.errorContext.printMessage("unknown register category",
                                          arg->getStart());
            return 1;
        } else
            categories = categories | regCat;
    }

    if (!any(categories)) // This will be the case if there weren't any args
        categories = defaultCategories;

    env.tracee.printRegisters(categories);

    return 0;
}
