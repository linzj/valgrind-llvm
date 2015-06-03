%{
#include "IRContext.h"
%}
%pure-parser
%parse-param {struct IRContext* context}
%lex-param { void* scanner }
%locations
%defines

%union {
    struct {
    } lex;
}

%{
extern int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, void* yyscanner);
void yyerror(YYLTYPE* yylloc, struct IRContext* context, const char* reason)
{
    contextYYError(yylloc->first_line, yylloc->first_column, context, reason);
}
#define scanner context->m_scanner
%}

%token <lex> IR_EXIT NEWLINE ERR

%start input
%%

input:
  %empty
| input line
| ERR
;

line:
  NEWLINE
| statement NEWLINE
;

statement
    : IR_EXIT {
        contextSawIRExit(context);
    }
    ;
%%
