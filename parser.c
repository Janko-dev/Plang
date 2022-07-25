#include "parser.h"
#include <stdio.h>
#include <stdbool.h>
#define UTILS_IMPLEMENT
#include "utils.h"

static Expr* expression();
static Expr* equality();
static Expr* comparison();
static Expr* term();
static Expr* factor();
static Expr* unary();
static Expr* primary();

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

static Expr* ternaryExpr(Expr* cond, Expr* true_branch, Expr* false_branch){
    Expr* e = newExpr(TERNARY);
    e->as.ternary.cond = cond;
    e->as.ternary.trueBranch = true_branch;
    e->as.ternary.falseBranch = false_branch;
    return e;
}

static Expr* unaryExpr(Token* op, Expr* right){
    Expr* e = newExpr(UNARY);
    e->as.unary.right = right;
    e->as.unary.op = op;
    return e;
}

static Expr* literalExpr(enum LiteralType valueType, void* value){
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
static long current;
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
    current = 0;
    return expression();
}

static Expr* expression(){
    Expr* cond = equality();
    while(check(QMARK)){
        advance();
        Expr* tbranch = expression();
        if (!check(COLON)){
            plerror(peek()->line, PARSE_ERROR, "expected ':' but got '%s'\n", peek()->lexeme);
            exit(1);
        }
        advance();
        Expr* fbranch = expression();
        cond = ternaryExpr(cond, tbranch, fbranch);
    }
    return cond;
}

static Expr* equality(){

    Expr* left = comparison();
    enum TokenTypes types[2] = {BANG_EQUAL, EQUAL_EQUAL};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = comparison();
        left = binaryExpr(op, left, right);
    }

    return left;
}

static Expr* comparison(){

    Expr* left = term();
    enum TokenTypes types[4] = {GREATER, GREATER_EQUAL, LESS, LESS_EQUAL};
    while(match(types, 4)){
        Token* op = previous();
        Expr* right = term();
        left = binaryExpr(op, left, right);
    }
    return left;
}

static Expr* term(){

    Expr* left = factor();
    enum TokenTypes types[2] = {PLUS, MINUS};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = factor();
        left = binaryExpr(op, left, right);
    }
    return left;
}

static Expr* factor(){

    Expr* left = unary();
    enum TokenTypes types[2] = {SLASH, STAR};
    while(match(types, 2)){
        Token* op = previous();
        Expr* right = unary();
        left = binaryExpr(op, left, right);
    }
    return left;
}

