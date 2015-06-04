%{
#include <stdlib.h>
#include "log.h"
#include "IRContext.h"
#include "IRParser.h"
%}
%option bison-bridge bison-locations reentrant noyywrap
%option nounput noyy_top_state
%option never-interactive
%option warn nodefault yylineno
%option extra-type="struct IRContext*"

IR_EXIT IR_EXIT
%%
{IR_EXIT}  return IR_EXIT;
[\n\r\f]   return NEWLINE;
[[:space:]]+ return SPACE;
0x[[:xdigit:]]{8,16}  %{
                        yylval->num = strtoull(yytext, NULL, 16);
                        return ADDR;
                    %}
.                   %{
                        fprintf(stderr, "error string is %s.\n", yytext);
                        return ERR;
                    %}
%%
