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
    while(!atEnd()){
         std::size_t before=pos_;
        std::unique_ptr<Decl> d=parseTopDecl();
        if(d)prog.decls.push_back(std::move(d));
        if(pos_==before)advance();
    }
    return prog;
}

std::unique_ptr<Decl> Parser::parseTopDecl(){
    if(check(TokenKind::KwStruct))return parseStructDecl();
    return parseFuncDecl();
}

std::unique_ptr<StructDecl> Parser::parseStructDecl(){
    SourceLocation loc=peek().loc;
    expect(TokenKind::KwStruct,"'struct'");
    Token nameTok=expect(TokenKind::Ident,"struct name");
    expect(TokenKind::LBrace,"'{' after struct name");
    std::vector<Field> fields;
    while(!check(TokenKind::RBrace)&&!atEnd()){
        std::size_t before=pos_;
        SourceLocation fieldLoc=peek().loc;
        TypeRef ty=parseTypeRef();
        Token fname=expect(TokenKind::Ident,"field name");
        expect(TokenKind::Semi,"';' after field");
        fields.push_back(Field{std::move(ty),fname.lexeme,fieldLoc});
        if(pos_==before)advance();
    }
    expect(TokenKind::RBrace,"'}'");
    return std::make_unique<StructDecl>(loc,nameTok.lexeme,std::move(fields));
}

std::unique_ptr<FuncDecl> Parser::parseFuncDecl(){
    SourceLocation loc=peek().loc;
    TypeRef returnType=parseTypeRef();
    Token nameTok=expect(TokenKind::Ident,"function name");
    expect(TokenKind::LParen,"'(' after function name");
    std::vector<Param> params=parseParams();
    expect(TokenKind::RParen,"')' after parameters");
    std::unique_ptr<BlockStmt> body=parseBlock();
    return std::make_unique<FuncDecl>(loc,std::move(returnType),nameTok.lexeme,std::move(params),std::move(body));
}

std::vector<Param> Parser::parseParams(){
    std::vector<Param> params;
    if(check(TokenKind::RParen))return params;
    for(;;){
        SourceLocation loc=peek().loc;
        TypeRef ty=parseTypeRef();
        Token n=expect(TokenKind::Ident,"parameter name");
        params.push_back(Param{std::move(ty),n.lexeme,loc});
        if(!match(TokenKind::Comma))break;
    }
    return params;
}

TypeRef Parser::parseTypeRef(){
    SourceLocation loc=peek().loc;
    std::string name;
    if(match(TokenKind::KwInt))name="int";
    else if(match(TokenKind::KwBool))name="bool";
    else if(match(TokenKind::KwString))name="string";
    else if(match(TokenKind::KwVoid))name="void";
    else if(check(TokenKind::Ident))name=advance().lexeme;
    else{
        error(peek(),"expected type");
        return TypeRef{"<error>",0,loc};
    }
    int dims=0;
    while(check(TokenKind::LBracket)&&peek(1).kind==TokenKind::RBracket){
        advance();
        advance();
        dims++;
    }
    return TypeRef{std::move(name),dims,loc};
}

std::unique_ptr<BlockStmt> Parser::parseBlock(){
    SourceLocation loc=peek().loc;
    expect(TokenKind::LBrace,"'{'");
    std::vector<std::unique_ptr<Stmt>> stmts;
    while(!check(TokenKind::RBrace)&&!atEnd()){
        std::size_t before=pos_;
        std::unique_ptr<Stmt> s=parseStmt();
        if(s)stmts.push_back(std::move(s));
        if(pos_==before)advance();
    }
    expect(TokenKind::RBrace,"'}'");
    return std::make_unique<BlockStmt>(loc,std::move(stmts));
}

std::unique_ptr<Stmt> Parser::parseStmt(){
    if(check(TokenKind::LBrace))return parseBlock();
    if(check(TokenKind::KwIf))return parseIf();
    if(check(TokenKind::KwWhile))return parseWhile();
    if(check(TokenKind::KwBreak))return parseBreak();
    if(check(TokenKind::KwReturn))return parseReturn();
    if(looksLikeTypePrefix())return parseDeclStmt();
    return parseAssignOrExprStmt();
}

bool Parser::looksLikeTypePrefix()const{
    TokenKind k=peek(0).kind;
    bool startsType=
        k==TokenKind::KwInt||
        k==TokenKind::KwBool||
        k==TokenKind::KwString||
        k==TokenKind::KwVoid||
        k==TokenKind::Ident;
    if(!startsType)return false;
    std::size_t off=1;
    while(peek(off).kind==TokenKind::LBracket&&peek(off+1).kind==TokenKind::RBracket){
        off+=2;
    }
    return peek(off).kind==TokenKind::Ident;
}

