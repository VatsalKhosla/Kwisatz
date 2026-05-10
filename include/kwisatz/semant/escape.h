#pragma once

#include<string>
#include<unordered_map>
#include<vector>

#include"kwisatz/ast/ast.h"

namespace kwisatz{

class EscapeAnalyzer{
public:
    void analyze(Program& prog);

private:
    int depth_=0;

    struct Entry{
        bool* escapesFlag;
        int declDepth;
    };

    std::vector<std::unordered_map<std::string,Entry>> scopes_;

    void enterScope();
    void exitScope();
    void define(const std::string& name,bool* flag,int declDepth);
    Entry* lookup(const std::string& name);

    void analyzeFuncDecl(FuncDecl& f);
    void analyzeStmt(Stmt& s);
    void analyzeBlockBody(BlockStmt& b);
    void analyzeExpr(Expr& e);
};

}
