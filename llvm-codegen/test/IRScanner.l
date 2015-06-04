%{
#include <stdlib.h>
#include <string.h>
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
CHECKEQ CheckEqual
CHECKSTATE CheckState
REGISTER_NAME rax|rbx|rcx|rdx|rsi|rdi|rsp|rbp|r8|r9|r10|r11|r12|r13|r14|r15|rip
ADDR 0x[[:xdigit:]]{1,16}
EQUAL =
NEWLINE [\n\r\f]
SPACE [[:space:]]+
SEPARATOR \%\%
%%
{IR_EXIT}           return IR_EXIT;
{CHECKEQ}           return CHECKEQ;
{CHECKSTATE}        return CHECKSTATE;
{REGISTER_NAME}     %{
                        yylval->text = strdup(yytext);
                        return REGISTER_NAME;
                    %}
{EQUAL}             return EQUAL;
{NEWLINE}           return NEWLINE;
{SPACE}             return SPACE;
{SEPARATOR}         return SEPARATOR;
{ADDR}              %{
                        yylval->num = strtoull(yytext, NULL, 16);
                        return ADDR;
                    %}
.                   %{
                        fprintf(stderr, "error string is %s.\n", yytext);
                        return ERR;
                    %}
%%