std::unique_ptr<Stmt> Parser::parseIf(){
    SourceLocation loc=peek().loc;
    expect(TokenKind::KwIf,"'if'");
    expect(TokenKind::LParen,"'(' after 'if'");
    auto cond=parseExpression();
    expect(TokenKind::RParen,"')'");
    auto thenBranch=parseBlock();
    std::unique_ptr<Stmt> elseBranch;
    if(match(TokenKind::KwElse)){
        if(check(TokenKind::KwIf))elseBranch=parseIf();
        else elseBranch=parseBlock();
    }
    return std::make_unique<IfStmt>(loc,std::move(cond),std::move(thenBranch),std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::parseWhile(){
    SourceLocation loc=peek().loc;
    expect(TokenKind::KwWhile,"'while'");
    expect(TokenKind::LParen,"'(' after 'while'");
    auto cond=parseExpression();
    expect(TokenKind::RParen,"')'");
    auto body=parseBlock();
    return std::make_unique<WhileStmt>(loc,std::move(cond),std::move(body));
}

std::unique_ptr<Stmt> Parser::parseBreak(){
    SourceLocation loc=peek().loc;
    expect(TokenKind::KwBreak,"'break'");
    expect(TokenKind::Semi,"';' after 'break'");
    return std::make_unique<BreakStmt>(loc);
}

std::unique_ptr<Stmt> Parser::parseReturn(){
    SourceLocation loc=peek().loc;
    expect(TokenKind::KwReturn,"'return'");
    std::unique_ptr<Expr> value;
    if(!check(TokenKind::Semi))value=parseExpression();
    expect(TokenKind::Semi,"';' after return");
    return std::make_unique<ReturnStmt>(loc,std::move(value));
}

std::unique_ptr<Stmt> Parser::parseDeclStmt(){
    SourceLocation loc=peek().loc;
    TypeRef ty=parseTypeRef();
    Token nameTok=expect(TokenKind::Ident,"name");
    if(check(TokenKind::LParen)){
        expect(TokenKind::LParen,"'('");
        auto params=parseParams();
        expect(TokenKind::RParen,"')'");
        auto body=parseBlock();
        auto fd=std::make_unique<FuncDecl>(loc,std::move(ty),nameTok.lexeme,std::move(params),std::move(body));
        return std::make_unique<NestedFuncStmt>(loc,std::move(fd));
    }
    std::unique_ptr<Expr> init;
    if(match(TokenKind::Eq))init=parseExpression();
    expect(TokenKind::Semi,"';' after declaration");
    return std::make_unique<VarDeclStmt>(loc,std::move(ty),nameTok.lexeme,std::move(init));
}

std::unique_ptr<Stmt> Parser::parseAssignOrExprStmt(){
    SourceLocation loc=peek().loc;
    auto lhs=parseExpression();
    if(match(TokenKind::Eq)){
        auto rhs=parseExpression();
        expect(TokenKind::Semi,"';' after assignment");
        return std::make_unique<AssignStmt>(loc,std::move(lhs),std::move(rhs));
    }
    expect(TokenKind::Semi,"';' after expression");
    return std::make_unique<ExprStmtNode>(loc,std::move(lhs));
}

std::unique_ptr<Expr> Parser::parseExpression(){
    return parseOr();
}

std::unique_ptr<Expr> Parser::parseOr(){
    auto left=parseAnd();
    while(check(TokenKind::PipePipe)){
        SourceLocation loc=peek().loc;
        advance();
        auto right=parseAnd();
        left=std::make_unique<BinaryExpr>(loc,BinaryOp::Or,std::move(left),std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseAnd(){
    auto left=parseEquality();
    while(check(TokenKind::AmpAmp)){
        SourceLocation loc=peek().loc;
        advance();
        auto right=parseEquality();
        left=std::make_unique<BinaryExpr>(loc,BinaryOp::And,std::move(left),std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseEquality(){
    auto left=parseComparison();
    while(check(TokenKind::EqEq)||check(TokenKind::BangEq)){
        SourceLocation loc=peek().loc;
        BinaryOp op=check(TokenKind::EqEq)?BinaryOp::Eq:BinaryOp::Ne;
        advance();
        auto right=parseComparison();
        left=std::make_unique<BinaryExpr>(loc,op,std::move(left),std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseComparison(){
    auto left=parseAdditive();
    while(check(TokenKind::Lt)||check(TokenKind::Gt)||check(TokenKind::LtEq)||check(TokenKind::GtEq)){
        SourceLocation loc=peek().loc;
        BinaryOp op;
        if(check(TokenKind::Lt))op=BinaryOp::Lt;
        else if(check(TokenKind::Gt))op=BinaryOp::Gt;
        else if(check(TokenKind::LtEq))op=BinaryOp::Le;
        else op=BinaryOp::Ge;
        advance();
        auto right=parseAdditive();
        left=std::make_unique<BinaryExpr>(loc,op,std::move(left),std::move(right));
    }
    return left;
}
std::unique_ptr<Expr> Parser::parseAdditive(){
    auto left=parseMultiplicative();
    while(check(TokenKind::Plus)||check(TokenKind::Minus)){
        SourceLocation loc=peek().loc;
        BinaryOp op=check(TokenKind::Plus)?BinaryOp::Add:BinaryOp::Sub;
        advance();
        auto right=parseMultiplicative();
        left=std::make_unique<BinaryExpr>(loc,op,std::move(left),std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseMultiplicative(){
    auto left=parseUnary();
    while(check(TokenKind::Star)||check(TokenKind::Slash)||check(TokenKind::Percent)){
        SourceLocation loc=peek().loc;
        BinaryOp op;
        if(check(TokenKind::Star))op=BinaryOp::Mul;
        else if(check(TokenKind::Slash))op=BinaryOp::Div;
        else op=BinaryOp::Mod;
        advance();
        auto right=parseUnary();
        left=std::make_unique<BinaryExpr>(loc,op,std::move(left),std::move(right));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseUnary(){
    if(check(TokenKind::Minus)){
        SourceLocation loc=peek().loc;
        advance();
        auto operand=parseUnary();
        return std::make_unique<UnaryExpr>(loc,UnaryOp::Neg,std::move(operand));
    }
    if(check(TokenKind::Bang)){
        SourceLocation loc=peek().loc;
        advance();
        auto operand=parseUnary();
        return std::make_unique<UnaryExpr>(loc,UnaryOp::Not,std::move(operand));
    }
    return parsePostfix();
}
std::unique_ptr<Expr> Parser::parsePostfix(){
    auto expr=parsePrimary();
    for(;;){
        if(check(TokenKind::LParen)){
            SourceLocation loc=peek().loc;
            advance();
            auto args=parseArgs();
            expect(TokenKind::RParen,"')'");
            expr=std::make_unique<CallExpr>(loc,std::move(expr),std::move(args));
        }else if(check(TokenKind::LBracket)){
            SourceLocation loc=peek().loc;
            advance();
            auto idx=parseExpression();
            expect(TokenKind::RBracket,"']'");
            expr=std::make_unique<IndexExpr>(loc,std::move(expr),std::move(idx));
        }else if(check(TokenKind::Dot)){
            SourceLocation loc=peek().loc;
            advance();
            Token name=expect(TokenKind::Ident,"field name");
            expr=std::make_unique<FieldExpr>(loc,std::move(expr),name.lexeme);
        }else{
            break;
        }
    }
    return expr;
}
std::unique_ptr<Expr> Parser::parsePrimary(){
    SourceLocation loc=peek().loc;
    const Token& t=peek();
    switch(t.kind){
        case TokenKind::IntLit:{
            Token tk=advance();
            long long v=0;
            try{v=std::stoll(tk.lexeme);}
            catch(const std::out_of_range&){error(tk,"integer literal out of range");}
            return std::make_unique<IntLitExpr>(loc,v);
        }
        case TokenKind::StringLit:{
            Token tk=advance();
            return std::make_unique<StringLitExpr>(loc,tk.lexeme);
        }
        case TokenKind::KwTrue:advance();return std::make_unique<BoolLitExpr>(loc,true);
        case TokenKind::KwFalse:advance();return std::make_unique<BoolLitExpr>(loc,false);
        case TokenKind::KwNull:advance();return std::make_unique<NullLitExpr>(loc);
        case TokenKind::Ident:{
            Token tk=advance();
            return std::make_unique<VarExpr>(loc,tk.lexeme);
        }
        case TokenKind::LParen:{
            advance();
            auto inner=parseExpression();
            expect(TokenKind::RParen,"')'");
            return inner;
        }
        case TokenKind::KwNew:return parseNew(loc);
        default:
            error(t,"expected expression");
            advance();
            return std::make_unique<NullLitExpr>(loc);
    }
}
std::unique_ptr<Expr> Parser::parseNew(SourceLocation loc){
    expect(TokenKind::KwNew,"'new'");
    TypeRef ty=parseTypeRef();
    if(check(TokenKind::LParen)){
        if(ty.arrayDims!=0){
            error(peek(),"cannot construct array type with '('; use '[length]'");
        }
        advance();
        auto args=parseArgs();
        expect(TokenKind::RParen,"')'");
        return std::make_unique<NewStructExpr>(loc,ty.name,std::move(args));
    }
    if(check(TokenKind::LBracket)){
        advance();
        auto length=parseExpression();
        expect(TokenKind::RBracket,"']'");
        return std::make_unique<NewArrayExpr>(loc,std::move(ty),std::move(length));
    }
    error(peek(),"expected '(' or '[' after 'new <type>'");
    return std::make_unique<NullLitExpr>(loc);
}

std::vector<std::unique_ptr<Expr>> Parser::parseArgs(){
    std::vector<std::unique_ptr<Expr>> args;
    if(check(TokenKind::RParen))return args;
    for(;;){
        args.push_back(parseExpression());
        if(!match(TokenKind::Comma))break;
    }
    return args;
}


}