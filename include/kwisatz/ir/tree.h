#pragma once

#include<memory>
#include<vector>

#include"kwisatz/frame/temp.h"

namespace kwisatz::ir{

enum class BinOp{
    Plus,Minus,Mul,Div,Mod,
    And,Or,Xor,
    LShift,RShift,ARShift,
};

enum class RelOp{
    Eq,Ne,Lt,Gt,Le,Ge,
    Ult,Ugt,Ule,Uge,
};

class Stm;

enum class ExpKind{
    Const,Name,Temp,BinOp,Mem,Call,ESeq,
};

class Exp{
public:
    ExpKind kind;
    explicit Exp(ExpKind k):kind(k){}
    virtual ~Exp()=default;
};

class ConstExp:public Exp{
public:
    long long value;
    explicit ConstExp(long long v):Exp(ExpKind::Const),value(v){}
};

class NameExp:public Exp{
public:
    kwisatz::Label label;
    explicit NameExp(kwisatz::Label l):Exp(ExpKind::Name),label(std::move(l)){}
};

class TempExp:public Exp{
public:
    kwisatz::Temp temp;
    explicit TempExp(kwisatz::Temp t):Exp(ExpKind::Temp),temp(t){}
};

class BinOpExp:public Exp{
public:
    BinOp op;
    std::unique_ptr<Exp> left;
    std::unique_ptr<Exp> right;
    BinOpExp(BinOp o,std::unique_ptr<Exp> l,std::unique_ptr<Exp> r)
        :Exp(ExpKind::BinOp),op(o),left(std::move(l)),right(std::move(r)){}
};

class MemExp:public Exp{
public:
    std::unique_ptr<Exp> addr;
    explicit MemExp(std::unique_ptr<Exp> a):Exp(ExpKind::Mem),addr(std::move(a)){}
};

class CallExp:public Exp{
public:
    std::unique_ptr<Exp> callee;
    std::vector<std::unique_ptr<Exp>> args;
    CallExp(std::unique_ptr<Exp> c,std::vector<std::unique_ptr<Exp>> a)
        :Exp(ExpKind::Call),callee(std::move(c)),args(std::move(a)){}
};

class ESeqExp:public Exp{
public:
    std::unique_ptr<Stm> stm;
    std::unique_ptr<Exp> exp;
    ESeqExp(std::unique_ptr<Stm> s,std::unique_ptr<Exp> e)
        :Exp(ExpKind::ESeq),stm(std::move(s)),exp(std::move(e)){}
};

enum class StmKind{
    Move,Exp,Jump,CJump,Seq,Label,
};

class Stm{
public:
    StmKind kind;
    explicit Stm(StmKind k):kind(k){}
    virtual ~Stm()=default;
};

class MoveStm:public Stm{
public:
    std::unique_ptr<Exp> dst;
    std::unique_ptr<Exp> src;
    MoveStm(std::unique_ptr<Exp> d,std::unique_ptr<Exp> s)
        :Stm(StmKind::Move),dst(std::move(d)),src(std::move(s)){}
};

class ExpStm:public Stm{
public:
    std::unique_ptr<Exp> exp;
    explicit ExpStm(std::unique_ptr<Exp> e):Stm(StmKind::Exp),exp(std::move(e)){}
};

class JumpStm:public Stm{
public:
    kwisatz::Label target;
    explicit JumpStm(kwisatz::Label t):Stm(StmKind::Jump),target(std::move(t)){}
};

class CJumpStm:public Stm{
public:
    RelOp op;
    std::unique_ptr<Exp> left;
    std::unique_ptr<Exp> right;
    kwisatz::Label trueLabel;
    kwisatz::Label falseLabel;
    CJumpStm(RelOp o,std::unique_ptr<Exp> l,std::unique_ptr<Exp> r,
             kwisatz::Label t,kwisatz::Label f)
        :Stm(StmKind::CJump),op(o),left(std::move(l)),right(std::move(r)),
         trueLabel(std::move(t)),falseLabel(std::move(f)){}
};

class SeqStm:public Stm{
public:
    std::unique_ptr<Stm> first;
    std::unique_ptr<Stm> second;
    SeqStm(std::unique_ptr<Stm> a,std::unique_ptr<Stm> b)
        :Stm(StmKind::Seq),first(std::move(a)),second(std::move(b)){}
};

class LabelStm:public Stm{
public:
    kwisatz::Label label;
    explicit LabelStm(kwisatz::Label l):Stm(StmKind::Label),label(std::move(l)){}
};

}
