#pragma once

#include<string>

namespace kwisatz{

struct Temp{
    int id;
    bool operator==(const Temp& o)const{return id==o.id;}
    bool operator!=(const Temp& o)const{return !(*this==o);}
    std::string toString()const;
};

struct Label{
    int id;
    std::string name;
    bool operator==(const Label& o)const{return id==o.id;}
    bool operator!=(const Label& o)const{return !(*this==o);}
    std::string toString()const;
};

Temp newTemp();
Label newLabel();
Label namedLabel(const std::string& name);

}
