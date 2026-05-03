#include "kwisatz/lex/lexer.h"

#include<unordered_map>
#include <utility>

namespace kwisatz{
namespace{

    bool isAlpha(char c){
        return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';
    }
    bool isDigit(char c){
        return c>='0'&&c<='9';
    }
    bool isAlnum(char c){
        return isAlpha(c)||isDigit(c);
    }
    TokenKind keywordKindOrIdent(const std::string& s){
        static const std::unordered_map<std::string,TokenKind> kw{
        {"if",TokenKind::KwIf},
        {"else",TokenKind::KwElse},
        {"while",TokenKind::KwWhile},
        {"break",TokenKind::KwBreak},
        {"return",TokenKind::KwReturn},
        {"struct",TokenKind::KwStruct},
        {"new",TokenKind::KwNew},
        {"null",TokenKind::KwNull},
        {"int",TokenKind::KwInt},
        {"bool",TokenKind::KwBool},
        {"string",TokenKind::KwString},
        {"void",TokenKind::KwVoid},
        {"true",TokenKind::KwTrue},
        {"false",TokenKind::KwFalse},
        };
        auto it=kw.find(s);
        if(it==kw.end()) return TokenKind::Ident;
        return it->second;
    }
}

Lexer::Lexer(std::string source, std::string filename)
    :source_(std::move(source)),
    filename_(std::move(filename)),
    pos_(0),
    line_(1),
    col_(1){}

bool Lexer::atEnd() const{
    return pos_>=source_.size();
}
char Lexer::peek(int offset) const{
    std::size_t p=pos_+static_cast<std::size_t>(offset);
    if(p>=source_.size()) return '\0';
    return source_[p];
}
char Lexer::advance(){
    char c=source_[pos_++];
    if(c=='\n'){
        line_++;
        col_=1;
    }else{
        col_++;
    }
    return c;
}
bool Lexer::match(char expected){
    if(atEnd())return false;
    if(source_[pos_]!=expected)return false;
    advance();
    return true;
}
SourceLocation Lexer::here() const{
    return SourceLocation{filename_,line_,col_};
}

void Lexer::skipWhitespaceAndComments(){
    while(!atEnd()){
        char c=peek();
        if(c==' '||c=='\t'||c=='\r'||c=='\n'){
            advance();
        }else if(c=='/'&&peek(1)=='/'){
            while(!atEnd()&&peek()!='\n') advance();
        }else if(c=='/'&&peek(1)=='*'){
            advance();advance();
            while(!atEnd()&&(peek()!='*'||peek(1)!='/')) advance();
            if(!atEnd()){
                advance();advance();
            }
        }else{
            return;
        }
    }
}
Token Lexer::makeToken(TokenKind kind, SourceLocation start,std::string lexeme){
    return Token{kind,std::move(lexeme),start};
}
Token Lexer::errorToken(SourceLocation start, std::string msg){
    return Token{TokenKind::Error,std::move(msg),start};
}
Token Lexer::scanIdentOrKeyword(SourceLocation start){
    std::size_t startPos=pos_-1;
    while(!atEnd()&&isAlnum(peek())) advance();
    std::string word=source_.substr(startPos,pos_-startPos);
    TokenKind kind=keywordKindOrIdent(word);
    return makeToken(kind,start,std::move(word));
}
Token Lexer::scanNumber(SourceLocation start){
    std::size_t startPos=pos_-1;
    while(!atEnd()&&isDigit(peek())) advance();
    std::string num=source_.substr(startPos,pos_-startPos);
    return makeToken(TokenKind::IntLit,start,std::move(num));
}
Token Lexer::scanString(SourceLocation start){
    std::string value;
    while(!atEnd()&&peek()!='"'){
        char c=peek();
        if(c=='\n')return errorToken(start,"Unterminated string");
        if(c=='\\'){
            advance();
            if(atEnd())return errorToken(start,"Unterminated string after escape");
            char esc=advance();
            switch(esc){
                case 'n':value+='\n';break;
                case 't':value+='\t';break;
                case '\\':value+='\\';break;
                case '"':value+='"';break;
                default:return errorToken(start,std::string("Invalid escape sequence \\")+esc);
            }
        }else{
            value+=advance();
        }
    }
    if(atEnd())return errorToken(start,"Unterminated string");
    advance();
    return makeToken(TokenKind::StringLit,start,std::move(value));
}
Token Lexer::next(){
    skipWhitespaceAndComments();
    SourceLocation start=here();
    if(atEnd())return makeToken(TokenKind::EndOfFile,start,"");
    char c=advance();

    if(isAlpha(c))return scanIdentOrKeyword(start);
    if(isDigit(c))return scanNumber(start);
    
    switch(c){
        case '"':return scanString(start);
        case '+':return makeToken(TokenKind::Plus,start,"+");
        case '-':return makeToken(TokenKind::Minus,start,"-");
        case '*':return makeToken(TokenKind::Star,start,"*");
        case '/':return makeToken(TokenKind::Slash,start,"/");
        case '%':return makeToken(TokenKind::Percent,start,"%");
        case '(':return makeToken(TokenKind::LParen,start,"(");
        case ')':return makeToken(TokenKind::RParen,start,")");
        case '{':return makeToken(TokenKind::LBrace,start,"{");
        case '}':return makeToken(TokenKind::RBrace,start,"}");
        case '[':return makeToken(TokenKind::LBracket,start,"[");
        case ']':return makeToken(TokenKind::RBracket,start,"]");
        case ',':return makeToken(TokenKind::Comma,start,",");
        case ';':return makeToken(TokenKind::Semi,start,";");
        case '.':return makeToken(TokenKind::Dot,start,".");
        case '=':
            if(match('='))return makeToken(TokenKind::EqEq,start,"==");
            return makeToken(TokenKind::Eq,start,"=");
        case '!':
            if(match('='))return makeToken(TokenKind::BangEq,start,"!=");
            return makeToken(TokenKind::Bang,start,"!");
        case '<':
            if(match('='))return makeToken(TokenKind::LtEq,start,"<=");
            return makeToken(TokenKind::Lt,start,"<");
        case '>':
            if(match('='))return makeToken(TokenKind::GtEq,start,">=");
            return makeToken(TokenKind::Gt,start,">");
        case '&':
            if(match('&'))return makeToken(TokenKind::AmpAmp,start,"&&");
            return errorToken(start,"unexpected '&' (did you mean '&&'?)");
        case '|':
            if(match('|'))return makeToken(TokenKind::PipePipe,start,"||");
            return errorToken(start,"unexpected '|' (did you mean '||'?)");
    }

    std::string msg="unexpected character: '";
    msg+=c;
    msg+="'";
    return errorToken(start,msg);

}
}
