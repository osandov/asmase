#ifndef ASMASE_BUILTINS_SUPPORT_H
#define ASMASE_BUILTINS_SUPPORT_H

namespace Builtins {

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

}

#endif /* ASMASE_BUILTINS_SUPPORT_H */
