#ifndef ASMASE_BUILTINS_SUPPORT_H
#define ASMASE_BUILTINS_SUPPORT_H

namespace Builtins {

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
