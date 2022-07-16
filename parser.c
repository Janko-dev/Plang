#include "parser.h"
#include <stdio.h>
#include <stdbool.h>

static long current = 0;
static TokenList* tokensList;

Expr* equality();
Expr* comparison();
Expr* term();
Expr* factor();
Expr* unary();
Expr* primary();

static Token* peek(){
    return &tokensList->tokens[current];
}

static bool check(enum TokenTypes type){
    if (peek()->type == ENDFILE) return false;
    return peek()->type == type;
}

static Token* previous(){
    return &tokensList->tokens[current-1];
}

static Token* advance(){
    if (peek()->type != ENDFILE) current++;
    return previous();
}

static bool match(enum TokenTypes* types, int size){
    for (int i = 0; i < size; i++){
        if (check(types[i])){
            advance();
            return true;
        }
    }
    return false;
}

Expr* expression(){
    return equality();
}

Expr* equality(){

    Expr* result = malloc(sizeof(Expr));
    Expr* left = malloc(sizeof(Expr));
    left = comparison();
    enum TokenTypes types[2] = {BANG_EQUAL, EQUAL_EQUAL};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = malloc(sizeof(Expr));
        right = comparison();
        result->type = BINARY;
        result->binexpr = malloc(sizeof(BinaryExpr));
        result->binexpr->left = left;
        result->binexpr->op = op;
        result->binexpr->right = right;
    }
    return result->type == BINARY ? result : left;
}

Expr* comparison(){

    Expr* result = malloc(sizeof(Expr));
    Expr* left = malloc(sizeof(Expr));
    left = term();
    enum TokenTypes types[4] = {GREATER, GREATER_EQUAL, LESS, LESS_EQUAL};
    while(match(types, 4)){
        Token* op = previous();
        Expr* right = malloc(sizeof(Expr));
        right = term();
        result->type = BINARY;
        result->binexpr = malloc(sizeof(BinaryExpr));
        result->binexpr->left = left;
        result->binexpr->op = op;
        result->binexpr->right = right;
    }
    return result->type == BINARY ? result : left;
}

Expr* term(){

    Expr* result = malloc(sizeof(Expr));
    Expr* left = malloc(sizeof(Expr));
    left = factor();
    enum TokenTypes types[2] = {PLUS, MINUS};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = malloc(sizeof(Expr));
        right = factor();
        result->type = BINARY;
        result->binexpr = malloc(sizeof(BinaryExpr));
        result->binexpr->left = left;
        result->binexpr->op = op;
        result->binexpr->right = right;
    }
    return result->type == BINARY ? result : left;
}

Expr* factor(){

    Expr* result = malloc(sizeof(Expr));
    
    Expr* left = malloc(sizeof(Expr));
    left = unary();
    enum TokenTypes types[2] = {SLASH, STAR};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = malloc(sizeof(Expr));
        right = unary();
        result->type = BINARY;
        result->binexpr = malloc(sizeof(BinaryExpr));
        result->binexpr->left = left;
        result->binexpr->op = op;
        result->binexpr->right = right;
    }
    return result->type == BINARY ? result : left;
}

Expr* unary(){
    Expr* result = malloc(sizeof(Expr));
    enum TokenTypes types[2] = {BANG, MINUS};
    if (match(types, 2)){
        Token* op = previous();
        Expr* right = malloc(sizeof(Expr));
        right = unary();
        result->type = UNARY;
        result->unexpr = malloc(sizeof(UnaryExpr));
        result->unexpr->op = op;
        result->unexpr->right = right;
    } else {
        result = primary();
    }
    return result;
}

Expr* primary(){
    Expr* result = malloc(sizeof(Expr));
    enum TokenTypes num[1] = {NUMBER};
    if (match(num, 1)){
        result->type = LITERAL;
        result->litexpr = malloc(sizeof(LiteralExpr));
        result->litexpr->type = NUMBER;
        result->litexpr->value = previous()->literal;
    } else {
        enum TokenTypes left[1] = {LEFT_PAREN};
        enum TokenTypes right[1] = {RIGHT_PAREN};
        match(left, 1);
        result = expression();
        match(right, 1);
    }
    return result;
}

void AstPrinter(Expr* expr){
    switch (expr->type)
    {
    case BINARY: {
        printf("( %s ", expr->binexpr->op->lexeme);
        AstPrinter(expr->binexpr->left);
        AstPrinter(expr->binexpr->right);
        printf(" )");
    } break;
    case UNARY: {
        printf("( %s ", expr->unexpr->op->lexeme);
        AstPrinter(expr->unexpr->right);
        printf(" )");
    } break;
    case LITERAL: {
        switch (expr->litexpr->type){
            case NUMBER: printf(" %f ", *(double*)expr->litexpr->value); break;
            case TRUE:
            case FALSE:
            case NIL:
            case STRING: printf(" %s ", (char*)expr->litexpr->value); break;
            default: break;
        }
    } break;
    case GROUPING: {
        printf("( group ");
        AstPrinter(expr->groupexpr->expression);
        printf(" )");
    } break;
    default:
        break;
    }
}

Expr* parse(TokenList* list){
    tokensList = list;
    return expression();
}

void freeAst(Expr* expr){
    switch (expr->type)
    {
    case BINARY: {
        freeAst(expr->binexpr->left);
        free(expr->binexpr->left);
        freeAst(expr->binexpr->right);
        free(expr->binexpr->right);
        free(expr->binexpr);
    } break;
    case UNARY: {
        freeAst(expr->unexpr->right);
        free(expr->unexpr->right);
        free(expr->unexpr);
    } break;
    case LITERAL: {
        free(expr->litexpr);
    } break;
    case GROUPING: {
        freeAst(expr->groupexpr->expression);
        free(expr->groupexpr->expression);
        free(expr->groupexpr);
    } break;
    default:
        break;
    }
    free(expr);
}