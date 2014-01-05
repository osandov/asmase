/*
 * Common support for the built-in system.
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

#ifndef ASMASE_BUILTINS_SUPPORT_H
#define ASMASE_BUILTINS_SUPPORT_H

#include <memory>
#include <vector>

namespace Builtins {

class ErrorContext;
class ValueAST;
enum class ValueType;

/**
 * Find the value with the given key in the map object, returning the given
 * defaut value if it is not found.
 */
template <typename Map>
typename Map::mapped_type findWithDefault(Map map, typename Map::key_type key, typename Map::mapped_type defaultValue)
{
    typename Map::iterator it = map.find(key);
    if (it == map.end())
        return defaultValue;
    else
        return it->second;
}

/**
 * Assert that the given value has the given type. If it does not, print
 * the given error message.
 * @return True if there was a type error, false otherwise.
 */
bool checkValueType(const ValueAST &value, ValueType type,
                    const char *errorMsg, ErrorContext &errorContext);

bool wantsHelp(const std::vector<std::unique_ptr<ValueAST>> &args);

}

#endif /* ASMASE_BUILTINS_SUPPORT_H */