static Expr* unary(){
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

static Expr* primary(){
    Expr* result;
    if (check(NUMBER) || check(STRING)){
        advance();
        result = literalExpr(previous()->type == NUMBER ? NUM_T : STR_T, previous()->literal);
    } else if (check(FALSE)){
        advance();
        result = literalExpr(BOOL_T, (void*)false);
    } else if (check(TRUE)){
        advance();
        result = literalExpr(BOOL_T, (void*)true);
    } else if (check(NIL)){
        advance();
        result = literalExpr(NIL_T, (void*)NULL);
    } else if (check(LEFT_PAREN)){
        advance();
        Expr* e = expression();
        if (!check(RIGHT_PAREN)){
            plerror(peek()->line, PARSE_ERROR, "expected ')' but got %s\n", peek()->lexeme);
            exit(1);
        }
        result = groupExpr(e);
        advance();
    } else {
        plerror(peek()->line, PARSE_ERROR, "unhandled value, got \"%s\"\n", peek()->lexeme);
        exit(1);
    }
    return result;
}

// AST methods
LiteralExpr evaluate(Expr* expr){
    switch (expr->type)
    {
    case BINARY: {
        LiteralExpr left = evaluate(expr->as.binary.left);
        LiteralExpr right = evaluate(expr->as.binary.right);

        switch (expr->as.binary.op->type)
        {
        case EQUAL_EQUAL: {
            if (left.value == NULL && right.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)true};
            if (left.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)false};
            bool res = *(double*)left.value == *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)res}; 
        }
        case BANG_EQUAL: {
            if (left.value == NULL && right.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)false};
            if (left.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)true};
            bool res = *(double*)left.value != *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)res};
        }
        case GREATER: {
            bool res = *(double*)left.value > *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)res}; 
        }
        case GREATER_EQUAL: {
            bool res = *(double*)left.value >= *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)res}; 
        }
        case LESS: {
            bool res = *(double*)left.value < *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)res}; 
        }
        case LESS_EQUAL: {
            bool res = *(double*)left.value <= *(double*)right.value;
            return (LiteralExpr){.type=BOOL_T, .value=(void*)res}; 
        }
        case STAR: {
            double res = *(double*)left.value * *(double*)right.value;
            return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
        }
        case SLASH: {
            double res = *(double*)left.value / *(double*)right.value;
            return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
        }
        case MINUS: {
            double res = *(double*)left.value - *(double*)right.value;
            return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
        }
        case PLUS: {
            if (left.type == NUM_T && right.type == NUM_T){
                double res = *(double*)left.value + *(double*)right.value;
                return (LiteralExpr){.type=NUM_T, .value=(void*)&res}; 
            }
            if (left.type == STR_T && right.type == STR_T){
                size_t len_left = strlen((char*)left.value);
                size_t len_right = strlen((char*)right.value);
                char* res = malloc(len_left + len_right); // double '\0' so -1
                for (size_t i = 0; i < len_left; i++) res[i] = ((char*)left.value)[i];
                for (size_t i = len_left; i < len_left + len_right; i++) res[i] = ((char*)right.value)[i-len_left];
                res[len_left+len_right] = '\0';
                return (LiteralExpr){.type=STR_T, .value=(void*)res};
            }
            plerror(-1, RUNTIME_ERROR, "Unreachable\n");
            exit(1);
        }
        default:
            plerror(-1, RUNTIME_ERROR, "Unreachable\n");
            exit(1);;
        }
    } break;
    case TERNARY: {
        LiteralExpr res = evaluate(expr->as.ternary.cond);
        if (res.type != BOOL_T){
            plerror(-1, RUNTIME_ERROR, "Expected condition of type BOOL_T\n");
        }
        if (res.value) {
            return evaluate(expr->as.ternary.trueBranch);
        } else {
            return evaluate(expr->as.ternary.falseBranch);
        }
    } break;
    case UNARY: {
        LiteralExpr right = evaluate(expr->as.unary.right);
        switch(expr->as.unary.op->type){
            case MINUS: {
                *(double*)right.value *= -1;
                return right;
            };
            case BANG: {
                if (right.value == NULL) return (LiteralExpr){.type=BOOL_T, .value=(void*)false};
                if (right.type == BOOL_T) return (LiteralExpr){.type=BOOL_T, .value=(void*)(!*(bool*)right.value)};
                return (LiteralExpr){.type=BOOL_T, .value=(void*)true};
            }
            default: 
                plerror(-1, RUNTIME_ERROR, "Unreachable\n");
                exit(1);
        }
    } break;
    case LITERAL: return expr->as.literal; break;
    case GROUPING: return evaluate(expr->as.group.expression); break;
    default:
        plerror(-1, RUNTIME_ERROR, "Unreachable\n");
        exit(1);
    }
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
    case TERNARY: {
        printf("( ternary ");
        AstPrinter(expr->as.ternary.cond);
        printf(" ? ");
        AstPrinter(expr->as.ternary.trueBranch);
        printf(" : ");
        AstPrinter(expr->as.ternary.falseBranch);
        printf(" )");
    } break;
    case UNARY: {
        printf("( %s ", expr->as.unary.op->lexeme);
        AstPrinter(expr->as.unary.right);
        printf(" )");
    } break;
    case LITERAL: {
        switch (expr->as.literal.type){
            case NUM_T: printf(" %f ", *(double*)expr->as.literal.value); break;
            case BOOL_T: printf(*(bool*)expr->as.literal.value ? " true " : " false "); break;
            case NIL_T: printf(" nil "); break;
            case STR_T: printf(" %s ", (char*)expr->as.literal.value); break;
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

void freeAst(Expr* expr){
    switch (expr->type)
    {
    case BINARY: {
        freeAst(expr->as.binary.left);
        free(expr->as.binary.left);
        freeAst(expr->as.binary.right);
        free(expr->as.binary.right);
        free(expr);
    } break;
    case TERNARY: {
        freeAst(expr->as.ternary.cond);
        free(expr->as.ternary.cond);
        freeAst(expr->as.ternary.trueBranch);
        free(expr->as.ternary.trueBranch);
        freeAst(expr->as.ternary.falseBranch);
        free(expr->as.ternary.falseBranch);
        free(expr);
    } break;
    case UNARY: {
        freeAst(expr->as.unary.right);
        free(expr->as.unary.right);
        free(expr);
    } break;
    case LITERAL: {
        free(expr);
    } break;
    case GROUPING: {
        freeAst(expr->as.group.expression);
        free(expr->as.group.expression);
        free(expr);
    } break;
    default:
        break;
    }
}