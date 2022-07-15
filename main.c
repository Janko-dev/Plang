#include "lexer.h"

int main(void){

    TokenList* tokenlist = tokenize("source.plang");
    printf("Tokens: \n");
    for (size_t i = 0; i < tokenlist->index; i++){
        printf("\t[Line %d] %s: %s, literal %s\n", 
            tokenlist->tokens[i].line, 
            token_strings[tokenlist->tokens[i].type], 
            tokenlist->tokens[i].lexeme,
            (char*)tokenlist->tokens[i].literal);
    }

    freeTokenList(tokenlist);
    return 0;
}