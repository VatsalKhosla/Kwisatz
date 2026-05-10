#pragma once

#include<memory>
#include<string>
#include<vector>

#include"kwisatz/frame/temp.h"

namespace kwisatz{

enum class AccessKind{
    InReg,
    InFrame,
};

class Access{
public:
    AccessKind kind;
    explicit Access(AccessKind k):kind(k){}
    virtual ~Access()=default;
    virtual std::string toString()const=0;
};

class InRegAccess:public Access{
public:
    Temp reg;
    explicit InRegAccess(Temp t):Access(AccessKind::InReg),reg(t){}
    std::string toString()const override{return reg.toString();}
};

class InFrameAccess:public Access{
public:
    int offset;
    explicit InFrameAccess(int o):Access(AccessKind::InFrame),offset(o){}
    std::string toString()const override{
        std::string sign=offset>=0?"+":"";
        return "[fp"+sign+std::to_string(offset)+"]";
    }
};

class Frame{
public:
    Label name;
    std::vector<std::unique_ptr<Access>> formals;
    std::vector<std::unique_ptr<Access>> locals;

    explicit Frame(Label n):name(std::move(n)){}
    virtual ~Frame()=default;

    virtual Access* allocLocal(bool escapes)=0;
    virtual int wordSize()const=0;
};

}
