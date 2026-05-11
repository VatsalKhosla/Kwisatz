#include"kwisatz/ir/translate.h"
#include"kwisatz/semant/type.h"

#include<utility>

namespace kwisatz{

TrExp TrExp::makeEx(std::unique_ptr<ir::Exp> e){
    TrExp t;
    t.kind=Kind::Ex;
    t.ex=std::move(e);
    return t;
}

TrExp TrExp::makeNx(std::unique_ptr<ir::Stm> s){
    TrExp t;
    t.kind=Kind::Nx;
    t.nx=std::move(s);
    return t;
}

TrExp TrExp::makeCx(CxBuilder b){
    TrExp t;
    t.kind=Kind::Cx;
    t.cx=std::move(b);
    return t;
}

IrTranslator::IrTranslator():currentFrame_(nullptr),inFunction_(false){}

void IrTranslator::enterScope(){scopes_.emplace_back();}
void IrTranslator::exitScope(){if(!scopes_.empty())scopes_.pop_back();}

void IrTranslator::define(const std::string& name,Access* a){
    if(scopes_.empty())return;
    scopes_.back()[name]=a;
}

Access* IrTranslator::lookup(const std::string& name){
    for(auto it=scopes_.rbegin();it!=scopes_.rend();++it){
        auto found=it->find(name);
        if(found!=it->end())return found->second;
    }
    return nullptr;
}

std::unique_ptr<ir::Stm> IrTranslator::seqAll(std::vector<std::unique_ptr<ir::Stm>> stmts){
    if(stmts.empty())return std::make_unique<ir::ExpStm>(std::make_unique<ir::ConstExp>(0));
    auto result=std::move(stmts[0]);
    for(std::size_t i=1;i<stmts.size();i++){
        result=std::make_unique<ir::SeqStm>(std::move(result),std::move(stmts[i]));
    }
    return result;
}
std::unique_ptr<ir::Exp> IrTranslator::unEx(TrExp tr){
    switch(tr.kind){
        case TrExp::Kind::Ex:
            return std::move(tr.ex);
        case TrExp::Kind::Nx:
            return std::make_unique<ir::ESeqExp>(
                std::move(tr.nx),
                std::make_unique<ir::ConstExp>(0));
        case TrExp::Kind::Cx:{
            Temp r=newTemp();
            Label t=newLabel();
            Label f=newLabel();
            std::vector<std::unique_ptr<ir::Stm>> parts;
            parts.push_back(std::make_unique<ir::MoveStm>(
                std::make_unique<ir::TempExp>(r),
                std::make_unique<ir::ConstExp>(1)));
            parts.push_back(tr.cx(t,f));
            parts.push_back(std::make_unique<ir::LabelStm>(f));
            parts.push_back(std::make_unique<ir::MoveStm>(
                std::make_unique<ir::TempExp>(r),
                std::make_unique<ir::ConstExp>(0)));
            parts.push_back(std::make_unique<ir::LabelStm>(t));
            return std::make_unique<ir::ESeqExp>(
                seqAll(std::move(parts)),
                std::make_unique<ir::TempExp>(r));
        }
    }
    return std::make_unique<ir::ConstExp>(0);
}

std::unique_ptr<ir::Stm> IrTranslator::unNx(TrExp tr){
    switch(tr.kind){
        case TrExp::Kind::Ex:
            return std::make_unique<ir::ExpStm>(std::move(tr.ex));
        case TrExp::Kind::Nx:
            return std::move(tr.nx);
        case TrExp::Kind::Cx:{
            Label l=newLabel();
            return std::make_unique<ir::SeqStm>(
                tr.cx(l,l),
                std::make_unique<ir::LabelStm>(l));
        }
    }
    return std::make_unique<ir::ExpStm>(std::make_unique<ir::ConstExp>(0));
}

TrExp::CxBuilder IrTranslator::unCx(TrExp tr){
    if(tr.kind==TrExp::Kind::Cx)return std::move(tr.cx);
    if(tr.kind==TrExp::Kind::Ex){
        auto holder=std::make_shared<std::unique_ptr<ir::Exp>>(std::move(tr.ex));
        return [holder](Label t,Label f){
            auto e=std::move(*holder);
            return std::make_unique<ir::CJumpStm>(
                ir::RelOp::Ne,std::move(e),
                std::make_unique<ir::ConstExp>(0),t,f);
        };
    }
    return [](Label,Label f){return std::make_unique<ir::JumpStm>(f);};
}
std::unique_ptr<ir::Exp> IrTranslator::accessToExp(Access* a){
    if(a->kind==AccessKind::InReg){
        Temp r=static_cast<InRegAccess*>(a)->reg;
        return std::make_unique<ir::TempExp>(r);
    }
    int offset=static_cast<InFrameAccess*>(a)->offset;
    auto fpExp=std::make_unique<ir::TempExp>(currentFrame_->fp());
    auto offExp=std::make_unique<ir::ConstExp>(offset);
    auto addr=std::make_unique<ir::BinOpExp>(
        ir::BinOp::Plus,std::move(fpExp),std::move(offExp));
    return std::make_unique<ir::MemExp>(std::move(addr));
}

void IrTranslator::translate(Program& prog){
    for(auto& d:prog.decls){
        if(d->kind==DeclKind::Func){
            translateFunc(static_cast<FuncDecl&>(*d));
        }
    }
}

void IrTranslator::translateFunc(FuncDecl& f){
    allFuncs_.push_back(&f);
    Frame* prev=currentFrame_;
    Label prevEnd=currentEndLabel_;
    bool prevIn=inFunction_;

    currentFrame_=f.frame.get();
    currentEndLabel_=newLabel();
    inFunction_=true;

    enterScope();
    for(const auto& p:f.params){
        define(p.name,p.access);
    }

    std::vector<std::unique_ptr<ir::Stm>> bodyStmts;
    if(f.body){
        for(auto& s:f.body->stmts){
            bodyStmts.push_back(translateStmt(*s));
        }
    }
    bodyStmts.push_back(std::make_unique<ir::LabelStm>(currentEndLabel_));
    f.ir_body=seqAll(std::move(bodyStmts));

    exitScope();

    currentFrame_=prev;
    currentEndLabel_=prevEnd;
    inFunction_=prevIn;
}
std::unique_ptr<ir::Exp> IrTranslator::translateExpr(Expr& e){
    return unEx(translateExprT(e));
}

std::unique_ptr<ir::Stm> IrTranslator::translateStmt(Stmt& s){
    switch(s.kind){
        case StmtKind::VarDecl:{
            auto& v=static_cast<VarDeclStmt&>(s);
            define(v.name,v.access);
            if(v.init){
                auto rhs=translateExpr(*v.init);
                return std::make_unique<ir::MoveStm>(accessToExp(v.access),std::move(rhs));
            }
            return std::make_unique<ir::MoveStm>(
                accessToExp(v.access),
                std::make_unique<ir::ConstExp>(0));
        }
        case StmtKind::Assign:{
            auto& a=static_cast<AssignStmt&>(s);
            auto lhs=translateExpr(*a.lhs);
            auto rhs=translateExpr(*a.rhs);
            return std::make_unique<ir::MoveStm>(std::move(lhs),std::move(rhs));
        }
        case StmtKind::ExprStmt:{
            auto& e=static_cast<ExprStmtNode&>(s);
            return std::make_unique<ir::ExpStm>(translateExpr(*e.expr));
        }
        case StmtKind::Block:{
            auto& b=static_cast<BlockStmt&>(s);
            enterScope();
            std::vector<std::unique_ptr<ir::Stm>> stmts;
            for(auto& sub:b.stmts)stmts.push_back(translateStmt(*sub));
            exitScope();
            return seqAll(std::move(stmts));
        }
        case StmtKind::If:{
            auto& i=static_cast<IfStmt&>(s);
            auto cx=unCx(translateExprT(*i.cond));
            Label tLab=newLabel();
            Label fLab=newLabel();
            Label joinLab=newLabel();
            auto thenStm=translateStmt(*i.thenBranch);
            std::unique_ptr<ir::Stm> elseStm;
            if(i.elseBranch)elseStm=translateStmt(*i.elseBranch);
            std::vector<std::unique_ptr<ir::Stm>> parts;
            parts.push_back(cx(tLab,fLab));
            parts.push_back(std::make_unique<ir::LabelStm>(tLab));
            parts.push_back(std::move(thenStm));
            parts.push_back(std::make_unique<ir::JumpStm>(joinLab));
            parts.push_back(std::make_unique<ir::LabelStm>(fLab));
            if(elseStm)parts.push_back(std::move(elseStm));
            parts.push_back(std::make_unique<ir::LabelStm>(joinLab));
            return seqAll(std::move(parts));
        }
        case StmtKind::While:{
            auto& w=static_cast<WhileStmt&>(s);
            Label testLab=newLabel();
            Label bodyLab=newLabel();
            Label doneLab=newLabel();
            auto cx=unCx(translateExprT(*w.cond));
            breakLabels_.push_back(doneLab);
            auto bodyStm=translateStmt(*w.body);
            breakLabels_.pop_back();
            std::vector<std::unique_ptr<ir::Stm>> parts;
            parts.push_back(std::make_unique<ir::LabelStm>(testLab));
            parts.push_back(cx(bodyLab,doneLab));
            parts.push_back(std::make_unique<ir::LabelStm>(bodyLab));
            parts.push_back(std::move(bodyStm));
            parts.push_back(std::make_unique<ir::JumpStm>(testLab));
            parts.push_back(std::make_unique<ir::LabelStm>(doneLab));
            return seqAll(std::move(parts));
        }
        case StmtKind::Break:{
            if(breakLabels_.empty()){
                return std::make_unique<ir::ExpStm>(std::make_unique<ir::ConstExp>(0));
            }
            return std::make_unique<ir::JumpStm>(breakLabels_.back());
        }
        case StmtKind::Return:{
            auto& r=static_cast<ReturnStmt&>(s);
            std::vector<std::unique_ptr<ir::Stm>> parts;
            if(r.value){
                parts.push_back(std::make_unique<ir::MoveStm>(
                    std::make_unique<ir::TempExp>(currentFrame_->rv()),
                    translateExpr(*r.value)));
            }
            parts.push_back(std::make_unique<ir::JumpStm>(currentEndLabel_));
            return seqAll(std::move(parts));
        }
        case StmtKind::NestedFunc:{
            auto& n=static_cast<NestedFuncStmt&>(s);
            if(n.decl)translateFunc(*n.decl);
            return std::make_unique<ir::ExpStm>(std::make_unique<ir::ConstExp>(0));
        }
    }
    return std::make_unique<ir::ExpStm>(std::make_unique<ir::ConstExp>(0));
}
TrExp IrTranslator::translateExprT(Expr& e){
    switch(e.kind){
        case ExprKind::IntLit:{
            auto& lit=static_cast<IntLitExpr&>(e);
            return TrExp::makeEx(std::make_unique<ir::ConstExp>(lit.value));
        }
        case ExprKind::BoolLit:{
            auto& lit=static_cast<BoolLitExpr&>(e);
            return TrExp::makeEx(std::make_unique<ir::ConstExp>(lit.value?1:0));
        }
        case ExprKind::NullLit:
            return TrExp::makeEx(std::make_unique<ir::ConstExp>(0));
        case ExprKind::Var:{
            auto& v=static_cast<VarExpr&>(e);
            if(Access* a=lookup(v.name))return TrExp::makeEx(accessToExp(a));
            return TrExp::makeEx(std::make_unique<ir::NameExp>(namedLabel(v.name)));
        }
        case ExprKind::Binary:
            return translateBinary(static_cast<BinaryExpr&>(e));
        case ExprKind::Unary:
            return translateUnary(static_cast<UnaryExpr&>(e));
        case ExprKind::StringLit:
            return translateStringLit(static_cast<StringLitExpr&>(e));
        case ExprKind::Call:
            return translateCall(static_cast<CallExpr&>(e));
        case ExprKind::Index:
            return translateIndex(static_cast<IndexExpr&>(e));
        case ExprKind::Field:
            return translateField(static_cast<FieldExpr&>(e));
        case ExprKind::NewStruct:
            return translateNewStruct(static_cast<NewStructExpr&>(e));
        case ExprKind::NewArray:
            return translateNewArray(static_cast<NewArrayExpr&>(e));
    }
    return TrExp::makeEx(std::make_unique<ir::ConstExp>(0));
}

TrExp IrTranslator::translateBinary(BinaryExpr& b){
    auto arith=[&](ir::BinOp op){
        auto l=translateExpr(*b.lhs);
        auto r=translateExpr(*b.rhs);
        return TrExp::makeEx(std::make_unique<ir::BinOpExp>(op,std::move(l),std::move(r)));
    };
    auto cmp=[&](ir::RelOp op){
        auto lExp=translateExpr(*b.lhs);
        auto rExp=translateExpr(*b.rhs);
        auto lShared=std::make_shared<std::unique_ptr<ir::Exp>>(std::move(lExp));
        auto rShared=std::make_shared<std::unique_ptr<ir::Exp>>(std::move(rExp));
        return TrExp::makeCx([op,lShared,rShared](Label t,Label f){
            return std::make_unique<ir::CJumpStm>(
                op,std::move(*lShared),std::move(*rShared),t,f);
        });
    };
    switch(b.op){
        case BinaryOp::Add:return arith(ir::BinOp::Plus);
        case BinaryOp::Sub:return arith(ir::BinOp::Minus);
        case BinaryOp::Mul:return arith(ir::BinOp::Mul);
        case BinaryOp::Div:return arith(ir::BinOp::Div);
        case BinaryOp::Mod:return arith(ir::BinOp::Mod);
        case BinaryOp::Eq:return cmp(ir::RelOp::Eq);
        case BinaryOp::Ne:return cmp(ir::RelOp::Ne);
        case BinaryOp::Lt:return cmp(ir::RelOp::Lt);
        case BinaryOp::Gt:return cmp(ir::RelOp::Gt);
        case BinaryOp::Le:return cmp(ir::RelOp::Le);
        case BinaryOp::Ge:return cmp(ir::RelOp::Ge);
        case BinaryOp::And:return translateAnd(b);
        case BinaryOp::Or:return translateOr(b);
    }
    return TrExp::makeEx(std::make_unique<ir::ConstExp>(0));
}

TrExp IrTranslator::translateAnd(BinaryExpr& b){
    auto leftCx=unCx(translateExprT(*b.lhs));
    auto rightCx=unCx(translateExprT(*b.rhs));
    auto leftShared=std::make_shared<TrExp::CxBuilder>(std::move(leftCx));
    auto rightShared=std::make_shared<TrExp::CxBuilder>(std::move(rightCx));
    return TrExp::makeCx([leftShared,rightShared](Label t,Label f){
        Label cont=newLabel();
        std::vector<std::unique_ptr<ir::Stm>> parts;
        parts.push_back((*leftShared)(cont,f));
        parts.push_back(std::make_unique<ir::LabelStm>(cont));
        parts.push_back((*rightShared)(t,f));
        std::unique_ptr<ir::Stm> result=std::move(parts[0]);
        for(std::size_t i=1;i<parts.size();i++){
            result=std::make_unique<ir::SeqStm>(std::move(result),std::move(parts[i]));
        }
        return result;
    });
}

TrExp IrTranslator::translateOr(BinaryExpr& b){
    auto leftCx=unCx(translateExprT(*b.lhs));
    auto rightCx=unCx(translateExprT(*b.rhs));
    auto leftShared=std::make_shared<TrExp::CxBuilder>(std::move(leftCx));
    auto rightShared=std::make_shared<TrExp::CxBuilder>(std::move(rightCx));
    return TrExp::makeCx([leftShared,rightShared](Label t,Label f){
        Label cont=newLabel();
        std::vector<std::unique_ptr<ir::Stm>> parts;
        parts.push_back((*leftShared)(t,cont));
        parts.push_back(std::make_unique<ir::LabelStm>(cont));
        parts.push_back((*rightShared)(t,f));
        std::unique_ptr<ir::Stm> result=std::move(parts[0]);
        for(std::size_t i=1;i<parts.size();i++){
            result=std::make_unique<ir::SeqStm>(std::move(result),std::move(parts[i]));
        }
        return result;
    });
}
TrExp IrTranslator::translateUnary(UnaryExpr& u){
    if(u.op==UnaryOp::Neg){
        auto operand=translateExpr(*u.operand);
        return TrExp::makeEx(std::make_unique<ir::BinOpExp>(
            ir::BinOp::Minus,
            std::make_unique<ir::ConstExp>(0),
            std::move(operand)));
    }
    if(u.op==UnaryOp::Not){
        auto inner=unCx(translateExprT(*u.operand));
        auto innerShared=std::make_shared<TrExp::CxBuilder>(std::move(inner));
        return TrExp::makeCx([innerShared](Label t,Label f){
            return (*innerShared)(f,t);
        });
    }
    return TrExp::makeEx(std::make_unique<ir::ConstExp>(0));
}


TrExp IrTranslator::translateStringLit(StringLitExpr& s){
    Label l=newLabel();
    strings_.push_back(StringFragment{l,s.value});
    return TrExp::makeEx(std::make_unique<ir::NameExp>(l));
}

TrExp IrTranslator::translateCall(CallExpr& c){
    if(c.callee->kind==ExprKind::Var){
        const auto& v=static_cast<const VarExpr&>(*c.callee);
        if(v.name=="length"&&c.args.size()==1){
            auto arrExp=translateExpr(*c.args[0]);
            return TrExp::makeEx(std::make_unique<ir::MemExp>(std::move(arrExp)));
        }
    }
    auto callee=translateExpr(*c.callee);
    std::vector<std::unique_ptr<ir::Exp>> args;
    for(auto& a:c.args)args.push_back(translateExpr(*a));
    return TrExp::makeEx(std::make_unique<ir::CallExp>(std::move(callee),std::move(args)));
}
TrExp IrTranslator::translateIndex(IndexExpr& i){
    auto arrExp=translateExpr(*i.arr);
    auto idxExp=translateExpr(*i.index);
    int ws=currentFrame_->wordSize();
    auto idxPlus1=std::make_unique<ir::BinOpExp>(
        ir::BinOp::Plus,
        std::move(idxExp),
        std::make_unique<ir::ConstExp>(1));
    auto byteOff=std::make_unique<ir::BinOpExp>(
        ir::BinOp::Mul,
        std::move(idxPlus1),
        std::make_unique<ir::ConstExp>(ws));
    auto addr=std::make_unique<ir::BinOpExp>(
        ir::BinOp::Plus,
        std::move(arrExp),
        std::move(byteOff));
    return TrExp::makeEx(std::make_unique<ir::MemExp>(std::move(addr)));
}

TrExp IrTranslator::translateField(FieldExpr& f){
    auto objExp=translateExpr(*f.obj);
    int offset=0;
    int ws=currentFrame_->wordSize();
    if(f.obj->type&&f.obj->type->kind==TypeKind::Struct){
        auto* st=static_cast<StructType*>(f.obj->type);
        for(const auto& field:st->fields){
            if(field.name==f.name)break;
            offset+=ws;
        }
    }
    auto addr=std::make_unique<ir::BinOpExp>(
        ir::BinOp::Plus,
        std::move(objExp),
        std::make_unique<ir::ConstExp>(offset));
    return TrExp::makeEx(std::make_unique<ir::MemExp>(std::move(addr)));
}
TrExp IrTranslator::translateNewStruct(NewStructExpr& n){
    int ws=currentFrame_->wordSize();
    int numFields=static_cast<int>(n.args.size());
    int size=numFields*ws;

    Temp ptr=newTemp();

    std::vector<std::unique_ptr<ir::Exp>> allocArgs;
    allocArgs.push_back(std::make_unique<ir::ConstExp>(size));
    auto allocCall=std::make_unique<ir::CallExp>(
        std::make_unique<ir::NameExp>(namedLabel("kw_alloc")),
        std::move(allocArgs));

    std::vector<std::unique_ptr<ir::Stm>> stmts;
    stmts.push_back(std::make_unique<ir::MoveStm>(
        std::make_unique<ir::TempExp>(ptr),
        std::move(allocCall)));

    int offset=0;
    for(auto& arg:n.args){
        auto argExp=translateExpr(*arg);
        auto fieldAddr=std::make_unique<ir::BinOpExp>(
            ir::BinOp::Plus,
            std::make_unique<ir::TempExp>(ptr),
            std::make_unique<ir::ConstExp>(offset));
        stmts.push_back(std::make_unique<ir::MoveStm>(
            std::make_unique<ir::MemExp>(std::move(fieldAddr)),
            std::move(argExp)));
        offset+=ws;
    }

    return TrExp::makeEx(std::make_unique<ir::ESeqExp>(
        seqAll(std::move(stmts)),
        std::make_unique<ir::TempExp>(ptr)));
}

TrExp IrTranslator::translateNewArray(NewArrayExpr& n){
    auto lenExp=translateExpr(*n.length);
    std::vector<std::unique_ptr<ir::Exp>> args;
    args.push_back(std::move(lenExp));
    return TrExp::makeEx(std::make_unique<ir::CallExp>(
        std::make_unique<ir::NameExp>(namedLabel("kw_array_alloc")),
        std::move(args)));
}

}