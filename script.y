%{
#include <stdio.h>
#include <string>
#include "parser.hpp"
#define YYDEBUG 1
%}
%require "3.2"
%{
using Parser=Interpreter::Parser;
using Instruction= Interpreter::Instruction;
// https://stackoverflow.com/questions/23717039/generating-a-compiler-from-lex-and-yacc-grammar
int yylex();
void yyerror(const char *s);
%}
%union {
    const char* identifier;
    long        value_integer;
    double      value_double; 
    Instruction* object;
};


%token  <identifier>VAR FUNCTION FOR IF ELIF ELSE ADD SUB MUL DIV MOD ASSIGN
        EQ NE GT GE LT LE LP RP LC RC SEMICOLON IDENTIFIER 
        BREAK CONTINUE RETURN COMMA STRING_LITERAL COLON ADDASSIGN SUBASSIGN
        MULASSIGN DIVASSIGN INC DEC NOT LB RB IN SWITCH CASE DEFAULT

%token <value_integer> INT_LITERAL
%token <value_double>  DOUBLE_LITERAL

%right ASSIGN ADDASSIGN SUBASSIGN DIVASSIGN MULASSIGN



%left  EQ NE GT GE LT LE 
%left  ADD SUB
%left  MUL DIV MOD
%left  COMMA
%left  COLON

%right NOT INC DEC
%left LP RP


%type <object> declaration declarationlist var_declaration
%type <object> declaration_and_assign declaration_and_assign_list var_declaration_and_assign

%type <object> assign_expression assign_expression_list

%type <object> const_value value_expression compare_expression math_expression

%type <object> func_declaration func_call_expression return_expression
%type <object> formal_parameter formal_parameterlist value_list

%type <object> statementlist statement
%type <object> statement_in_block_list statement_in_block block
%type <object> condition_statement if_expresion elseif_expresion elseif_expresionlist else_expresion

%type <object> for_init for_statement for_condition for_update
%type <object> break_expression continue_expression 
%type <object> map_value array_value map_item map_item_list
%type <object> index_to_read slice index_to_write key_value
%type <object> for_in_statement
%type <object> case_item case_item_list switch_case_statement const_value_list
%%

%start  startstatement;


declaration: IDENTIFIER
        {
                $$= Parser::current()->VarDeclarationExpresion($1,NULL);
        }
        | IDENTIFIER ASSIGN value_expression
        {
                $$= Parser::current()->VarDeclarationExpresion($1,$3);
        }
        ;

declaration_and_assign:IDENTIFIER ASSIGN value_expression
        {
                $$= Parser::current()->VarDeclarationExpresion($1,$3);
        }
        ;
declaration_and_assign_list:declaration_and_assign_list COMMA declaration_and_assign
        {
               $$= Parser::current()->AddObjectToObjectList($1,$3);
        }
        |declaration_and_assign
        {
               $$= Parser::current()->CreateObjectList($1); 
        }
        ;

var_declaration_and_assign:VAR declaration_and_assign_list
        {       
                $$= $2;
        }
        ;

declarationlist:declarationlist COMMA declaration
        {
                $$= Parser::current()->AddObjectToObjectList($1,$3);
        }
        | declaration
        {
                $$= Parser::current()->CreateObjectList($1);
        }
        ;

var_declaration: VAR declarationlist
        {
                $$=$2;
        }
        ;


block: LC statement_in_block_list RC 
        {
                $$=$2;
        }
        | LC RC 
        {
             $$=Parser::current()->NULLObject();    
        }
        ;

condition_statement:if_expresion
        {
                $$=$1;
        }
        |if_expresion  else_expresion
        {
                $$=Parser::current()->CreateIFStatement($1,NULL,$2);
        }
        |if_expresion  elseif_expresionlist  else_expresion
        {
                $$=Parser::current()->CreateIFStatement($1,$2,$3);
        }
        ;

if_expresion: IF LP value_expression RP block
        {
                $$=Parser::current()->CreateConditionExpresion($3,$5);
        }
        ;

elseif_expresionlist:elseif_expresionlist  elseif_expresion
        {
                $$=Parser::current()->AddObjectToObjectList($1,$2);
        }
        |elseif_expresion
        {
                $$=Parser::current()->CreateObjectList($1);
        }
        ;

elseif_expresion: ELIF LP value_expression RP block
        {
                $$=Parser::current()->CreateConditionExpresion($3,$5);
        }
        ;

else_expresion: ELSE block
        {
                $$= $2;
        }
        ;

break_expression:BREAK
        {
                $$=Parser::current()->CreateBreakStatement();
        }
        ;

continue_expression:CONTINUE
        {
                $$=Parser::current()->CreateContinueStatement();
        }
        ;

