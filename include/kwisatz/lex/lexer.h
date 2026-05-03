#pragma once 

#include <string> 
#include <cstddef>

#include "kwisatz/util/source_location.h"
#include "kwisatz/lex/token.h"

namespace kwisatz {

class Lexer{
public:
    Lexer(std::string source, std::string filename);

    Token next();

private:
    bool atEnd() const;
    char peek(int offset=0) const;
    char advance();
    bool match(char expected);
    void skipWhitespaceAndComments();
    SourceLocation here() const;

    Token makeToken(TokenKind kind, SourceLocation loc,std::string lexeme);
    Token errorToken(SourceLocation start, std::string message);

    Token scanIdentOrKeyword(SourceLocation start);
    Token scanNumber(SourceLocation start);
    Token scanString(SourceLocation start);

    std::string source_;
    std::string filename_;
    std::size_t pos_;
    int line_;
    int col_;
};
}