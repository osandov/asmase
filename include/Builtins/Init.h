#ifndef ASMASE_BUILTINS_INIT_H
#define ASMASE_BUILTINS_INIT_H

#include <map>
#include "Builtins/AST.h"
#include "Builtins/Token.h"

namespace Builtins {

extern std::map<TokenType, UnaryOpcode> unaryTokenMap;
extern std::map<TokenType, BinaryOpcode> binaryTokenMap;

typedef ValueAST* (ValueAST::*UnaryOpFunction)(Environment &) const;
extern std::map<UnaryOpcode, UnaryOpFunction> unaryFunctionMap;

typedef ValueAST* (ValueAST::*BinaryOpFunction)(const ValueAST *, Environment &) const;
extern std::map<BinaryOpcode, BinaryOpFunction> binaryFunctionMap;

extern std::map<BinaryOpcode, int> binaryPrecedences;

int initParser();

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

#endif /* ASMASE_BUILTINS_INIT_H */
