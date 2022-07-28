#include "parser.h"
#include <stdio.h>
#include <stdbool.h>
#define UTILS_IMPLEMENT
#include "utils.h"

static Stmt* declaration();
static Stmt* varDecl();
static Stmt* statement();

static Expr* expression();
static Expr* equality();
static Expr* comparison();
static Expr* term();
static Expr* factor();
static Expr* unary();
static Expr* primary();

// global variables
static long current;
static TokenList* tokensList;

#pragma region List_utils

void initStmtList(StmtList* list){
    list->statements = malloc(sizeof(Stmt) * INITIAL_STMTLIST_SIZE);
    if (list->statements == NULL){
        plerror(-1, PARSE_ERROR, "Malloc failed at init statement list");
    }
    list->index = 0;
    list->size = INITIAL_STMTLIST_SIZE;
}

void addStatement(StmtList* list, Stmt stmt){
    if (list->index == list->size){
        list->size *= 2;
        list->statements = realloc(list->statements, sizeof(Stmt) * list->size);
    }
    list->statements[list->index++] = stmt;
}

void freeExpr(Expr* expr){
    if (expr == NULL) return;
    switch (expr->type)
    {
    case BINARY: {
        freeExpr(expr->as.binary.left);
        free(expr->as.binary.left);
        expr->as.binary.left = NULL;
        freeExpr(expr->as.binary.right);
        free(expr->as.binary.right);
        expr->as.binary.right = NULL;
        free(expr);
        expr = NULL;
    } break;
    case TERNARY: {
        freeExpr(expr->as.ternary.cond);
        free(expr->as.ternary.cond);
        expr->as.ternary.cond = NULL;
        freeExpr(expr->as.ternary.trueBranch);
        free(expr->as.ternary.trueBranch);
        expr->as.ternary.trueBranch = NULL;
        freeExpr(expr->as.ternary.falseBranch);
        free(expr->as.ternary.falseBranch);
        expr->as.ternary.falseBranch = NULL;
        free(expr);
        expr = NULL;
    } break;
    case UNARY: {
        freeExpr(expr->as.unary.right);
        free(expr->as.unary.right);
        expr->as.unary.right = NULL;
        free(expr);
        expr = NULL;
    } break;
    case LITERAL: {
        free(expr->as.literal.value); 
        expr->as.literal.value = NULL;
        free(expr);
        expr = NULL;
    } break;
    case GROUPING: {
        freeExpr(expr->as.group.expression);
        free(expr->as.group.expression);
        expr->as.group.expression = NULL;
        free(expr);
        expr = NULL;
    } break;
    case VAREXPR: {
        free(expr);
        expr = NULL;
    } break;
    case ASSIGN: {
        freeExpr(expr->as.assign.value);
        expr->as.assign.value = NULL;
        free(expr);
        expr = NULL;
    } break;
    default: break;
    }
}

void freeStmtList(StmtList* list){
    for (size_t i = 0; i < list->index; i++){
        switch(list->statements[i].type)
        {
        case EXPR_STMT: freeExpr(list->statements[i].as.expr.expression); break;
        case PRINT_STMT: freeExpr(list->statements[i].as.print.expression); break;
        case VAR_DECL_STMT: freeExpr(list->statements[i].as.var.initializer); break;
        case BLOCK_STMT: freeStmtList(list->statements[i].as.block.list); break;
        default: break;
        }
    }
    free(list);
    list = NULL;
}

#pragma endregion List_utils

#pragma region Constructors

// expression constructors
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

static Expr* varExpr(Token* name){
    Expr* e = newExpr(VAREXPR);
    e->as.var.name = name;
    return e;
}

static Expr* assignExpr(Token* name, Expr* value){
    Expr* e = newExpr(ASSIGN);
    e->as.assign.name = name;
    e->as.assign.value = value;
    return e;
}

// statement constructors
static Stmt* newStmt(enum StmtType type){
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = type;
    return stmt;
}

static Stmt* exprStmt(Expr* expr){
    Stmt* stmt = newStmt(EXPR_STMT);
    stmt->as.expr.expression = expr;
    return stmt;
}

static Stmt* printStmt(Expr* expr){
    Stmt* stmt = newStmt(PRINT_STMT);
    stmt->as.print.expression = expr;
    return stmt;
}

