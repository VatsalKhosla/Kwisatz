#include <cstdio>
#include <string>
#include <fstream> 
#include <sstream>
#include <iostream>

#include "kwisatz/frame/builder.h"
#include "kwisatz/semant/escape.h"
#include "kwisatz/semant/type_checker.h"
#include "kwisatz/ast/printer.h"
#include "kwisatz/parse/parser.h"
#include "kwisatz/ast/ast.h"
#include "kwisatz/lex/lexer.h"
#include "kwisatz/lex/token.h"
#include "kwisatz/util/version.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "kwisatz %s\n", kwisatz::kVersion);
        std::fprintf(stderr, "usage: %s <source.kw>\n", argv[0]);
        return 1;
    }

    std::string inputPath=argv[1];
    std::ifstream in(inputPath);
    if(!in){
        std::fprintf(stderr, "Error: Could not open file %s\n", inputPath.c_str());
        return 1;
    }
    std::stringstream buffer;
    buffer<<in.rdbuf();
    std::string source=buffer.str();
    kwisatz::Lexer lex(std::move(source), inputPath); 
    kwisatz::Parser parser(lex);
    kwisatz::Program prog=parser.parseProgram();

    if(parser.hadError())return 1;
    std::printf("parsed %zu top-level declarations\n",prog.decls.size());

    kwisatz::TypeContext tc;
    kwisatz::TypeChecker tcheck(tc);
    tcheck.check(prog);
    if(tcheck.hadError())return 1;

    std::printf("type-checked %zu top-level declarations\n",prog.decls.size());

    kwisatz::EscapeAnalyzer ea;
    ea.analyze(prog);
    
    kwisatz::FrameBuilder fb;
    fb.build(prog);

    bool dumpAst=false;
   // std::string inputPath;
    for(int i=1;i<argc;i++){
        std::string a=argv[i];
        if(a=="--dump-ast")dumpAst=true;
        else inputPath=a;
    }

    if(dumpAst)kwisatz::printProgram(prog,std::cout);



    //LEXER UNIT TEST
    /*for(;;){
        kwisatz::Token t=lex.next();
        std::printf("%-12s %3d:%-3d %s\n",
            kwisatz::tokenKindName(t.kind),
            t.loc.line,
            t.loc.column,
            t.lexeme.c_str());
        if(t.kind==kwisatz::TokenKind::EndOfFile) break;
    }*/
    
    return 0;
}
