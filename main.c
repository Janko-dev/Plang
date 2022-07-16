#include "lexer.h"
#include "parser.h"

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

    Expr* expr = parse(tokenlist);
    printf("\n");
    AstPrinter(expr);

    // double* x = malloc(sizeof(double));
    // *x = 2.0f;

    // double* y = malloc(sizeof(double));
    // *y = 3.0f;
    
    // double* z = malloc(sizeof(double));
    // *z = 8.0f;

    // (2 + 3) * 8
    // Expr expr = {
    //     .type=BINARY,
    //     .binexpr=&(BinaryExpr){
    //         .left=&(Expr){
    //             .type=BINARY, 
    //             .binexpr=&(BinaryExpr){
    //                 .left=&(Expr){
    //                     .type=LITERAL, 
    //                     .litexpr=&(LiteralExpr){
    //                         .type=NUMBER, 
    //                         .value=x}},
    //                 .op=&(Token){.type=PLUS, .lexeme="+", .literal=NULL, .line=1},
    //                 .right=&(Expr){
    //                     .type=LITERAL, 
    //                     .litexpr=&(LiteralExpr){
    //                         .type=NUMBER, 
    //                         .value=y}},
    //                 }},
    //         .op=&(Token){.type=STAR, .lexeme="*", .literal=NULL, .line=1},
    //         .right=&(Expr){
    //             .type=LITERAL, 
    //             .litexpr=&(LiteralExpr){
    //                 .type=NUMBER, 
    //                 .value=z}},
    //     }
    // };

    
    freeAst(expr);
    freeTokenList(tokenlist);
    return 0;
}