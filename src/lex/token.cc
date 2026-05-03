#include "kwisatz/lex/token.h"

namespace kwisatz{

    const char* tokenKindName(TokenKind k){
        switch(k){
            case TokenKind::IntLit:return "IntLit";
            case TokenKind::StringLit:return "StringLit";
            case TokenKind::Ident:return "Ident";
            case TokenKind::KwIf:return "KwIf";
            case TokenKind::KwElse:return "KwElse";
            case TokenKind::KwWhile:return "KwWhile";
            case TokenKind::KwBreak:return "KwBreak";
            case TokenKind::KwReturn:return "KwReturn";
            case TokenKind::KwStruct:return "KwStruct";
            case TokenKind::KwNew:return "KwNew";
            case TokenKind::KwNull:return "KwNull";
            case TokenKind::KwInt:return "KwInt";
            case TokenKind::KwBool:return "KwBool";
            case TokenKind::KwString:return "KwString";
            case TokenKind::KwVoid:return "KwVoid";
            case TokenKind::KwTrue:return "KwTrue";
            case TokenKind::KwFalse:return "KwFalse";
            case TokenKind::Plus:return "Plus";
            case TokenKind::Minus:return "Minus";
            case TokenKind::Star:return "Star";
            case TokenKind::Slash:return "Slash";
            case TokenKind::Percent:return "Percent";
            case TokenKind::EqEq:return "EqEq";
            case TokenKind::BangEq:return "BangEq";
            case TokenKind::Lt:return "Lt";
            case TokenKind::Gt:return "Gt";
            case TokenKind::LtEq:return "LtEq";
            case TokenKind::GtEq:return "GtEq";
            case TokenKind::AmpAmp:return "AmpAmp";
            case TokenKind::PipePipe:return "PipePipe";
            case TokenKind::Bang:return "Bang";
            case TokenKind::Eq:return "Eq";
            case TokenKind::LParen:return "LParen";
            case TokenKind::RParen:return "RParen";
            case TokenKind::LBrace:return "LBrace";
            case TokenKind::RBrace:return "RBrace";
            case TokenKind::LBracket:return "LBracket";
            case TokenKind::RBracket:return "RBracket";
            case TokenKind::Comma:return "Comma";
            case TokenKind::Semi:return "Semi";
            case TokenKind::Dot:return "Dot";
            case TokenKind::EndOfFile:return "EndOfFile";
            case TokenKind::Error:return "Error";
        }
        return "?";
    }
    
}