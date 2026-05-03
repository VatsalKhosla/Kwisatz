#pragma once

#include <string> 
#include "kwisatz/util/source_location.h"

namespace kwisatz {
    
enum class TokenKind{
    IntLit,
    StringLit,
    Ident,

    KwIf,
    KwElse,
    KwWhile,
    KwBreak,
    KwReturn,
    KwStruct,
    KwNew,
    KwNull,
    KwInt,
    KwBool,
    KwString,
    KwVoid,
    KwTrue,
    KwFalse,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    EqEq,
    BangEq,
    Lt,
    Gt,
    LtEq,
    GtEq,
    AmpAmp,
    PipePipe,
    Bang,
    Eq,

    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    Semi,
    Dot,

    EndOfFile,
    Error,
};

const char* tokenKindName(TokenKind k);

struct Token{
    TokenKind kind;
    std::string lexeme;
    SourceLocation loc;
};

}

