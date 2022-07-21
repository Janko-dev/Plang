#include "parser.h"
#include <stdio.h>
#include <stdbool.h>

static Expr* newExpr(enum ExprType type){
    Expr* e = malloc(sizeof(*e));
    e->type = type;
    return e;
}

static Expr* binaryExpr(Token* op, Expr* left, Expr* right){
    Expr* e = newExpr(BINARY);
    e->as.binary.left = left;
    e->as.binary.right = right;
    e->as.binary.op = op;
    return e;
}

static Expr* unaryExpr(Token* op, Expr* right){
    Expr* e = newExpr(UNARY);
    e->as.unary.right = right;
    e->as.unary.op = op;
    return e;
}

static Expr* literalExpr(enum TokenTypes valueType, void* value){
    Expr* e = newExpr(LITERAL);
    e->as.literal.type = valueType;
    e->as.literal.value = value;
    return e;
}

static Expr* groupExpr(Expr* expression){
    Expr* e = newExpr(GROUPING);
    e->as.group.expression = expression;
    return e;
}

// global variables
static long current = 0;
static TokenList* tokensList;

// utils
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

// parser
Expr* parse(TokenList* list){
    tokensList = list;
    return expression();
}

Expr* expression(){
    return equality();
}

Expr* equality(){

    Expr* left = comparison();
    enum TokenTypes types[2] = {BANG_EQUAL, EQUAL_EQUAL};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = comparison();
        left = binaryExpr(op, left, right);
    }

    return left;
}

Expr* comparison(){

    Expr* left = term();
    enum TokenTypes types[4] = {GREATER, GREATER_EQUAL, LESS, LESS_EQUAL};
    while(match(types, 4)){
        Token* op = previous();
        Expr* right = term();
        left = binaryExpr(op, left, right);
    }
    return left;
}

Expr* term(){

    Expr* left = factor();
    enum TokenTypes types[2] = {PLUS, MINUS};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = factor();
        left = binaryExpr(op, left, right);
    }
    return left;
}

Expr* factor(){

    Expr* left = unary();
    enum TokenTypes types[2] = {SLASH, STAR};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = unary();
        left = binaryExpr(op, left, right);
    }
    return left;
}

Expr* unary(){
    Expr* result;
    enum TokenTypes types[2] = {BANG, MINUS};
    if (match(types, 2)){
        Token* op = previous();
        Expr* right = unary();
        result = unaryExpr(op, right);
    } else {
        result = primary();
    }
    return result;
}

Expr* primary(){
    Expr* result;
    if (check(NUMBER) || check(STRING)){
        advance();
        result = literalExpr(previous()->type, previous()->literal);
    } else if (check(FALSE)){
        advance();
        result = literalExpr(FALSE, (void*)false);
    } else if (check(TRUE)){
        advance();
        result = literalExpr(TRUE, (void*)true);
    } else if (check(NIL)){
        advance();
        result = literalExpr(NIL, (void*)NULL);
    } else if (check(LEFT_PAREN)){
        advance();
        Expr* e = expression();
        if (!check(RIGHT_PAREN)){
            fprintf(stderr, "Error, expected ')' but got %s\n", peek()->lexeme);
            exit(1);
        }
        result = groupExpr(e);
        advance();
    } else {
        fprintf(stderr, "Error, unhandled primary value, got %s\n", peek()->lexeme);
        exit(1);
    }
    return result;
}

void AstPrinter(Expr* expr){
    switch (expr->type)
    {
    case BINARY: {
        printf("( %s ", expr->as.binary.op->lexeme);
        AstPrinter(expr->as.binary.left);
        AstPrinter(expr->as.binary.right);
        printf(" )");
    } break;
    case UNARY: {
        printf("( %s ", expr->as.unary.op->lexeme);
        AstPrinter(expr->as.unary.right);
        printf(" )");
    } break;
    case LITERAL: {
        switch (expr->as.literal.type){
            case NUMBER: printf(" %f ", *(double*)expr->as.literal.value); break;
            case TRUE: printf(" true "); break;
            case FALSE: printf(" false "); break; 
            case NIL: printf(" nil "); break;
            case STRING: printf(" %s ", (char*)expr->as.literal.value); break;
            default: break;
        }
    } break;
    case GROUPING: {
        printf("( group ");
        AstPrinter(expr->as.group.expression);
        printf(" )");
    } break;
    default:
        break;
    }
}

// void freeAst(Expr* expr){
//     switch (expr->type)
//     {
//     case BINARY: {
//         freeAst(expr->binexpr->left);
//         free(expr->binexpr->left);
//         freeAst(expr->binexpr->right);
//         free(expr->binexpr->right);
//         free(expr->binexpr);
//     } break;
//     case UNARY: {
//         freeAst(expr->unexpr->right);
//         free(expr->unexpr->right);
//         free(expr->unexpr);
//     } break;
//     case LITERAL: {
//         free(expr->litexpr);
//     } break;
//     case GROUPING: {
//         freeAst(expr->groupexpr->expression);
//         free(expr->groupexpr->expression);
//         free(expr->groupexpr);
//     } break;
//     default:
//         break;
//     }
//     free(expr);
// }