static Stmt* declStmt(Token* name, Expr* initializer){
    Stmt* stmt = newStmt(VAR_DECL_STMT);
    stmt->as.var.name = name;
    stmt->as.var.initializer = initializer;
    return stmt;
}

static Stmt* blockStmt(StmtList* list){
    Stmt* stmt = newStmt(BLOCK_STMT);
    stmt->as.block.list = list;
    return stmt;
}

static Stmt* ifStmt(Expr* cond, Stmt* trueBranch, Stmt* falseBranch){
    Stmt* stmt = newStmt(IF_STMT);
    stmt->as.if_stmt.cond = cond;
    stmt->as.if_stmt.trueBranch = trueBranch;
    stmt->as.if_stmt.falseBranch = falseBranch;
    return stmt;
}

static Stmt* whileStmt(Expr* cond, Stmt* body){
    Stmt* stmt = newStmt(WHILE_STMT);
    stmt->as.while_stmt.cond = cond;
    stmt->as.while_stmt.body = body;
    return stmt;
}

#pragma endregion Constructors

#pragma region Parse_utils
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

#pragma endregion parse_utils

#pragma region Grammar
// parser

// void synchronize(){
//     advance();

// }

StmtList* parse(TokenList* list){
    tokensList = list;
    current = 0;
    StmtList* statements = (StmtList*)malloc(sizeof(StmtList));
    initStmtList(statements);
    while(peek()->type != ENDFILE){
        addStatement(statements, *declaration());
    }
    return statements;
}

static Stmt* declaration(){
    if (check(VAR)){
        advance();
        return varDecl();
    }
    return statement();
}

static Stmt* varDecl(){
    if (!check(IDENTIFIER)){
        plerror(peek()->line, PARSE_ERROR, "expected an identifier after 'var' keyword, but got %s", peek()->lexeme);
    }
    advance();
    Token* id = previous();
    Expr* initializer = NULL;
    if (check(EQUAL)){
        advance();
        initializer = expression();
    }
    if (!check(SEMICOLON)){
        plerror(peek()->line, PARSE_ERROR, "expected ';' at the end of variable declaration, but got %s", peek()->lexeme);
    }
    advance();
    return declStmt(id, initializer);
}

static Stmt* statement(){
    if (check(PRINT)){
        advance();
        Expr* expr = expression();
        if (!check(SEMICOLON)){
            plerror(peek()->line, PARSE_ERROR, "expected ';' at the end of print statement, but got %s", peek()->lexeme);
        }
        advance();
        return printStmt(expr);
    } else if (check(IF)){
        advance();
        if (!check(LEFT_PAREN)){
            plerror(peek()->line, PARSE_ERROR, "expected '(' after 'if' keyword, but got %s", peek()->lexeme);
        }
        advance();
        Expr* cond = expression();
        if (!check(RIGHT_PAREN)){
            plerror(peek()->line, PARSE_ERROR, "expected ')' after condition of if-statement, but got %s", peek()->lexeme);
        }
        advance();
        Stmt* trueBranch = statement();
        Stmt* falseBranch = NULL;
        if (check(ELSE)){
            advance();
            falseBranch = statement();
        }
        return ifStmt(cond, trueBranch, falseBranch);
    } else if (check(WHILE)){
        advance();
        if (!check(LEFT_PAREN)){
            plerror(peek()->line, PARSE_ERROR, "expected '(' after 'while' keyword, but got %s", peek()->lexeme);
        }
        advance();
        Expr* cond = expression();
        if (!check(RIGHT_PAREN)){
            plerror(peek()->line, PARSE_ERROR, "expected ')' after condition of while-statement, but got %s", peek()->lexeme);
        }
        advance();
        Stmt* body = statement();
        return whileStmt(cond, body);
    } else if (check(LEFT_BRACE)) {
        advance();
        StmtList* list = (StmtList*)malloc(sizeof(StmtList));
        initStmtList(list);
        while (!check(RIGHT_BRACE) && peek()->type != ENDFILE){
            addStatement(list, *declaration());
        }
        if (!check(RIGHT_BRACE)){
            plerror(peek()->line, PARSE_ERROR, "expected '}' at the end of block statement, but got %s", peek()->lexeme);
        }
        advance();
        return blockStmt(list);
    } else {
        Expr* expr = expression();
        if (!check(SEMICOLON)){
            plerror(peek()->line, PARSE_ERROR, "expected ';' at the end of expression statement, but got %s", peek()->lexeme);
        }
        advance();
        return exprStmt(expr);
    }
}

