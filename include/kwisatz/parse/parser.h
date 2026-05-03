#pragma once

#include<cstddef>
#include<memory>
#include<string>
#include<vector>

#include"kwisatz/ast/ast.h"
#include"kwisatz/lex/lexer.h"
#include"kwisatz/lex/token.h"

namespace kwisatz{

class Parser{
public:
    explicit Parser(Lexer& lex);

    Program parseProgram();

    bool hadError()const{return hadError_;}

private:
    std::vector<Token> tokens_;
    std::size_t pos_;
    bool hadError_;

    const Token& peek(std::size_t offset=0)const;
    bool atEnd()const;
    bool check(TokenKind k)const;
    bool match(TokenKind k);
    Token advance();
    Token expect(TokenKind k,const char* what);

    void error(const Token& tok,const std::string& msg);
};

}
