#pragma once

#include<string>
#include<unordered_map>
#include<vector>

#include"kwisatz/semant/type.h"
#include"kwisatz/util/source_location.h"

namespace kwisatz{

enum class SymbolKind{
    Var,
    Param,
    Func,
    Struct,
    BuiltinLength,
};

struct Symbol{
    SymbolKind kind;
    std::string name;
    Type* type;
    SourceLocation declLoc;
};

class Env{
public:
    Env();

    void enterScope();
    void exitScope();

    bool define(const Symbol& sym);

    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrentScope(const std::string& name);

    std::size_t depth()const{return scopes_.size();}

private:
    std::vector<std::unordered_map<std::string,Symbol>> scopes_;
};

}
