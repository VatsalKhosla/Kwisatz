#include"kwisatz/frame/builder.h"

#include"kwisatz/frame/mips_frame.h"

namespace kwisatz{

void FrameBuilder::build(Program& prog){
    for(auto& d:prog.decls){
        if(d->kind==DeclKind::Func){
            buildFunc(static_cast<FuncDecl&>(*d));
        }
    }
}

void FrameBuilder::buildFunc(FuncDecl& f){
    std::vector<bool> escapes;
    for(const auto& p:f.params)escapes.push_back(p.escapes);
    auto frame=std::make_unique<MipsFrame>(namedLabel(f.name),escapes);

    for(std::size_t i=0;i<f.params.size();i++){
        f.params[i].access=frame->formals[i].get();
    }

    if(f.body){
        for(auto& s:f.body->stmts)walkStmt(*s,*frame);
    }

    f.frame=std::move(frame);
}

void FrameBuilder::walkStmt(Stmt& s,Frame& frame){
    switch(s.kind){
        case StmtKind::VarDecl:{
            auto& v=static_cast<VarDeclStmt&>(s);
            v.access=frame.allocLocal(v.escapes);
            if(v.init)walkExpr(*v.init,frame);
            break;
        }
        case StmtKind::Assign:{
            auto& a=static_cast<AssignStmt&>(s);
            walkExpr(*a.lhs,frame);
            walkExpr(*a.rhs,frame);
            break;
        }
        case StmtKind::ExprStmt:{
            auto& e=static_cast<ExprStmtNode&>(s);
            walkExpr(*e.expr,frame);
            break;
        }
        case StmtKind::Block:{
            auto& b=static_cast<BlockStmt&>(s);
            for(auto& sub:b.stmts)walkStmt(*sub,frame);
            break;
        }
        case StmtKind::If:{
            auto& i=static_cast<IfStmt&>(s);
            walkExpr(*i.cond,frame);
            walkStmt(*i.thenBranch,frame);
            if(i.elseBranch)walkStmt(*i.elseBranch,frame);
            break;
        }
        case StmtKind::While:{
            auto& w=static_cast<WhileStmt&>(s);
            walkExpr(*w.cond,frame);
            walkStmt(*w.body,frame);
            break;
        }
        case StmtKind::Break:break;
        case StmtKind::Return:{
            auto& r=static_cast<ReturnStmt&>(s);
            if(r.value)walkExpr(*r.value,frame);
            break;
        }
        case StmtKind::NestedFunc:{
            auto& n=static_cast<NestedFuncStmt&>(s);
            if(n.decl)buildFunc(*n.decl);
            break;
        }
    }
}

void FrameBuilder::walkExpr(Expr& e,Frame& frame){
    switch(e.kind){
        case ExprKind::IntLit:
        case ExprKind::StringLit:
        case ExprKind::BoolLit:
        case ExprKind::NullLit:
        case ExprKind::Var:
            break;
        case ExprKind::Binary:{
            auto& b=static_cast<BinaryExpr&>(e);
            walkExpr(*b.lhs,frame);
            walkExpr(*b.rhs,frame);
            break;
        }
        case ExprKind::Unary:{
            auto& u=static_cast<UnaryExpr&>(e);
            walkExpr(*u.operand,frame);
            break;
        }
        case ExprKind::Call:{
            auto& c=static_cast<CallExpr&>(e);
            walkExpr(*c.callee,frame);
            for(auto& a:c.args)walkExpr(*a,frame);
            break;
        }
        case ExprKind::Index:{
            auto& i=static_cast<IndexExpr&>(e);
            walkExpr(*i.arr,frame);
            walkExpr(*i.index,frame);
            break;
        }
        case ExprKind::Field:{
            auto& f=static_cast<FieldExpr&>(e);
            walkExpr(*f.obj,frame);
            break;
        }
        case ExprKind::NewStruct:{
            auto& n=static_cast<NewStructExpr&>(e);
            for(auto& a:n.args)walkExpr(*a,frame);
            break;
        }
        case ExprKind::NewArray:{
            auto& n=static_cast<NewArrayExpr&>(e);
            walkExpr(*n.length,frame);
            break;
        }
    }
}

}
