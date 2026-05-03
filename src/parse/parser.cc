#include"kwisatz/parse/parser.h"

#include<cstdio>
#include<utility>

namespace kwisatz{

Parser::Parser(Lexer& lex)
    :pos_(0),hadError_(false){
    for(;;){
        Token t=lex.next();
        bool isEnd=(t.kind==TokenKind::EndOfFile);
        tokens_.push_back(std::move(t));
        if(isEnd)break;
    }
}

const Token& Parser::peek(std::size_t offset)const{
    std::size_t idx=pos_+offset;
    if(idx>=tokens_.size())return tokens_.back();
    return tokens_[idx];
}

bool Parser::atEnd()const{
    return peek().kind==TokenKind::EndOfFile;
}

bool Parser::check(TokenKind k)const{
    return peek().kind==k;
}

bool Parser::match(TokenKind k){
    if(!check(k))return false;
    advance();
    return true;
}

Token Parser::advance(){
    Token t=tokens_[pos_];
    if(!atEnd())pos_++;
    return t;
}

Token Parser::expect(TokenKind k,const char* what){
    if(check(k))return advance();
    const Token& t=peek();
    std::string msg="expected ";
    msg+=what;
    msg+=" but got ";
    msg+=tokenKindName(t.kind);
    error(t,msg);
    return t;
}

void Parser::error(const Token& tok,const std::string& msg){
    hadError_=true;
    std::fprintf(stderr,"%s:%d:%d: error: %s\n",
        tok.loc.file.c_str(),
        tok.loc.line,
        tok.loc.column,
        msg.c_str());
}

Program Parser::parseProgram(){
    Program prog;
    return prog;
}

}
