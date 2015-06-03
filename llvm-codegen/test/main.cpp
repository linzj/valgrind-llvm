#include "IRContext.h"

extern "C" {
void yyparse(IRContext*);
typedef void* yyscan_t;
int yylex_init(yyscan_t* scanner);

int yylex_init_extra(struct IRContext* user_defined, yyscan_t* scanner);

int yylex_destroy(yyscan_t yyscanner);
}

int main()
{
    IRContext context;
    yylex_init_extra(&context, &context.m_scanner);
    yyparse(&context);
    yylex_destroy(context.m_scanner);
    return 0;
}
