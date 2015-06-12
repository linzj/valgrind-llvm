%{
#include <stdlib.h>
#include "IRContext.h"
%}
%pure-parser
%parse-param {struct IRContext* context}
%lex-param { void* scanner }
%locations
%defines

%union {
    unsigned long long num;
    char* text;
    void* any;
}

%{
extern int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, void* yyscanner);
void yyerror(YYLTYPE* yylloc, struct IRContext* context, const char* reason)
{
    contextYYError(yyget_lineno(context->m_scanner), yyget_column(context->m_scanner), context, reason);
}
#define scanner context->m_scanner
%}

%token NEWLINE ERR COMMA
%token SEPARATOR EQUAL CHECKSTATE CHECKEQ
%token LEFT_BRACKET RIGHT_BRACKET MEMORY
%token PLUS MINUS MULTIPLE DIVIDE

%token IRST_PUT IRST_EXIT
%token IREXP_CONST IREXP_RDTMP IREXP_LOAD

%token <num> INTNUM
%token <text> REGISTER_NAME IDENTIFIER

%type <any> expression
%type <num> numberic_expression

%left PLUS MINUS
%left MULTIPLE DIVIDE

%start input
%%

input:
  %empty
| register_init_list SEPARATOR statement_list SEPARATOR check_statment_list
;

register_init_list:
    %empty
| register_init_statment_line
| NEWLINE
| register_init_list register_init_statment_line
;

register_init_statment_line:
register_init_statment NEWLINE
;

register_init_statment:
    REGISTER_NAME EQUAL numberic_expression {
        contextSawRegisterInit(context, $1, $3);
        free($1);
    }
    | REGISTER_NAME EQUAL MEMORY LEFT_BRACKET numberic_expression COMMA numberic_expression RIGHT_BRACKET {
        contextSawRegisterInitMemory(context, $1, $5, $7);
        free($1);
    }
;


statement_list:
  NEWLINE
| statement_line
| statement_list statement_line
;

statement_line:
statement NEWLINE
;

statement
    : IRST_EXIT numberic_expression {
        contextSawIRExit(context, $2);
    }
    | IDENTIFIER EQUAL expression {
        contextSawIRWr(context, $1, $3);
        free($1);
    }
    | IRST_PUT LEFT_BRACKET numberic_expression COMMA expression RIGHT_BRACKET {
        contextSawIRPutExpr(context, $3, $5);
    }
    ;

expression
    : IREXP_CONST LEFT_BRACKET numberic_expression RIGHT_BRACKET {
        $$ = contextNewConstExpr(context, $3);
        if ($$ == NULL) {
            YYABORT;
        }
    }
    | IREXP_RDTMP LEFT_BRACKET IDENTIFIER RIGHT_BRACKET {
        $$ = contextNewRdTmpExpr(context, $3);
        free($3);
        if ($$ == NULL) {
            YYABORT;
        }
    }
    | IREXP_LOAD LEFT_BRACKET expression RIGHT_BRACKET {
        $$ = contextNewLoadExpr(context, $3);
        if ($$ == NULL) {
            YYABORT;
        }
    }
    ;

numberic_expression
    : INTNUM {
        $$ = $1;
    }
    | numberic_expression PLUS numberic_expression {
        $$ = $1 + $3;
    }
    | numberic_expression MINUS numberic_expression {
        $$ = $1 - $3;
    }
    | numberic_expression MULTIPLE numberic_expression {
        $$ = $1 * $3;
    }
    | numberic_expression DIVIDE numberic_expression {
        $$ = $1 / $3;
    }
    | LEFT_BRACKET numberic_expression RIGHT_BRACKET {
        $$ = $2;
    }
    ;

check_statment_list:
    NEWLINE
| check_statment_line
| check_statment_list check_statment_line
;

check_statment_line:
check_statment NEWLINE
;

check_statment:
    CHECKEQ REGISTER_NAME numberic_expression {
        contextSawCheckRegisterConst(context, $2, $3);
        free($2);
    }
|   CHECKEQ REGISTER_NAME REGISTER_NAME {
        contextSawCheckRegister(context, $2, $3);
        free($2);
        free($3);
    }
| CHECKSTATE numberic_expression numberic_expression {
        contextSawChecktState(context, $2, $3);
    }
;
%%
