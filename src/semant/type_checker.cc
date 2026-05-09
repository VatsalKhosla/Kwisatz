#include"kwisatz/semant/type_checker.h"

#include<cstdio>
#include<utility>

namespace kwisatz{

namespace{

bool isReferenceType(Type* t){
    if(!t)return false;
    switch(t->kind){
        case TypeKind::String:
        case TypeKind::Array:
        case TypeKind::Struct:
            return true;
        default:
            return false;
    }
}

}

TypeChecker::TypeChecker(TypeContext& tc)
    :tc_(tc),hadError_(false),loopDepth_(0){}

void TypeChecker::check(Program& prog){
    registerBuiltins();

    for(auto& d:prog.decls){
        if(d->kind==DeclKind::Struct){
            registerStruct(static_cast<StructDecl&>(*d));
        }
    }

    for(auto& d:prog.decls){
        if(d->kind==DeclKind::Struct){
            resolveStructFields(static_cast<StructDecl&>(*d));
        }else{
            registerFunc(static_cast<FuncDecl&>(*d));
        }
    }

    for(auto& d:prog.decls){
        if(d->kind==DeclKind::Func){
            checkFuncBody(static_cast<FuncDecl&>(*d));
        }
    }

    Symbol* mainSym=env_.lookup("main");
    if(!mainSym||mainSym->kind!=SymbolKind::Func){
        SourceLocation loc{"<program>",0,0};
        error(loc,"no 'int main()' function defined");
    }else{
        auto* ft=static_cast<FuncType*>(mainSym->type);
        if(ft->returnType!=tc_.intType()||!ft->paramTypes.empty()){
            error(mainSym->declLoc,"'main' must have signature 'int main()'");
        }
    }
}


void TypeChecker::error(const SourceLocation& loc,const std::string& msg){
    hadError_=true;
    std::fprintf(stderr,"%s:%d:%d: error: %s\n",
        loc.file.c_str(),loc.line,loc.column,msg.c_str());
}

Type* TypeChecker::resolveTypeRef(const TypeRef& ref){
    Type* base=nullptr;
    if(ref.name=="int")base=tc_.intType();
    else if(ref.name=="bool")base=tc_.boolType();
    else if(ref.name=="string")base=tc_.stringType();
    else if(ref.name=="void")base=tc_.voidType();
    else if(ref.name=="<error>")return tc_.errorType();
    else{
        StructType* s=tc_.findStruct(ref.name);
        if(!s){
            error(ref.loc,"unknown type '"+ref.name+"'");
            return tc_.errorType();
        }
        base=s;
    }
    for(int i=0;i<ref.arrayDims;i++)base=tc_.arrayOf(base);
    return base;
}
Type* TypeChecker::checkExpr(Expr& e){
    switch(e.kind){
        case ExprKind::IntLit:e.type=tc_.intType();break;
        case ExprKind::StringLit:e.type=tc_.stringType();break;
        case ExprKind::BoolLit:e.type=tc_.boolType();break;
        case ExprKind::NullLit:e.type=tc_.nullType();break;
        case ExprKind::Var:e.type=checkVar(static_cast<VarExpr&>(e));break;
        case ExprKind::Binary:e.type=checkBinary(static_cast<BinaryExpr&>(e));break;
        case ExprKind::Unary:e.type=checkUnary(static_cast<UnaryExpr&>(e));break;
        case ExprKind::Call:e.type=checkCall(static_cast<CallExpr&>(e));break;
        case ExprKind::Index:e.type=checkIndex(static_cast<IndexExpr&>(e));break;
        case ExprKind::Field:e.type=checkField(static_cast<FieldExpr&>(e));break;
        case ExprKind::NewStruct:e.type=checkNewStruct(static_cast<NewStructExpr&>(e));break;
        case ExprKind::NewArray:e.type=checkNewArray(static_cast<NewArrayExpr&>(e));break;
    }
    if(!e.type)e.type=tc_.errorType();
    return e.type;
}



Type* TypeChecker::checkVar(VarExpr& e){
    Symbol* sym=env_.lookup(e.name);
    if(!sym){
        error(e.loc,"undeclared name '"+e.name+"'");
        return tc_.errorType();
    }
    return sym->type;
}
Type* TypeChecker::checkBinary(BinaryExpr& e){
    Type* lhs=checkExpr(*e.lhs);
    Type* rhs=checkExpr(*e.rhs);
    if(lhs->kind==TypeKind::Error||rhs->kind==TypeKind::Error){
        return tc_.errorType();
    }
    Type* intT=tc_.intType();
    Type* boolT=tc_.boolType();
    switch(e.op){
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if(lhs!=intT||rhs!=intT){
                error(e.loc,"arithmetic requires int operands, got "+
                    tc_.typeToString(lhs)+" and "+tc_.typeToString(rhs));
                return tc_.errorType();
            }
            return intT;
        case BinaryOp::Lt:
        case BinaryOp::Gt:
        case BinaryOp::Le:
        case BinaryOp::Ge:
            if(lhs!=intT||rhs!=intT){
                error(e.loc,"order comparison requires int operands, got "+
                    tc_.typeToString(lhs)+" and "+tc_.typeToString(rhs));
                return tc_.errorType();
            }
            return boolT;
        case BinaryOp::Eq:
        case BinaryOp::Ne:{
            bool ok=(lhs==rhs)
                ||(lhs->kind==TypeKind::Null&&isReferenceType(rhs))
                ||(rhs->kind==TypeKind::Null&&isReferenceType(lhs));
            if(!ok){
                error(e.loc,"cannot compare "+
                    tc_.typeToString(lhs)+" and "+tc_.typeToString(rhs));
                return tc_.errorType();
            }
            return boolT;
        }
        case BinaryOp::And:
        case BinaryOp::Or:
            if(lhs!=boolT||rhs!=boolT){
                error(e.loc,"logical operator requires bool operands, got "+
                    tc_.typeToString(lhs)+" and "+tc_.typeToString(rhs));
                return tc_.errorType();
            }
            return boolT;
    }
    return tc_.errorType();
}
Type* TypeChecker::checkUnary(UnaryExpr& e){
    Type* op=checkExpr(*e.operand);
    if(op->kind==TypeKind::Error)return tc_.errorType();
    switch(e.op){
        case UnaryOp::Neg:
            if(op!=tc_.intType()){
                error(e.loc,"unary '-' requires int operand, got "+
                    tc_.typeToString(op));
                return tc_.errorType();
            }
            return tc_.intType();
        case UnaryOp::Not:
            if(op!=tc_.boolType()){
                error(e.loc,"unary '!' requires bool operand, got "+
                    tc_.typeToString(op));
                return tc_.errorType();
            }
            return tc_.boolType();
    }
    return tc_.errorType();
}
bool TypeChecker::isAssignable(Type* target,Type* value){
    if(!target||!value)return false;
    if(target==value)return true;
    if(target->kind==TypeKind::Error||value->kind==TypeKind::Error)return true;
    if(value->kind==TypeKind::Null){
        switch(target->kind){
            case TypeKind::String:
            case TypeKind::Array:
            case TypeKind::Struct:
                return true;
            default:
                return false;
        }
    }
    return false;
}

bool TypeChecker::isLvalueExpr(const Expr& e){
    return e.kind==ExprKind::Var||e.kind==ExprKind::Index||e.kind==ExprKind::Field;
}
Type* TypeChecker::checkCall(CallExpr& e){
    if(e.callee->kind==ExprKind::Var){
        const auto& v=static_cast<const VarExpr&>(*e.callee);
        Symbol* sym=env_.lookup(v.name);
        if(sym&&sym->kind==SymbolKind::BuiltinLength){
            return checkBuiltinLength(e);
        }
    }

    Type* callee=checkExpr(*e.callee);
    std::vector<Type*> argTypes;
    for(const auto& a:e.args)argTypes.push_back(checkExpr(*a));
    if(callee->kind==TypeKind::Error)return tc_.errorType();
    if(callee->kind!=TypeKind::Func){
        error(e.callee->loc,"called expression is not a function (type: "+
            tc_.typeToString(callee)+")");
        return tc_.errorType();
    }
    auto* ft=static_cast<FuncType*>(callee);
    if(argTypes.size()!=ft->paramTypes.size()){
        error(e.loc,"function expects "+std::to_string(ft->paramTypes.size())+
            " argument(s), got "+std::to_string(argTypes.size()));
        return ft->returnType;
    }
    for(std::size_t i=0;i<argTypes.size();i++){
        if(!isAssignable(ft->paramTypes[i],argTypes[i])){
            error(e.args[i]->loc,"argument "+std::to_string(i+1)+": expected "+
                tc_.typeToString(ft->paramTypes[i])+", got "+
                tc_.typeToString(argTypes[i]));
        }
    }
    return ft->returnType;
}

Type* TypeChecker::checkIndex(IndexExpr& e){
    Type* arrT=checkExpr(*e.arr);
    Type* idxT=checkExpr(*e.index);
    if(arrT->kind==TypeKind::Error||idxT->kind==TypeKind::Error)return tc_.errorType();
    if(arrT->kind!=TypeKind::Array){
        error(e.arr->loc,"indexed expression is not an array (type: "+
            tc_.typeToString(arrT)+")");
        return tc_.errorType();
    }
    if(idxT!=tc_.intType()){
        error(e.index->loc,"array index must be int, got "+tc_.typeToString(idxT));
    }
    return static_cast<ArrayType*>(arrT)->elem;
}

Type* TypeChecker::checkField(FieldExpr& e){
    Type* objT=checkExpr(*e.obj);
    if(objT->kind==TypeKind::Error)return tc_.errorType();
    if(objT->kind!=TypeKind::Struct){
        error(e.obj->loc,"field access on non-struct type "+tc_.typeToString(objT));
        return tc_.errorType();
    }
    auto* st=static_cast<StructType*>(objT);
    if(!st->resolved){
        error(e.obj->loc,"struct '"+st->name+"' has not been declared");
        return tc_.errorType();
    }
    for(const auto& f:st->fields){
        if(f.name==e.name)return f.type;
    }
    error(e.loc,"no field named '"+e.name+"' in struct '"+st->name+"'");
    return tc_.errorType();
}
Type* TypeChecker::checkNewStruct(NewStructExpr& e){
    StructType* st=tc_.findStruct(e.typeName);
    std::vector<Type*> argTypes;
    for(const auto& a:e.args)argTypes.push_back(checkExpr(*a));
    if(!st){
        error(e.loc,"unknown struct type '"+e.typeName+"'");
        return tc_.errorType();
    }
    if(!st->resolved){
        error(e.loc,"struct '"+e.typeName+"' has not been fully declared");
        return tc_.errorType();
    }
    if(argTypes.size()!=st->fields.size()){
        error(e.loc,"struct '"+e.typeName+"' has "+std::to_string(st->fields.size())+
            " field(s), got "+std::to_string(argTypes.size())+" arg(s)");
        return st;
    }
    for(std::size_t i=0;i<argTypes.size();i++){
        if(!isAssignable(st->fields[i].type,argTypes[i])){
            error(e.args[i]->loc,"field '"+st->fields[i].name+"' expects "+
                tc_.typeToString(st->fields[i].type)+", got "+
                tc_.typeToString(argTypes[i]));
        }
    }
    return st;
}

Type* TypeChecker::checkNewArray(NewArrayExpr& e){
    Type* elem=resolveTypeRef(e.elemType);
    Type* lenT=checkExpr(*e.length);
    if(elem->kind==TypeKind::Error||lenT->kind==TypeKind::Error)return tc_.errorType();
    if(elem->kind==TypeKind::Void){
        error(e.elemType.loc,"cannot create array of void");
        return tc_.errorType();
    }
    if(lenT!=tc_.intType()){
        error(e.length->loc,"array length must be int, got "+tc_.typeToString(lenT));
    }
    return tc_.arrayOf(elem);
}
void TypeChecker::checkStmt(Stmt& s){
    switch(s.kind){
        case StmtKind::VarDecl:checkVarDecl(static_cast<VarDeclStmt&>(s));break;
        case StmtKind::Assign:checkAssign(static_cast<AssignStmt&>(s));break;
        case StmtKind::ExprStmt:checkExprStmt(static_cast<ExprStmtNode&>(s));break;
        case StmtKind::Block:checkBlock(static_cast<BlockStmt&>(s));break;
        case StmtKind::If:checkIf(static_cast<IfStmt&>(s));break;
        case StmtKind::While:checkWhile(static_cast<WhileStmt&>(s));break;
        case StmtKind::Break:checkBreak(static_cast<BreakStmt&>(s));break;
        case StmtKind::Return:checkReturn(static_cast<ReturnStmt&>(s));break;
        case StmtKind::NestedFunc:checkNestedFunc(static_cast<NestedFuncStmt&>(s));break;
    }
}

void TypeChecker::checkVarDecl(VarDeclStmt& s){
    Type* declared=resolveTypeRef(s.type);
    if(declared->kind==TypeKind::Void){
        error(s.type.loc,"cannot declare variable of type 'void'");
        declared=tc_.errorType();
    }
    if(s.init){
        Type* initT=checkExpr(*s.init);
        if(declared->kind!=TypeKind::Error&&initT->kind!=TypeKind::Error){
            if(!isAssignable(declared,initT)){
                error(s.loc,"cannot initialize "+tc_.typeToString(declared)+
                    " from "+tc_.typeToString(initT));
            }
        }
    }
    Symbol sym{SymbolKind::Var,s.name,declared,s.loc};
    if(!env_.define(sym)){
        error(s.loc,"redeclaration of '"+s.name+"'");
    }
}

void TypeChecker::checkAssign(AssignStmt& s){
    Type* lhs=checkExpr(*s.lhs);
    Type* rhs=checkExpr(*s.rhs);
    if(!isLvalueExpr(*s.lhs)){
        error(s.lhs->loc,"left side of assignment is not an lvalue");
        return;
    }
    if(lhs->kind==TypeKind::Error||rhs->kind==TypeKind::Error)return;
    if(!isAssignable(lhs,rhs)){
        error(s.loc,"cannot assign "+tc_.typeToString(rhs)+" to "+tc_.typeToString(lhs));
    }
}

void TypeChecker::checkExprStmt(ExprStmtNode& s){
    checkExpr(*s.expr);
}

void TypeChecker::checkBlock(BlockStmt& s){
    env_.enterScope();
    for(const auto& stmt:s.stmts)checkStmt(*stmt);
    env_.exitScope();
}

void TypeChecker::checkIf(IfStmt& s){
    Type* cond=checkExpr(*s.cond);
    if(cond->kind!=TypeKind::Error&&cond!=tc_.boolType()){
        error(s.cond->loc,"if condition must be bool, got "+tc_.typeToString(cond));
    }
    checkStmt(*s.thenBranch);
    if(s.elseBranch)checkStmt(*s.elseBranch);
}

void TypeChecker::checkWhile(WhileStmt& s){
    Type* cond=checkExpr(*s.cond);
    if(cond->kind!=TypeKind::Error&&cond!=tc_.boolType()){
        error(s.cond->loc,"while condition must be bool, got "+tc_.typeToString(cond));
    }
    loopDepth_++;
    checkStmt(*s.body);
    loopDepth_--;
}

void TypeChecker::checkBreak(BreakStmt& s){
    if(loopDepth_==0){
        error(s.loc,"'break' outside of any loop");
    }
}

void TypeChecker::checkReturn(ReturnStmt& s){
    if(returnTypeStack_.empty()){
        error(s.loc,"'return' outside of any function");
        return;
    }
    Type* expected=returnTypeStack_.back();
    if(s.value){
        Type* got=checkExpr(*s.value);
        if(got->kind==TypeKind::Error||expected->kind==TypeKind::Error)return;
        if(expected->kind==TypeKind::Void){
            error(s.loc,"void function should return without a value");
            return;
        }
        if(!isAssignable(expected,got)){
            error(s.loc,"return type mismatch: expected "+tc_.typeToString(expected)+
                ", got "+tc_.typeToString(got));
        }
    }else{
        if(expected->kind!=TypeKind::Void){
            error(s.loc,"non-void function must return a value of type "+
                tc_.typeToString(expected));
        }
    }
}
void TypeChecker::registerFunc(FuncDecl& f){
    Type* ret=resolveTypeRef(f.returnType);
    std::vector<Type*> paramTypes;
    for(const auto& p:f.params){
        Type* pt=resolveTypeRef(p.type);
        if(pt->kind==TypeKind::Void){
            error(p.type.loc,"parameter cannot be of type 'void'");
            pt=tc_.errorType();
        }
        paramTypes.push_back(pt);
    }
    Type* funcType=tc_.makeFunc(ret,std::move(paramTypes));
    Symbol sym{SymbolKind::Func,f.name,funcType,f.loc};
    if(!env_.define(sym)){
        error(f.loc,"redeclaration of '"+f.name+"'");
    }
}

void TypeChecker::checkFuncBody(FuncDecl& f){
    Type* ret=resolveTypeRef(f.returnType);
    int savedLoopDepth=loopDepth_;
    loopDepth_=0;

    env_.enterScope();
    for(const auto& p:f.params){
        Type* pt=resolveTypeRef(p.type);
        if(pt->kind==TypeKind::Void)pt=tc_.errorType();
        Symbol sym{SymbolKind::Param,p.name,pt,p.loc};
        if(!env_.define(sym)){
            error(p.loc,"duplicate parameter '"+p.name+"'");
        }
    }

    returnTypeStack_.push_back(ret);
    if(f.body){
        for(const auto& s:f.body->stmts)checkStmt(*s);
    }
    returnTypeStack_.pop_back();

    env_.exitScope();
    loopDepth_=savedLoopDepth;
}

void TypeChecker::checkNestedFunc(NestedFuncStmt& s){
    if(!s.decl)return;
    registerFunc(*s.decl);
    checkFuncBody(*s.decl);
}

void TypeChecker::registerBuiltins(){
    SourceLocation builtinLoc{"<builtin>",0,0};

    {
        Symbol s;
        s.kind=SymbolKind::Func;
        s.name="print_int";
        s.type=tc_.makeFunc(tc_.voidType(),{tc_.intType()});
        s.declLoc=builtinLoc;
        env_.define(s);
    }
    {
        Symbol s;
        s.kind=SymbolKind::Func;
        s.name="print_str";
        s.type=tc_.makeFunc(tc_.voidType(),{tc_.stringType()});
        s.declLoc=builtinLoc;
        env_.define(s);
    }
    {
        Symbol s;
        s.kind=SymbolKind::Func;
        s.name="concat";
        s.type=tc_.makeFunc(tc_.stringType(),{tc_.stringType(),tc_.stringType()});
        s.declLoc=builtinLoc;
        env_.define(s);
    }
    {
        Symbol s;
        s.kind=SymbolKind::BuiltinLength;
        s.name="length";
        s.type=tc_.makeFunc(tc_.intType(),{});
        s.declLoc=builtinLoc;
        env_.define(s);
    }
}
void TypeChecker::registerStruct(StructDecl& s){
    if(env_.lookupCurrentScope(s.name)){
        error(s.loc,"redeclaration of '"+s.name+"'");
        return;
    }
    StructType* st=tc_.getOrCreateStruct(s.name);
    Symbol sym;
    sym.kind=SymbolKind::Struct;
    sym.name=s.name;
    sym.type=st;
    sym.declLoc=s.loc;
    env_.define(sym);
}

void TypeChecker::resolveStructFields(StructDecl& s){
    StructType* st=tc_.getOrCreateStruct(s.name);
    if(st->resolved)return;
    std::unordered_set<std::string> seen;
    for(const auto& f:s.fields){
        Type* ft=resolveTypeRef(f.type);
        if(ft->kind==TypeKind::Void){
            error(f.type.loc,"field '"+f.name+"' cannot be of type 'void'");
            ft=tc_.errorType();
        }
        if(seen.count(f.name)){
            error(f.loc,"duplicate field '"+f.name+"' in struct '"+s.name+"'");
            continue;
        }
        seen.insert(f.name);
        st->fields.push_back(StructField{f.name,ft});
    }
    st->resolved=true;
}

Type* TypeChecker::checkBuiltinLength(CallExpr& e){
    if(e.callee->kind==ExprKind::Var){
        Symbol* sym=env_.lookup(static_cast<VarExpr&>(*e.callee).name);
        if(sym)e.callee->type=sym->type;
    }
    if(e.args.size()!=1){
        error(e.loc,"length() takes exactly 1 argument, got "+
            std::to_string(e.args.size()));
        for(auto& a:e.args)checkExpr(*a);
        return tc_.intType();
    }
    Type* argT=checkExpr(*e.args[0]);
    if(argT->kind==TypeKind::Error)return tc_.intType();
    if(argT->kind!=TypeKind::Array){
        error(e.args[0]->loc,"length() requires an array argument, got "+
            tc_.typeToString(argT));
    }
    return tc_.intType();
}


}
