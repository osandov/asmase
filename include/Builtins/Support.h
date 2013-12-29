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