for_init: var_declaration_and_assign
        {
                $$=$1;
        }
        | assign_expression_list
        {
                $$=$1;
        }
        |
        {
                $$=Parser::current()->NULLObject();
        }
        ;

for_condition: value_expression
        {
                $$=$1;
        }
        |
        {
                $$=Parser::current()->NULLObject();
        }
        ;

for_update:assign_expression_list
        {
                $$=$1;
        }
        |
        {
                $$=Parser::current()->NULLObject();
        }
        ;


for_statement: FOR LP for_init SEMICOLON for_condition SEMICOLON for_update RP block
        {
                $$=Parser::current()->CreateForStatement($3,$5,$7,$9);
        }
        |FOR block
        {
                $$=Parser::current()->CreateForStatement(NULL,NULL,NULL,$2);
        }
        ;

for_in_statement:FOR IDENTIFIER COMMA IDENTIFIER IN value_expression block
        {
                $$=Parser::current()->CreateForInStatement($2,$4,$6,$7);
        }
        | FOR IDENTIFIER IN value_expression block
        {
                $$=Parser::current()->CreateForInStatement("",$2,$4,$5);
        }
        ;


const_value:INT_LITERAL
        {
                $$= Parser::current()->CreateConst($1);
        }
        |DOUBLE_LITERAL
        {
                $$= Parser::current()->CreateConst($1);
        }
        |STRING_LITERAL
        {
                $$= Parser::current()->CreateConst($1);
        }
        ;

const_value_list:const_value_list COMMA const_value
        {
                $$=Parser::current()->AddObjectToObjectList($1,$3);        
        }
        |const_value
        {
                $$=Parser::current()->CreateObjectList($1);
        }
        ;

return_expression: RETURN value_expression
        {
                $$= Parser::current()->CreateReturnStatement($2);
        }
        ;

func_declaration:FUNCTION IDENTIFIER LP formal_parameterlist RP block
        {
                $$=Parser::current()->CreateFunction($2,$4,$6);
        }
        | FUNCTION IDENTIFIER LP RP block
        {
                $$=Parser::current()->CreateFunction($2,NULL,$5);
        }
        ;

func_call_expression: IDENTIFIER LP value_list RP
        {
                $$=Parser::current()->CreateFunctionCall($1,$3);
        }
        | IDENTIFIER LP RP
        {
                $$=Parser::current()->CreateFunctionCall($1,NULL);
        }
        ;


formal_parameterlist:formal_parameterlist COMMA formal_parameter
        {
            $$=Parser::current()->AddObjectToObjectList($1,$3);      
        }
        |formal_parameter
        {
             $$=Parser::current()->CreateObjectList($1);   
        }
        ;

formal_parameter:IDENTIFIER
        {
                $$=Parser::current()->VarDeclarationExpresion($1,NULL);
        }
        ;


value_list:value_list COMMA value_expression
        {
                $$=Parser::current()->AddObjectToObjectList($1,$3);    
        }
        |value_expression
        {
                $$=Parser::current()->CreateObjectList($1);
        }
        ;


value_expression: const_value
        {
                $$=$1;
        }
        | IDENTIFIER
        {
                $$=Parser::current()->VarReadExpresion($1);
        }
        | func_call_expression
        {
                $$=$1;
        }
        | math_expression
        {
                $$=$1;
        }
        | compare_expression
        {
                $$=$1;
        }
        |LP math_expression RP
        {
                $$=$2;
        }
        |map_value
        {
                $$=$1;
        }
        |array_value
        {
                $$=$1;
        }
        |index_to_read
        {
                $$=$1;
        }
        |slice
        {
                $$=$1;
        }
        ;


math_expression: value_expression ADD value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kADD);
        }
        | value_expression SUB value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kSUB);
        }
        | value_expression MUL value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kMUL);
        }
        | value_expression DIV value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kDIV);
        }
        | value_expression MOD value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kMOD);
        }
        ;

compare_expression:value_expression EQ value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kEQ);
        }
        |value_expression NE value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kNE);
        }
        |value_expression GT value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kGT);
        }
        |value_expression GE value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kGE);
        }
        |value_expression LT value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kLT);
        }
        |value_expression LE value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($1,$3,Interpreter::Instructions::kLE);
        }
        |NOT value_expression
        {
                $$=Parser::current()->CreateArithmeticOperation($2,NULL,Interpreter::Instructions::kNOT);
        }
        ;

