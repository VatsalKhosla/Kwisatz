#include"kwisatz/ast/printer.h"

#include<ostream>
#include<string>

namespace kwisatz{

namespace{

const char* binOpStr(BinaryOp op){
    switch(op){
        case BinaryOp::Add:return "+";
        case BinaryOp::Sub:return "-";
        case BinaryOp::Mul:return "*";
        case BinaryOp::Div:return "/";
        case BinaryOp::Mod:return "%";
        case BinaryOp::Eq:return "==";
        case BinaryOp::Ne:return "!=";
        case BinaryOp::Lt:return "<";
        case BinaryOp::Gt:return ">";
        case BinaryOp::Le:return "<=";
        case BinaryOp::Ge:return ">=";
        case BinaryOp::And:return "&&";
        case BinaryOp::Or:return "||";
    }
    return "?";
}

const char* unaryOpStr(UnaryOp op){
    switch(op){
        case UnaryOp::Neg:return "-";
        case UnaryOp::Not:return "!";
    }
    return "?";
}

std::string typeRefStr(const TypeRef& t){
    std::string s=t.name;
    for(int i=0;i<t.arrayDims;i++)s+="[]";
    return s;
}

std::string escapeStr(const std::string& s){
    std::string out="\"";
    for(char c:s){
        switch(c){
            case '\n':out+="\\n";break;
            case '\t':out+="\\t";break;
            case '\\':out+="\\\\";break;
            case '"':out+="\\\"";break;
            default:out+=c;
        }
    }
    out+="\"";
    return out;
}

class AstPrinter{
public:
    explicit AstPrinter(std::ostream& out):out_(out),indent_(0){}

    void print(const Program& p){
        line("Program");
        indent_++;
        for(const auto& d:p.decls)printDecl(*d);
        indent_--;
    }

private:
    std::ostream& out_;
    int indent_;

    void line(const std::string& s){
        for(int i=0;i<indent_;i++)out_<<"  ";
        out_<<s<<"\n";
    }
    void enter(const std::string& s){line(s);indent_++;}
    void leave(){indent_--;}

