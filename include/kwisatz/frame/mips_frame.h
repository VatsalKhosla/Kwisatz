#pragma once

#include<vector>

#include"kwisatz/frame/frame.h"

namespace kwisatz{

class MipsFrame:public Frame{
public:
    static constexpr int kWordSize=4;

    MipsFrame(Label name,const std::vector<bool>& formalEscapes);

    Access* allocLocal(bool escapes)override;
    int wordSize()const override{return kWordSize;}

private:
    int nextOffset_;
};

}
