#include"kwisatz/ir/printer.h"

#include<string>
#include<vector>

namespace kwisatz{

namespace{

const char* binOpName(ir::BinOp op){
    switch(op){
        case ir::BinOp::Plus:return "Plus";
        case ir::BinOp::Minus:return "Minus";
        case ir::BinOp::Mul:return "Mul";
        case ir::BinOp::Div:return "Div";
        case ir::BinOp::Mod:return "Mod";
        case ir::BinOp::And:return "And";
        case ir::BinOp::Or:return "Or";
        case ir::BinOp::Xor:return "Xor";
        case ir::BinOp::LShift:return "LShift";
        case ir::BinOp::RShift:return "RShift";
        case ir::BinOp::ARShift:return "ARShift";
    }
    return "?";
}

const char* relOpName(ir::RelOp op){
    switch(op){
        case ir::RelOp::Eq:return "Eq";
        case ir::RelOp::Ne:return "Ne";
        case ir::RelOp::Lt:return "Lt";
        case ir::RelOp::Gt:return "Gt";
        case ir::RelOp::Le:return "Le";
        case ir::RelOp::Ge:return "Ge";
        case ir::RelOp::Ult:return "Ult";
        case ir::RelOp::Ugt:return "Ugt";
        case ir::RelOp::Ule:return "Ule";
        case ir::RelOp::Uge:return "Uge";
    }
    return "?";
}

std::string escapeIrString(const std::string& s){
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

class IrPrinter{
public:
    explicit IrPrinter(std::ostream& out):out_(out),indent_(0){}

    void printProgram(const Program& prog,const IrTranslator& tr);

private:
    std::ostream& out_;
    int indent_;

    void line(const std::string& s);
    void enter(const std::string& s);
    void leave();

    void printFunc(const FuncDecl& f);
    void printStm(const ir::Stm& s);
    void printExp(const ir::Exp& e);

    void flattenSeq(const ir::Stm& s,std::vector<const ir::Stm*>& out);
};

void IrPrinter::line(const std::string& s){
    for(int i=0;i<indent_;i++)out_<<"  ";
    out_<<s<<"\n";
}
void IrPrinter::enter(const std::string& s){line(s);indent_++;}
void IrPrinter::leave(){indent_--;}
void IrPrinter::printExp(const ir::Exp& e){
    switch(e.kind){
        case ir::ExpKind::Const:{
            const auto& c=static_cast<const ir::ConstExp&>(e);
            line("Const "+std::to_string(c.value));
            break;
        }
        case ir::ExpKind::Name:{
            const auto& n=static_cast<const ir::NameExp&>(e);
            line("Name "+n.label.toString());
            break;
        }
        case ir::ExpKind::Temp:{
            const auto& t=static_cast<const ir::TempExp&>(e);
            line("Temp "+t.temp.toString());
            break;
        }
        case ir::ExpKind::BinOp:{
            const auto& b=static_cast<const ir::BinOpExp&>(e);
            enter(std::string("BinOp ")+binOpName(b.op));
            printExp(*b.left);
            printExp(*b.right);
            leave();
            break;
        }
        case ir::ExpKind::Mem:{
            const auto& m=static_cast<const ir::MemExp&>(e);
            enter("Mem");
            printExp(*m.addr);
            leave();
            break;
        }
        case ir::ExpKind::Call:{
            const auto& c=static_cast<const ir::CallExp&>(e);
            enter("Call");
            enter("callee:");
            printExp(*c.callee);
            leave();
            enter("args:");
            for(const auto& a:c.args)printExp(*a);
            leave();
            leave();
            break;
        }
        case ir::ExpKind::ESeq:{
            const auto& es=static_cast<const ir::ESeqExp&>(e);
            enter("ESeq");
            enter("stm:");
            printStm(*es.stm);
            leave();
            enter("exp:");
            printExp(*es.exp);
            leave();
            leave();
            break;
        }
    }
}
void IrPrinter::flattenSeq(const ir::Stm& s,std::vector<const ir::Stm*>& out){
    if(s.kind==ir::StmKind::Seq){
        const auto& seq=static_cast<const ir::SeqStm&>(s);
        if(seq.first)flattenSeq(*seq.first,out);
        if(seq.second)flattenSeq(*seq.second,out);
    }else{
        out.push_back(&s);
    }
}

void IrPrinter::printStm(const ir::Stm& s){
    if(s.kind==ir::StmKind::Seq){
        std::vector<const ir::Stm*> stmts;
        flattenSeq(s,stmts);
        for(auto* sub:stmts)printStm(*sub);
        return;
    }
    switch(s.kind){
        case ir::StmKind::Move:{
            const auto& m=static_cast<const ir::MoveStm&>(s);
            enter("Move");
            printExp(*m.dst);
            printExp(*m.src);
            leave();
            break;
        }
        case ir::StmKind::Exp:{
            const auto& es=static_cast<const ir::ExpStm&>(s);
            enter("Eval");
            printExp(*es.exp);
            leave();
            break;
        }
        case ir::StmKind::Jump:{
            const auto& j=static_cast<const ir::JumpStm&>(s);
            line("Jump "+j.target.toString());
            break;
        }
        case ir::StmKind::CJump:{
            const auto& cj=static_cast<const ir::CJumpStm&>(s);
            enter(std::string("CJump ")+relOpName(cj.op));
            printExp(*cj.left);
            printExp(*cj.right);
            line("true: "+cj.trueLabel.toString());
            line("false: "+cj.falseLabel.toString());
            leave();
            break;
        }
        case ir::StmKind::Label:{
            const auto& l=static_cast<const ir::LabelStm&>(s);
            line("Label "+l.label.toString());
            break;
        }
        case ir::StmKind::Seq:
            break;
    }
}

void IrPrinter::printFunc(const FuncDecl& f){
    out_<<"PROC "<<f.name<<":\n";
    indent_=1;
    if(f.ir_body)printStm(*f.ir_body);
    indent_=0;
    out_<<"\n";
}

void IrPrinter::printProgram(const Program& prog,const IrTranslator& tr){
    (void)prog;
    for(FuncDecl* f:tr.functions()){
        if(f)printFunc(*f);
    }
    for(const auto& s:tr.strings()){
        out_<<"STRING "<<s.label.toString()<<": "<<escapeIrString(s.value)<<"\n";
    }
}

}

void printIrProgram(const Program& prog,const IrTranslator& tr,std::ostream& out){
    IrPrinter p(out);
    p.printProgram(prog,tr);
}

}