static Expr* expression(){
    Expr* expr = equality();
    if (check(EQUAL)){
        Token* equal = previous();
        advance();
        Expr* value = expression();
        if (expr->type == VAREXPR){
            Token* name = expr->as.var.name;
            return assignExpr(name, value);
        }
        plerror(equal->line, PARSE_ERROR, "Invalid assignment target");
    } else {
        while(check(QMARK)){
            advance();
            Expr* tbranch = expression();
            if (!check(COLON)){
                plerror(peek()->line, PARSE_ERROR, "expected ':' but got '%s'", peek()->lexeme);
            }
            advance();
            Expr* fbranch = expression();
            expr = ternaryExpr(expr, tbranch, fbranch);
        }
    } 
    return expr;
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
    } else if (check(IDENTIFIER)){
        advance();
        result = varExpr(previous());
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
            plerror(peek()->line, PARSE_ERROR, "expected ')' but got %s", peek()->lexeme);
        }
        result = groupExpr(e);
        advance();
    } else {
        plerror(peek()->line, PARSE_ERROR, "unhandled value, got \"%s\"", peek()->lexeme);
    }
    return result;
}
#pragma endregion Grammar

#pragma region AST
// AST methods

void expressionPrinter(Expr* expr){
    
    switch (expr->type)
    {
    case BINARY: {
        printf("( %s ", expr->as.binary.op->lexeme);
        expressionPrinter(expr->as.binary.left);
        expressionPrinter(expr->as.binary.right);
        printf(" )");
    } break;
    case TERNARY: {
        printf("( ternary ");
        expressionPrinter(expr->as.ternary.cond);
        printf(" ? ");
        expressionPrinter(expr->as.ternary.trueBranch);
        printf(" : ");
        expressionPrinter(expr->as.ternary.falseBranch);
        printf(" )");
    } break;
    case UNARY: {
        printf("( %s ", expr->as.unary.op->lexeme);
        expressionPrinter(expr->as.unary.right);
        printf(" )");
    } break;
    case LITERAL: {
        switch (expr->as.literal.type){
            case NUM_T: printf(" %f ", *(double*)expr->as.literal.value); break;
            case BOOL_T: printf(expr->as.literal.value ? " true " : " false "); break;
            case NIL_T: printf(" nil "); break;
            case STR_T: printf(" %s ", (char*)expr->as.literal.value); break;
            default: break;
        }
    } break;
    case GROUPING: {
        printf("( group ");
        expressionPrinter(expr->as.group.expression);
        printf(" )");
    } break;
    case VAREXPR: printf("( id %s )", expr->as.var.name->lexeme); break;
    case ASSIGN: {
        printf("( assign %s ", expr->as.assign.name->lexeme);
        expressionPrinter(expr->as.assign.value);
        printf(" )");
    } break;
    default: break;
    }
}

void statementPrinter(Stmt* stmt){
    switch(stmt->type)
    {
    case EXPR_STMT: {
        printf("( expr ");
        expressionPrinter(stmt->as.expr.expression);
        printf(" )");
    } break;
    case PRINT_STMT: {
        printf("( print ");
        expressionPrinter(stmt->as.expr.expression);
        printf(" )");
    } break;
    case VAR_DECL_STMT: {
        printf("( var decl %s ", stmt->as.var.name->lexeme);
        if (stmt->as.var.initializer != NULL)
            expressionPrinter(stmt->as.var.initializer);
        printf(" )");
    } break;
    case IF_STMT: {
        printf("( if ");
        expressionPrinter(stmt->as.if_stmt.cond);
        printf(" then ");
        statementPrinter(stmt->as.if_stmt.trueBranch);
        if (stmt->as.if_stmt.falseBranch != NULL){
            printf(" else ");
            statementPrinter(stmt->as.if_stmt.falseBranch);
        }
        printf(" )");
    } break;
    case WHILE_STMT: {
        printf("( while ");
        expressionPrinter(stmt->as.while_stmt.cond);
        printf(" then ");
        statementPrinter(stmt->as.while_stmt.body);
        printf(" )");
    } break;
    case BLOCK_STMT: {
        printf("( block [ ");
        for (size_t i = 0; i < stmt->as.block.list->index; i++){
            printf("\n");
            statementPrinter(&stmt->as.block.list->statements[i]);
        }
        printf("\n] )");
    } break;
    default: break;
    }
}

#pragma endregion AST