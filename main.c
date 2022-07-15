#include "lexer.h"

int main(void){

    TokenList* tokenlist = tokenize("source.plang");
    printf("\nTokens: \n");
    for (size_t i = 0; i < tokenlist->index; i++){
        if (tokenlist->tokens[i].type == NUMBER){
                printf("\t[Line %d] %s: %s | literal: %f\n", 
                tokenlist->tokens[i].line, 
                token_strings[tokenlist->tokens[i].type], 
                tokenlist->tokens[i].lexeme,
                *(double*)tokenlist->tokens[i].literal);
        } else {
            printf("\t[Line %d] %s: %s | literal: %s\n", 
                tokenlist->tokens[i].line, 
                token_strings[tokenlist->tokens[i].type], 
                tokenlist->tokens[i].lexeme,
                (char*)tokenlist->tokens[i].literal);

        }
    }

    freeTokenList(tokenlist);
    return 0;
}