    void printDecl(const Decl& d);
    void printFuncDecl(const FuncDecl& f);
    void printStructDecl(const StructDecl& s);
    void printStmt(const Stmt& s);
    void printBlock(const BlockStmt& b);
    void printVarDecl(const VarDeclStmt& v);
    void printAssign(const AssignStmt& a);
    void printExprStmt(const ExprStmtNode& e);
    void printIf(const IfStmt& s);
    void printWhile(const WhileStmt& w);
    void printReturn(const ReturnStmt& r);
    void printNestedFunc(const NestedFuncStmt& n);
    void printExpr(const Expr& e);
};

void AstPrinter::printExpr(const Expr& e){
    switch(e.kind){
        case ExprKind::IntLit:
            line("IntLit "+std::to_string(static_cast<const IntLitExpr&>(e).value));
            break;
        case ExprKind::StringLit:
            line("StringLit "+escapeStr(static_cast<const StringLitExpr&>(e).value));
            break;
        case ExprKind::BoolLit:
            line(static_cast<const BoolLitExpr&>(e).value?"BoolLit true":"BoolLit false");
            break;
        case ExprKind::NullLit:
            line("Null");
            break;
        case ExprKind::Var:
            line("Var "+static_cast<const VarExpr&>(e).name);
            break;
        case ExprKind::Binary:{
            const auto& b=static_cast<const BinaryExpr&>(e);
            enter(std::string("Binary ")+binOpStr(b.op));
            printExpr(*b.lhs);
            printExpr(*b.rhs);
            leave();
            break;
        }
        case ExprKind::Unary:{
            const auto& u=static_cast<const UnaryExpr&>(e);
            enter(std::string("Unary ")+unaryOpStr(u.op));
            printExpr(*u.operand);
            leave();
            break;
        }
        case ExprKind::Call:{
            const auto& c=static_cast<const CallExpr&>(e);
            enter("Call");
            enter("callee:");
            printExpr(*c.callee);
            leave();
            enter("args:");
            for(const auto& a:c.args)printExpr(*a);
            leave();
            leave();
            break;
        }
        case ExprKind::Index:{
            const auto& i=static_cast<const IndexExpr&>(e);
            enter("Index");
            printExpr(*i.arr);
            printExpr(*i.index);
            leave();
            break;
        }
        case ExprKind::Field:{
            const auto& f=static_cast<const FieldExpr&>(e);
            enter("Field ."+f.name);
            printExpr(*f.obj);
            leave();
            break;
        }
        case ExprKind::NewStruct:{
            const auto& n=static_cast<const NewStructExpr&>(e);
            enter("NewStruct "+n.typeName);
            for(const auto& a:n.args)printExpr(*a);
            leave();
            break;
        }
        case ExprKind::NewArray:{
            const auto& n=static_cast<const NewArrayExpr&>(e);
            enter("NewArray "+typeRefStr(n.elemType));
            enter("length:");
            printExpr(*n.length);
            leave();
            leave();
            break;
        }
    }
}

void AstPrinter::printDecl(const Decl& d){
    if(d.kind==DeclKind::Func)printFuncDecl(static_cast<const FuncDecl&>(d));
    else printStructDecl(static_cast<const StructDecl&>(d));
}

void AstPrinter::printFuncDecl(const FuncDecl& f){
    enter("FuncDecl "+f.name+" -> "+typeRefStr(f.returnType));
    for(const auto& p:f.params){
        std::string s="Param "+p.name+": "+typeRefStr(p.type);
        if(p.escapes)s+=" (escapes)";
        line(s);
    }
    if(f.body)printBlock(*f.body);
    leave();
}

void AstPrinter::printStructDecl(const StructDecl& s){
    enter("StructDecl "+s.name);
    for(const auto& f:s.fields){
        line("Field "+f.name+": "+typeRefStr(f.type));
    }
    leave();
}

void AstPrinter::printStmt(const Stmt& s){
    switch(s.kind){
        case StmtKind::VarDecl:printVarDecl(static_cast<const VarDeclStmt&>(s));break;
        case StmtKind::Assign:printAssign(static_cast<const AssignStmt&>(s));break;
        case StmtKind::ExprStmt:printExprStmt(static_cast<const ExprStmtNode&>(s));break;
        case StmtKind::Block:printBlock(static_cast<const BlockStmt&>(s));break;
        case StmtKind::If:printIf(static_cast<const IfStmt&>(s));break;
        case StmtKind::While:printWhile(static_cast<const WhileStmt&>(s));break;
        case StmtKind::Break:line("Break");break;
        case StmtKind::Return:printReturn(static_cast<const ReturnStmt&>(s));break;
        case StmtKind::NestedFunc:printNestedFunc(static_cast<const NestedFuncStmt&>(s));break;
    }
}

void AstPrinter::printBlock(const BlockStmt& b){
    enter("Block");
    for(const auto& s:b.stmts)printStmt(*s);
    leave();
}

void AstPrinter::printVarDecl(const VarDeclStmt& v){
    std::string header="VarDecl "+v.name+": "+typeRefStr(v.type);
    if(v.escapes)header+=" (escapes)";
    enter(header);
    if(v.init){
        enter("init:");
        printExpr(*v.init);
        leave();
    }
    leave();
}

void AstPrinter::printAssign(const AssignStmt& a){
    enter("Assign");
    enter("lhs:");
    printExpr(*a.lhs);
    leave();
    enter("rhs:");
    printExpr(*a.rhs);
    leave();
    leave();
}

void AstPrinter::printExprStmt(const ExprStmtNode& e){
    enter("ExprStmt");
    printExpr(*e.expr);
    leave();
}

void AstPrinter::printIf(const IfStmt& s){
    enter("If");
    enter("cond:");
    printExpr(*s.cond);
    leave();
    enter("then:");
    printStmt(*s.thenBranch);
    leave();
    if(s.elseBranch){
        enter("else:");
        printStmt(*s.elseBranch);
        leave();
    }
    leave();
}

void AstPrinter::printWhile(const WhileStmt& w){
    enter("While");
    enter("cond:");
    printExpr(*w.cond);
    leave();
    enter("body:");
    printStmt(*w.body);
    leave();
    leave();
}

void AstPrinter::printReturn(const ReturnStmt& r){
    enter("Return");
    if(r.value)printExpr(*r.value);
    leave();
}

void AstPrinter::printNestedFunc(const NestedFuncStmt& n){
    if(n.decl)printFuncDecl(*n.decl);
}

}

void printProgram(const Program& prog,std::ostream& out){
    AstPrinter p(out);
    p.print(prog);
}

}
