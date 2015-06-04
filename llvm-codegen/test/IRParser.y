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
}

%{
extern int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, void* yyscanner);
void yyerror(YYLTYPE* yylloc, struct IRContext* context, const char* reason)
{
    contextYYError(yyget_lineno(context->m_scanner), yyget_column(context->m_scanner), context, reason);
}
#define scanner context->m_scanner
%}

%token IR_EXIT NEWLINE ERR SPACE
%token SEPARATOR EQUAL CHECKSTATE CHECKEQ
%token <num> ADDR
%token <text> REGISTER_NAME

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
    REGISTER_NAME SPACE EQUAL SPACE ADDR {
        contextSawRegisterInit(context, $1, $5);
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
    : IR_EXIT SPACE ADDR {
        contextSawIRExit(context, $3);
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
    CHECKEQ SPACE REGISTER_NAME SPACE ADDR {
        contextSawCheckRegisterConst(context, $3, $5);
    }
|   CHECKEQ SPACE REGISTER_NAME SPACE REGISTER_NAME {
        contextSawCheckRegister(context, $3, $5);
    }
| CHECKSTATE SPACE ADDR {
        contextSawChecktState(context, $3);
    }
;
%%
