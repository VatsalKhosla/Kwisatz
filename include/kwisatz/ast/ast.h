#pragma once

#include <memory>
#include <string>
#include <vector>

#include "kwisatz/util/source_location.h"

namespace kwisatz {

struct TypeRef{
    std::string name;
    int arrayDims;
    SourceLocation loc;
};
class Type; 

enum class ExprKind{
    IntLit,
    StringLit,
    BoolLit,
    NullLit,
    Var,
    Binary,
    Unary,
    Call,
    Index,
    Field,
    NewStruct,
    NewArray, 
};
enum class BinaryOp{
    Add,Sub,Mul,Div,Mod,
    Eq,Ne,Lt,Gt,Le,Ge,
    And,Or,
};
enum class UnaryOp{
    Neg,Not,
};

class Expr{
public:
    ExprKind kind;
    SourceLocation loc;
    Type* type=nullptr;
    Expr(ExprKind kind, SourceLocation loc):kind(kind),loc(loc){}
    virtual ~Expr()=default;
};

class IntLitExpr:public Expr{
public:
    long long value;
    IntLitExpr(SourceLocation l, long long v)
        :Expr(ExprKind::IntLit,l),value(v){}
};

class StringLitExpr:public Expr{
public:
    std::string value;
    StringLitExpr(SourceLocation l,std::string v)
        :Expr(ExprKind::StringLit,l),value(std::move(v)){}
};

class BoolLitExpr:public Expr{
public:
    bool value;
    BoolLitExpr(SourceLocation l,bool v)
        :Expr(ExprKind::BoolLit,l),value(v){}
};

class NullLitExpr:public Expr{
public:
    explicit NullLitExpr(SourceLocation l)
        :Expr(ExprKind::NullLit,l){}
};

class VarExpr:public Expr{
public:
    std::string name;
    VarExpr(SourceLocation l,std::string n)
        :Expr(ExprKind::Var,l),name(std::move(n)){}
};

class BinaryExpr:public Expr{
public:
    BinaryOp op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    BinaryExpr(SourceLocation l,BinaryOp o,std::unique_ptr<Expr> a,std::unique_ptr<Expr> b)
        :Expr(ExprKind::Binary,l),op(o),lhs(std::move(a)),rhs(std::move(b)){}
};

class UnaryExpr:public Expr{
public:
    UnaryOp op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(SourceLocation l,UnaryOp o,std::unique_ptr<Expr> e)
        :Expr(ExprKind::Unary,l),op(o),operand(std::move(e)){}
};

class CallExpr:public Expr{
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(SourceLocation l,std::unique_ptr<Expr> c,std::vector<std::unique_ptr<Expr>> a)
        :Expr(ExprKind::Call,l),callee(std::move(c)),args(std::move(a)){}
};

class IndexExpr:public Expr{
public:
    std::unique_ptr<Expr> arr;
    std::unique_ptr<Expr> index;
    IndexExpr(SourceLocation l,std::unique_ptr<Expr> a,std::unique_ptr<Expr> i)
        :Expr(ExprKind::Index,l),arr(std::move(a)),index(std::move(i)){}
};

class FieldExpr:public Expr{
public:
    std::unique_ptr<Expr> obj;
    std::string name;
    FieldExpr(SourceLocation l,std::unique_ptr<Expr> o,std::string n)
        :Expr(ExprKind::Field,l),obj(std::move(o)),name(std::move(n)){}
};

class NewStructExpr:public Expr{
public:
    std::string typeName;
    std::vector<std::unique_ptr<Expr>> args;
    NewStructExpr(SourceLocation l,std::string n,std::vector<std::unique_ptr<Expr>> a)
        :Expr(ExprKind::NewStruct,l),typeName(std::move(n)),args(std::move(a)){}
};

class NewArrayExpr:public Expr{
public:
    TypeRef elemType;
    std::unique_ptr<Expr> length;
    NewArrayExpr(SourceLocation l,TypeRef et,std::unique_ptr<Expr> len)
        :Expr(ExprKind::NewArray,l),elemType(std::move(et)),length(std::move(len)){}
};

enum class StmtKind{
    VarDecl,
    Assign,
    ExprStmt,
    Block,
    If,
    While,
    Break,
    Return,
    NestedFunc,
};

class Stmt{
public:
    StmtKind kind;
    SourceLocation loc;
    Stmt(StmtKind k, SourceLocation l):kind(k),loc(l){}
    virtual ~Stmt()=default;
};

class VarDeclStmt:public Stmt{
public:
    TypeRef type;
    std::string name;
    std::unique_ptr<Expr> init;
    VarDeclStmt(SourceLocation l,TypeRef t,std::string n,std::unique_ptr<Expr> i)
        :Stmt(StmtKind::VarDecl,l),type(std::move(t)),name(std::move(n)),init(std::move(i)){}
};

class AssignStmt:public Stmt{
public:
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    AssignStmt(SourceLocation l,std::unique_ptr<Expr> a,std::unique_ptr<Expr> b)
        :Stmt(StmtKind::Assign,l),lhs(std::move(a)),rhs(std::move(b)){}
};

class ExprStmtNode:public Stmt{
public:
    std::unique_ptr<Expr> expr;
    ExprStmtNode(SourceLocation l,std::unique_ptr<Expr> e)
        :Stmt(StmtKind::ExprStmt,l),expr(std::move(e)){}
};

class BlockStmt:public Stmt{
public:
    std::vector<std::unique_ptr<Stmt>> stmts;
    BlockStmt(SourceLocation l,std::vector<std::unique_ptr<Stmt>> s)
        :Stmt(StmtKind::Block,l),stmts(std::move(s)){}
};

class IfStmt:public Stmt{
public:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    IfStmt(SourceLocation l,std::unique_ptr<Expr> c,std::unique_ptr<Stmt> t,std::unique_ptr<Stmt> e)
        :Stmt(StmtKind::If,l),cond(std::move(c)),thenBranch(std::move(t)),elseBranch(std::move(e)){}
};

class WhileStmt:public Stmt{
public:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;
    WhileStmt(SourceLocation l,std::unique_ptr<Expr> c,std::unique_ptr<Stmt> b)
        :Stmt(StmtKind::While,l),cond(std::move(c)),body(std::move(b)){}
};

class BreakStmt:public Stmt{
public:
    explicit BreakStmt(SourceLocation l):Stmt(StmtKind::Break,l){}
};

class ReturnStmt:public Stmt{
public:
    std::unique_ptr<Expr> value;
    ReturnStmt(SourceLocation l,std::unique_ptr<Expr> v)
        :Stmt(StmtKind::Return,l),value(std::move(v)){}
};

enum class DeclKind{
    Func,
    Struct,
};

class Decl{
public:
    DeclKind kind;
    SourceLocation loc;
    Decl(DeclKind k,SourceLocation l):kind(k),loc(l){}
    virtual ~Decl()=default;
};

struct Param{
    TypeRef type;
    std::string name;
    SourceLocation loc;
};

struct Field{
    TypeRef type;
    std::string name;
    SourceLocation loc;
};

class FuncDecl:public Decl{
public:
    TypeRef returnType;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<BlockStmt> body;
    FuncDecl(SourceLocation l,TypeRef rt,std::string n,std::vector<Param> p,std::unique_ptr<BlockStmt> b)
        :Decl(DeclKind::Func,l),returnType(std::move(rt)),name(std::move(n)),params(std::move(p)),body(std::move(b)){}
};

class StructDecl:public Decl{
public:
    std::string name;
    std::vector<Field> fields;
    StructDecl(SourceLocation l,std::string n,std::vector<Field> f)
        :Decl(DeclKind::Struct,l),name(std::move(n)),fields(std::move(f)){}
};

class NestedFuncStmt:public Stmt{
public:
    std::unique_ptr<FuncDecl> decl;
    NestedFuncStmt(SourceLocation l,std::unique_ptr<FuncDecl> d)
        :Stmt(StmtKind::NestedFunc,l),decl(std::move(d)){}
};

struct Program{
    std::vector<std::unique_ptr<Decl>> decls;
};

}
