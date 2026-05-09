#include"kwisatz/semant/env.h"

namespace kwisatz{

Env::Env(){
    scopes_.emplace_back();
}

void Env::enterScope(){
    scopes_.emplace_back();
}

void Env::exitScope(){
    if(scopes_.size()>1)scopes_.pop_back();
}

bool Env::define(const Symbol& sym){
    auto& top=scopes_.back();
    if(top.find(sym.name)!=top.end())return false;
    top[sym.name]=sym;
    return true;
}

Symbol* Env::lookup(const std::string& name){
    for(auto it=scopes_.rbegin();it!=scopes_.rend();++it){
        auto found=it->find(name);
        if(found!=it->end())return &found->second;
    }
    return nullptr;
}

Symbol* Env::lookupCurrentScope(const std::string& name){
    auto& top=scopes_.back();
    auto it=top.find(name);
    if(it==top.end())return nullptr;
    return &it->second;
}

}