assign_expression: IDENTIFIER ASSIGN value_expression
        {       
                $$=Parser::current()->VarWriteExpresion($1,$3);
        }
        |IDENTIFIER ADDASSIGN value_expression
        {
                $$=Parser::current()->VarUpdateExpression($1,$3,Interpreter::Instructions::kADDWrite);
        }
        |IDENTIFIER SUBASSIGN value_expression
        {
                $$=Parser::current()->VarUpdateExpression($1,$3,Interpreter::Instructions::kSUBWrite);
        }
        |IDENTIFIER MULASSIGN value_expression
        {
                $$=Parser::current()->VarUpdateExpression($1,$3,Interpreter::Instructions::kMULWrite);
        }
        |IDENTIFIER DIVASSIGN value_expression
        {
                $$=Parser::current()->VarUpdateExpression($1,$3,Interpreter::Instructions::kDIVWrite);
        }
        |IDENTIFIER INC
        {
                $$=Parser::current()->VarUpdateExpression($1,NULL,Interpreter::Instructions::kINCWrite);
        }
        |IDENTIFIER DEC 
        {
                $$=Parser::current()->VarUpdateExpression($1,NULL,Interpreter::Instructions::kDECWrite);
        }
        ;

index_to_write:IDENTIFIER LB key_value RB ASSIGN value_expression
        {
                $$=Parser::current()->VarUpdateAtExression($1,$3,$6);
        }
        ;

assign_expression_list: assign_expression_list COMMA assign_expression
        {
               $$=Parser::current()->AddObjectToObjectList($1,$3);  
        }
        |assign_expression
        {
               $$=Parser::current()->CreateObjectList($1);    
        }
        ;


map_item:value_expression COLON value_expression
        {
                $$=Parser::current()->CreateMapItem($1,$3);
        }
        ;
map_item_list:map_item_list COMMA map_item
        {
                $$=Parser::current()->AddObjectToObjectList($1,$3); 
        }
        |map_item
        {
                $$=Parser::current()->CreateObjectList($1);  
        }
        ;

array_value:LB value_list RB
        {
                $$=Parser::current()->CreateArray($2);
        }
        |LB RB
        {
                $$=Parser::current()->CreateArray(NULL);
        }
        ;

map_value: LC map_item_list RC
        {
                $$=Parser::current()->CreateMap($2);
        }
        | LC RC
        {
                $$=Parser::current()->CreateMap(NULL);
        }
        ;

key_value:math_expression
        {
                $$=$1;
        }
        |const_value
        {
                $$=$1;
        }
        |IDENTIFIER{
                $$=Parser::current()->VarReadExpresion($1);
        }
        ;

index_to_read:IDENTIFIER LB key_value RB
        {
                $$=Parser::current()->VarReadAtExpression($1,$3);
        }
        ;

slice:IDENTIFIER LB key_value COLON key_value RB
        {
                $$=Parser::current()->VarSlice($1,$3,$5);
        }
        |IDENTIFIER LB key_value COLON RB
        {
                $$=Parser::current()->VarSlice($1,$3,NULL);
        }
        |IDENTIFIER LB COLON key_value RB
        {
                $$=Parser::current()->VarSlice($1,NULL,$4);
        }
        ;

case_item:CASE const_value_list COLON block
        {
                Instruction* obj =Parser::current()->CreateObjectList($2);
                $$=Parser::current()->AddObjectToObjectList(obj,$4);
        }
        ;
case_item_list:case_item_list case_item
        {
                $$=Parser::current()->AddObjectToObjectList($1,$2);  
        }
        |case_item
        {
                $$=Parser::current()->CreateObjectList($1);
        }
        ;

switch_case_statement: SWITCH LP value_expression RP LC case_item_list DEFAULT COLON block RC
        {
                $$=Parser::current()->CreateSwitchCaseStatement($3,$6,$9);
        }
        |SWITCH LP value_expression RP LC case_item_list RC
        {
                $$=Parser::current()->CreateSwitchCaseStatement($3,$6,NULL);
        }
        ;


statement_in_block:var_declaration SEMICOLON
        |index_to_write SEMICOLON
        |assign_expression SEMICOLON
        |condition_statement
        |return_expression SEMICOLON
        |func_call_expression SEMICOLON
        |for_statement
        |break_expression SEMICOLON
        |continue_expression SEMICOLON
        |for_in_statement
        |switch_case_statement
        ;

statement_in_block_list:statement_in_block_list statement_in_block
        {
                $$=Parser::current()->AddObjectToObjectList($1,$2);
        }
        |statement_in_block
        {
                $$=Parser::current()->CreateObjectList($1);
        }
        ;

statement: func_declaration
        |statement_in_block
        ;

statementlist: statementlist  statement
        {
                $$=Parser::current()->AddObjectToObjectList($1,$2);
        }
        |statement
        {
                $$=Parser::current()->CreateObjectList($1);
        }
        ;

startstatement:statementlist
        {
                Parser::current()->SetEntryPoint($1);
        }
        ;