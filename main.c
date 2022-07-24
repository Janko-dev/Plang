#include "lexer.h"
#include "parser.h"

int main(void){

    TokenList* tokenlist = tokenize("source.plang");
    printTokenlist(tokenlist);
    

    Expr* expr = parse(tokenlist);
    
    printf("\n");
    AstPrinter(expr);
    
    freeAst(expr);
    freeTokenList(tokenlist);
    return 0;
}