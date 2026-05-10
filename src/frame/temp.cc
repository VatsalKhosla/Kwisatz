#include"kwisatz/frame/temp.h"

#include<unordered_map>

namespace kwisatz{

namespace{
int nextTempId_=0;
int nextLabelId_=0;
std::unordered_map<std::string,Label> namedLabels_;
}

Temp newTemp(){
    return Temp{++nextTempId_};
}

Label newLabel(){
    return Label{++nextLabelId_,""};
}

Label namedLabel(const std::string& name){
    auto it=namedLabels_.find(name);
    if(it!=namedLabels_.end())return it->second;
    Label l{++nextLabelId_,name};
    namedLabels_[name]=l;
    return l;
}

std::string Temp::toString()const{
    return "t"+std::to_string(id);
}

std::string Label::toString()const{
    if(!name.empty())return name;
    return "L"+std::to_string(id);
}

}
