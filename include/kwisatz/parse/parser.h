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

    bool looksLikeTypePrefix()const;
    std::unique_ptr<Decl> parseTopDecl();
    std::unique_ptr<FuncDecl> parseFuncDecl();
    std::unique_ptr<StructDecl> parseStructDecl();
    std::vector<Param> parseParams();
    TypeRef parseTypeRef();

    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<Stmt> parseDeclStmt();
    std::unique_ptr<Stmt> parseIf();
    std::unique_ptr<Stmt> parseWhile();
    std::unique_ptr<Stmt> parseReturn();
    std::unique_ptr<Stmt> parseBreak();
    std::unique_ptr<Stmt> parseAssignOrExprStmt();

    std::unique_ptr<Expr> parseExpression();

};

}
