#include "parser.hpp"
#include "vm.hpp"
#include <stdio.h>
using namespace Interpreter;
#include "script.lex.hpp"
#include "script.tab.hpp"


void yyerror(const char* s) {
    printf("%s on line:%d\n", s,yylineno);
}

int main(int argc, char*argv[]) {
    FILE * file = fopen(argv[1],"r");
    yyin = file;
    Parser* inst= Parser::current();
    inst->Start();
    if (yyparse()) // 语法分析
    {
        /* BUGBUG */
        fprintf(stderr, "Error !\n");
        exit(1);
    }

    scoped_ptr<Script> st = Parser::current()->Finish();
    Executor exe;
    std::string err = "";
    if(!exe.Execute(st,err)){
        fprintf(stderr,"execute error:%s\n",err.c_str());
    }
    return 0;
}