#pragma once

#include<memory>
#include<string>
#include<unordered_map>
#include<vector>

namespace kwisatz{

enum class TypeKind{
    Int,
    Bool,
    String,
    Void,
    Null,
    Array,
    Struct,
    Func,
    Error,
};

class Type{
public:
    TypeKind kind;
    explicit Type(TypeKind k):kind(k){}
    virtual ~Type()=default;
};

class IntType:public Type{public:IntType():Type(TypeKind::Int){}};
class BoolType:public Type{public:BoolType():Type(TypeKind::Bool){}};
class StringType:public Type{public:StringType():Type(TypeKind::String){}};
class VoidType:public Type{public:VoidType():Type(TypeKind::Void){}};
class NullType:public Type{public:NullType():Type(TypeKind::Null){}};
class ErrorType:public Type{public:ErrorType():Type(TypeKind::Error){}};

class ArrayType:public Type{
public:
    Type* elem;
    explicit ArrayType(Type* e):Type(TypeKind::Array),elem(e){}
};

struct StructField{
    std::string name;
    Type* type;
};

class StructType:public Type{
public:
    std::string name;
    std::vector<StructField> fields;
    bool resolved;
    explicit StructType(std::string n)
        :Type(TypeKind::Struct),name(std::move(n)),resolved(false){}
};

class FuncType:public Type{
public:
    Type* returnType;
    std::vector<Type*> paramTypes;
    FuncType(Type* rt,std::vector<Type*> pts)
        :Type(TypeKind::Func),returnType(rt),paramTypes(std::move(pts)){}
};

class TypeContext{
public:
    TypeContext();

    Type* intType()const{return intType_.get();}
    Type* boolType()const{return boolType_.get();}
    Type* stringType()const{return stringType_.get();}
    Type* voidType()const{return voidType_.get();}
    Type* nullType()const{return nullType_.get();}
    Type* errorType()const{return errorType_.get();}

    Type* arrayOf(Type* elem);
    StructType* getOrCreateStruct(const std::string& name);
    StructType* findStruct(const std::string& name);
    Type* makeFunc(Type* ret,std::vector<Type*> params);

    std::string typeToString(Type* t);

private:
    std::unique_ptr<IntType> intType_;
    std::unique_ptr<BoolType> boolType_;
    std::unique_ptr<StringType> stringType_;
    std::unique_ptr<VoidType> voidType_;
    std::unique_ptr<NullType> nullType_;
    std::unique_ptr<ErrorType> errorType_;

    std::unordered_map<Type*,std::unique_ptr<ArrayType>> arrayCache_;
    std::unordered_map<std::string,std::unique_ptr<StructType>> structs_;
    std::vector<std::unique_ptr<FuncType>> funcs_;
};

}
