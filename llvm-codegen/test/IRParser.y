%{
#include "IRContext.h"
%}
%pure-parser
%parse-param {struct IRContext* context}
%lex-param { void* scanner }
%locations
%defines

%union {
    unsigned long long num;
}

%{
extern int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, void* yyscanner);
void yyerror(YYLTYPE* yylloc, struct IRContext* context, const char* reason)
{
    contextYYError(yylloc->first_line, yylloc->first_column, context, reason);
}
#define scanner context->m_scanner
%}

%token IR_EXIT NEWLINE ERR SPACE
%token <num> ADDR

%start input
%%

input:
  %empty
| input line
;

line:
  NEWLINE
| statement NEWLINE
;

statement
    : IR_EXIT SPACE ADDR {
        contextSawIRExit(context, $3);
    }
    ;
%%
