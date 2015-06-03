%{
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
<<EOF>>    contextSawEof(yyextra);
.          contextSawError(yyextra);
%%
