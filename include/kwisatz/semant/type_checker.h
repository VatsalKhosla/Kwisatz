#pragma once

#include<string>
#include<unordered_set>

#include"kwisatz/ast/ast.h"
#include"kwisatz/semant/env.h"
#include"kwisatz/semant/type.h"
#include"kwisatz/util/source_location.h"

namespace kwisatz{

class TypeChecker{
public:
    explicit TypeChecker(TypeContext& tc);

    void check(Program& prog);
    bool hadError()const{return hadError_;}

private:
    TypeContext& tc_;
    Env env_;
    bool hadError_;
    int loopDepth_;
    std::vector<Type*> returnTypeStack_;

    Type* resolveTypeRef(const TypeRef& ref);

    Type* checkExpr(Expr& e);
    Type* checkVar(VarExpr& e);
    Type* checkBinary(BinaryExpr& e);
    Type* checkUnary(UnaryExpr& e);
    Type* checkCall(CallExpr& e);
    Type* checkIndex(IndexExpr& e);
    Type* checkField(FieldExpr& e);
    Type* checkNewStruct(NewStructExpr& e);
    Type* checkNewArray(NewArrayExpr& e);

    void checkStmt(Stmt& s);
    void checkVarDecl(VarDeclStmt& s);
    void checkAssign(AssignStmt& s);
    void checkExprStmt(ExprStmtNode& s);
    void checkBlock(BlockStmt& s);
    void checkIf(IfStmt& s);
    void checkWhile(WhileStmt& s);
    void checkBreak(BreakStmt& s);
    void checkReturn(ReturnStmt& s);
    void checkNestedFunc(NestedFuncStmt& s);

    void registerBuiltins();
    void registerStruct(StructDecl& s);
    void resolveStructFields(StructDecl& s);
    Type* checkBuiltinLength(CallExpr& e);

    void registerFunc(FuncDecl& f);
    void checkFuncBody(FuncDecl& f);

    bool isAssignable(Type* target,Type* value);
    bool isLvalueExpr(const Expr& e);

    void error(const SourceLocation& loc,const std::string& msg);
};

}
