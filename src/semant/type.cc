#include"kwisatz/semant/type.h"

#include<utility>

namespace kwisatz{

TypeContext::TypeContext()
    :intType_(std::make_unique<IntType>()),
     boolType_(std::make_unique<BoolType>()),
     stringType_(std::make_unique<StringType>()),
     voidType_(std::make_unique<VoidType>()),
     nullType_(std::make_unique<NullType>()),
     errorType_(std::make_unique<ErrorType>()){}

Type* TypeContext::arrayOf(Type* elem){
    auto it=arrayCache_.find(elem);
    if(it!=arrayCache_.end())return it->second.get();
    auto arr=std::make_unique<ArrayType>(elem);
    Type* p=arr.get();
    arrayCache_[elem]=std::move(arr);
    return p;
}

StructType* TypeContext::getOrCreateStruct(const std::string& name){
    auto it=structs_.find(name);
    if(it!=structs_.end())return it->second.get();
    auto s=std::make_unique<StructType>(name);
    StructType* p=s.get();
    structs_[name]=std::move(s);
    return p;
}

StructType* TypeContext::findStruct(const std::string& name){
    auto it=structs_.find(name);
    if(it==structs_.end())return nullptr;
    return it->second.get();
}

Type* TypeContext::makeFunc(Type* ret,std::vector<Type*> params){
    auto f=std::make_unique<FuncType>(ret,std::move(params));
    Type* p=f.get();
    funcs_.push_back(std::move(f));
    return p;
}

std::string TypeContext::typeToString(Type* t){
    if(!t)return "<null type>";
    switch(t->kind){
        case TypeKind::Int:return "int";
        case TypeKind::Bool:return "bool";
        case TypeKind::String:return "string";
        case TypeKind::Void:return "void";
        case TypeKind::Null:return "null";
        case TypeKind::Error:return "<error>";
        case TypeKind::Array:{
            auto a=static_cast<ArrayType*>(t);
            return typeToString(a->elem)+"[]";
        }
        case TypeKind::Struct:{
            auto s=static_cast<StructType*>(t);
            return s->name;
        }
        case TypeKind::Func:{
            auto f=static_cast<FuncType*>(t);
            std::string r="(";
            for(std::size_t i=0;i<f->paramTypes.size();i++){
                if(i>0)r+=",";
                r+=typeToString(f->paramTypes[i]);
            }
            r+=")->";
            r+=typeToString(f->returnType);
            return r;
        }
    }
    return "?";
}

}
