#include"kwisatz/frame/mips_frame.h"

namespace kwisatz{

MipsFrame::MipsFrame(Label name,const std::vector<bool>& formalEscapes)
    :Frame(std::move(name)),nextOffset_(0){
    for(bool escapes:formalEscapes){
        if(escapes){
            nextOffset_-=kWordSize;
            formals.push_back(std::make_unique<InFrameAccess>(nextOffset_));
        }else{
            formals.push_back(std::make_unique<InRegAccess>(newTemp()));
        }
    }
}

Access* MipsFrame::allocLocal(bool escapes){
    std::unique_ptr<Access> a;
    if(escapes){
        nextOffset_-=kWordSize;
        a=std::make_unique<InFrameAccess>(nextOffset_);
    }else{
        a=std::make_unique<InRegAccess>(newTemp());
    }
    Access* p=a.get();
    locals.push_back(std::move(a));
    return p;
}
Temp MipsFrame::fp()const{
    static const Temp f=newTemp();
    return f;
}
Temp MipsFrame::rv()const{
    static const Temp r=newTemp();
    return r;
}

}
