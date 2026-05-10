#pragma once

#include<functional>
#include<memory>
#include<string>
#include<unordered_map>
#include<vector>

#include"kwisatz/ast/ast.h"
#include"kwisatz/frame/frame.h"
#include"kwisatz/ir/tree.h"

namespace kwisatz{

class TrExp{
public:
    using CxBuilder=std::function<std::unique_ptr<ir::Stm>(Label,Label)>;
    enum class Kind{Ex,Nx,Cx};

    Kind kind;
    std::unique_ptr<ir::Exp> ex;
    std::unique_ptr<ir::Stm> nx;
    CxBuilder cx;

    static TrExp makeEx(std::unique_ptr<ir::Exp> e);
    static TrExp makeNx(std::unique_ptr<ir::Stm> s);
    static TrExp makeCx(CxBuilder b);
};

class IrTranslator{
public:
    IrTranslator();

    void translate(Program& prog);

    std::unique_ptr<ir::Exp> translateExpr(Expr& e);
    std::unique_ptr<ir::Stm> translateStmt(Stmt& s);

private:
    Frame* currentFrame_;
    Label currentEndLabel_;
    bool inFunction_;
    std::vector<Label> breakLabels_;
    std::vector<std::unordered_map<std::string,Access*>> scopes_;

    void enterScope();
    void exitScope();
    void define(const std::string& name,Access* a);
    Access* lookup(const std::string& name);

    void translateFunc(FuncDecl& f);

    TrExp translateExprT(Expr& e);
    TrExp translateBinary(BinaryExpr& b);
    TrExp translateAnd(BinaryExpr& b);
    TrExp translateOr(BinaryExpr& b);
    TrExp translateUnary(UnaryExpr& u);

    std::unique_ptr<ir::Exp> unEx(TrExp tr);
    std::unique_ptr<ir::Stm> unNx(TrExp tr);
    TrExp::CxBuilder unCx(TrExp tr);

    std::unique_ptr<ir::Exp> accessToExp(Access* a);
    std::unique_ptr<ir::Stm> seqAll(std::vector<std::unique_ptr<ir::Stm>> stmts);
};

}
