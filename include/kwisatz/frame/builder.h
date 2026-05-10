#pragma once

#include"kwisatz/ast/ast.h"

namespace kwisatz{

class FrameBuilder{
public:
    void build(Program& prog);

private:
    void buildFunc(FuncDecl& f);
    void walkStmt(Stmt& s,Frame& frame);
    void walkExpr(Expr& e,Frame& frame);
};

}
