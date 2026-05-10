#include"kwisatz/semant/escape.h"

namespace kwisatz{

void EscapeAnalyzer::enterScope(){
    scopes_.emplace_back();
}

void EscapeAnalyzer::exitScope(){
    if(!scopes_.empty())scopes_.pop_back();
}

void EscapeAnalyzer::define(const std::string& name,bool* flag,int declDepth){
    if(scopes_.empty())return;
    scopes_.back()[name]=Entry{flag,declDepth};
}

EscapeAnalyzer::Entry* EscapeAnalyzer::lookup(const std::string& name){
    for(auto it=scopes_.rbegin();it!=scopes_.rend();++it){
        auto found=it->find(name);
        if(found!=it->end())return &found->second;
    }
    return nullptr;
}

void EscapeAnalyzer::analyze(Program& prog){
    depth_=0;
    enterScope();

    for(auto& d:prog.decls){
        if(d->kind==DeclKind::Func){
            auto& f=static_cast<FuncDecl&>(*d);
            (void)f;
        }
    }

    for(auto& d:prog.decls){
        if(d->kind==DeclKind::Func){
            analyzeFuncDecl(static_cast<FuncDecl&>(*d));
        }
    }

    exitScope();
}
void EscapeAnalyzer::analyzeFuncDecl(FuncDecl& f){
    depth_++;
    enterScope();

    for(auto& p:f.params){
        p.escapes=false;
        define(p.name,&p.escapes,depth_);
    }

    if(f.body)analyzeBlockBody(*f.body);

    exitScope();
    depth_--;
}

void EscapeAnalyzer::analyzeBlockBody(BlockStmt& b){
    for(auto& s:b.stmts)analyzeStmt(*s);
}

void EscapeAnalyzer::analyzeStmt(Stmt& s){
    switch(s.kind){
        case StmtKind::VarDecl:{
            auto& v=static_cast<VarDeclStmt&>(s);
            v.escapes=false;
            if(v.init)analyzeExpr(*v.init);
            define(v.name,&v.escapes,depth_);
            break;
        }
        case StmtKind::Assign:{
            auto& a=static_cast<AssignStmt&>(s);
            analyzeExpr(*a.lhs);
            analyzeExpr(*a.rhs);
            break;
        }
        case StmtKind::ExprStmt:{
            auto& e=static_cast<ExprStmtNode&>(s);
            analyzeExpr(*e.expr);
            break;
        }
        case StmtKind::Block:{
            auto& b=static_cast<BlockStmt&>(s);
            enterScope();
            for(auto& sub:b.stmts)analyzeStmt(*sub);
            exitScope();
            break;
        }
        case StmtKind::If:{
            auto& i=static_cast<IfStmt&>(s);
            analyzeExpr(*i.cond);
            analyzeStmt(*i.thenBranch);
            if(i.elseBranch)analyzeStmt(*i.elseBranch);
            break;
        }
        case StmtKind::While:{
            auto& w=static_cast<WhileStmt&>(s);
            analyzeExpr(*w.cond);
            analyzeStmt(*w.body);
            break;
        }
        case StmtKind::Break:break;
        case StmtKind::Return:{
            auto& r=static_cast<ReturnStmt&>(s);
            if(r.value)analyzeExpr(*r.value);
            break;
        }
        case StmtKind::NestedFunc:{
            auto& n=static_cast<NestedFuncStmt&>(s);
            if(n.decl)analyzeFuncDecl(*n.decl);
            break;
        }
    }
}
void EscapeAnalyzer::analyzeExpr(Expr& e){
    switch(e.kind){
        case ExprKind::IntLit:
        case ExprKind::StringLit:
        case ExprKind::BoolLit:
        case ExprKind::NullLit:
            break;

        case ExprKind::Var:{
            auto& v=static_cast<VarExpr&>(e);
            if(Entry* entry=lookup(v.name)){
                if(depth_>entry->declDepth){
                    *entry->escapesFlag=true;
                }
            }
            break;
        }

        case ExprKind::Binary:{
            auto& b=static_cast<BinaryExpr&>(e);
            analyzeExpr(*b.lhs);
            analyzeExpr(*b.rhs);
            break;
        }
        case ExprKind::Unary:{
            auto& u=static_cast<UnaryExpr&>(e);
            analyzeExpr(*u.operand);
            break;
        }
        case ExprKind::Call:{
            auto& c=static_cast<CallExpr&>(e);
            analyzeExpr(*c.callee);
            for(auto& a:c.args)analyzeExpr(*a);
            break;
        }
        case ExprKind::Index:{
            auto& i=static_cast<IndexExpr&>(e);
            analyzeExpr(*i.arr);
            analyzeExpr(*i.index);
            break;
        }
        case ExprKind::Field:{
            auto& f=static_cast<FieldExpr&>(e);
            analyzeExpr(*f.obj);
            break;
        }
        case ExprKind::NewStruct:{
            auto& n=static_cast<NewStructExpr&>(e);
            for(auto& a:n.args)analyzeExpr(*a);
            break;
        }
        case ExprKind::NewArray:{
            auto& n=static_cast<NewArrayExpr&>(e);
            analyzeExpr(*n.length);
            break;
        }
    }
}

}
