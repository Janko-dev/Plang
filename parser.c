#include "parser.h"
#include <stdio.h>
#include <stdbool.h>
#define UTILS_IMPLEMENT
#include "utils.h"

static Stmt declaration(Parser* parser);
static Stmt var_decl(Parser* parser);
static Stmt statement(Parser* parser);

static Expr* expression(Parser* parser);
static Expr* or(Parser* parser);
static Expr* and(Parser* parser);
static Expr* equality(Parser* parser);
static Expr* comparison(Parser* parser);
static Expr* term(Parser* parser);
static Expr* factor(Parser* parser);
static Expr* unary(Parser* parser);
static Expr* primary(Parser* parser);

#pragma region List_utils

StmtList* init_stmt_list(){
    StmtList* list = (StmtList*)malloc(sizeof(StmtList));
    if (list == NULL){
        plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for StmtList");
        exit(1);
    }
    list->statements = (Stmt*)malloc(sizeof(Stmt) * INITIAL_STMTLIST_SIZE);
    if (list->statements == NULL){
        plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for statements");
        free(list);
        exit(1);
    }
    list->index = 0;
    list->size = INITIAL_STMTLIST_SIZE;
    return list;
}

void add_statement(StmtList* list, Stmt stmt){
    if (list->index == list->size){
        list->size *= 2;
        list->statements = realloc(list->statements, sizeof(Stmt) * list->size);
        if (list->statements == NULL){
            plerror(-1, -1, MEMORY_ERR, "Couldn't reallocate memory for statements");
            exit(1);
        }
    }
    list->statements[list->index++] = stmt;
}

void free_expr(Expr* expr){
    if (expr == NULL) return;
    switch (expr->type)
    {
    case BINARY: {
        free_expr(expr->as.binary.left);
        free(expr->as.binary.left);
        expr->as.binary.left = NULL;
        free_expr(expr->as.binary.right);
        free(expr->as.binary.right);
        expr->as.binary.right = NULL;
        free(expr);
        // expr = NULL;
    } break;
    case TERNARY: {
        free_expr(expr->as.ternary.cond);
        free(expr->as.ternary.cond);
        expr->as.ternary.cond = NULL;
        free_expr(expr->as.ternary.trueBranch);
        free(expr->as.ternary.trueBranch);
        expr->as.ternary.trueBranch = NULL;
        free_expr(expr->as.ternary.falseBranch);
        free(expr->as.ternary.falseBranch);
        expr->as.ternary.falseBranch = NULL;
        free(expr);
        expr = NULL;
    } break;
    case UNARY: {
        free_expr(expr->as.unary.right);
        free(expr->as.unary.right);
        expr->as.unary.right = NULL;
        free(expr);
        expr = NULL;
    } break;
    case LITERAL: {
        free(expr);
        expr = NULL;
    } break;
    case GROUPING: {
        free_expr(expr->as.group.expression);
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
        free_expr(expr->as.assign.value);
        expr->as.assign.value = NULL;
        free(expr);
        expr = NULL;
    } break;
    default: break;
    }
}

void free_stmt(Stmt stmt){
    switch(stmt.type)
    {
    case EXPR_STMT: free_expr(stmt.as.expr.expression); break;
    case PRINT_STMT: free_expr(stmt.as.print.expression); break;
    case VAR_DECL_STMT: free_expr(stmt.as.var.initializer); break;
    case IF_STMT: {
        free_expr(stmt.as.if_stmt.cond);
        free_stmt(*stmt.as.if_stmt.trueBranch);
        free(stmt.as.if_stmt.trueBranch);
        if (stmt.as.if_stmt.falseBranch != NULL) free_stmt(*stmt.as.if_stmt.falseBranch);
        free(stmt.as.if_stmt.falseBranch);
    } break;
    case WHILE_STMT: {
        free_expr(stmt.as.while_stmt.cond);
        free_stmt(*stmt.as.while_stmt.body);
        free(stmt.as.while_stmt.body);
    } break;
    case BLOCK_STMT: {
        for (size_t i = 0; i < stmt.as.block.list->index; i++){
            free_stmt(stmt.as.block.list->statements[i]);
        }
        free(stmt.as.block.list->statements);
        stmt.as.block.list->statements = NULL;
        free(stmt.as.block.list);
        stmt.as.block.list = NULL;
    } break;
    default: break;
    }
}

#pragma endregion List_utils

#pragma region Constructors

// expression constructors
static Expr* new_expr(ExprType type){
    Expr* e = malloc(sizeof(*e));
    e->type = type;
    return e;
}

static Expr* binary_expr(Token* op, Expr* left, Expr* right){
    Expr* e = new_expr(BINARY);
    e->as.binary.left = left;
    e->as.binary.right = right;
    e->as.binary.op = op;
    return e;
}

static Expr* ternary_expr(Expr* cond, Expr* true_branch, Expr* false_branch){
    Expr* e = new_expr(TERNARY);
    e->as.ternary.cond = cond;
    e->as.ternary.trueBranch = true_branch;
    e->as.ternary.falseBranch = false_branch;
    return e;
}

static Expr* unary_expr(Token* op, Expr* right){
    Expr* e = new_expr(UNARY);
    e->as.unary.right = right;
    e->as.unary.op = op;
    return e;
}

static Expr* literal_expr(ValueType type){
    Expr* e = new_expr(LITERAL);
    e->as.literal.type = type;
    return e;
}

static Expr* group_expr(Expr* expression){
    Expr* e = new_expr(GROUPING);
    e->as.group.expression = expression;
    return e;
}

static Expr* var_expr(Token* name){
    Expr* e = new_expr(VAREXPR);
    e->as.var.name = name;
    return e;
}

static Expr* assign_expr(Token* name, Expr* value){
    Expr* e = new_expr(ASSIGN);
    e->as.assign.name = name;
    e->as.assign.value = value;
    return e;
}

// statement constructors
// static Stmt* newStmt(enum StmtType type){
//     Stmt* stmt = malloc(sizeof(Stmt));
//     stmt->type = type;
//     return stmt;
// }

static Stmt exprStmt(Expr* expr){
    return (Stmt){
        .type = EXPR_STMT,
        .as.expr.expression = expr
    };
}

static Stmt printStmt(Expr* expr){
    return (Stmt){
        .type = PRINT_STMT,
        .as.print.expression = expr
    };
}

static Stmt declStmt(Token* name, Expr* initializer){
    return (Stmt){
        .type = VAR_DECL_STMT,
        .as.var.name = name,
        .as.var.initializer = initializer,
    };
}

static Stmt blockStmt(StmtList* list){
    return (Stmt){
        .type = BLOCK_STMT,
        .as.block.list = list
    };
}

static Stmt ifStmt(Expr* cond, Stmt* trueBranch, Stmt* falseBranch){
    return (Stmt){
        .type = IF_STMT,
        .as.if_stmt.cond = cond,
        .as.if_stmt.trueBranch = trueBranch,
        .as.if_stmt.falseBranch = falseBranch
    };
}

static Stmt whileStmt(Expr* cond, Stmt* body){
    return (Stmt){
        .type = WHILE_STMT,
        .as.while_stmt.cond = cond,
        .as.while_stmt.body = body
    };
}

#pragma endregion Constructors

#pragma region Parse_utils
// utils
static const char* token_strings[] = {   
    "(", ")", "{", "}", ",", ".", "-", "+", ";", "/", "*", "?", ":",
    "!", "!=", "=", "==", ">", ">=", "<", "<=", "IDENTIFIER", "STRING", "NUMBER",
    "and", "or", "print", "if", "else", "true", "false", "nil", "for", "while", "fun", 
    "return", "class", "super", "this", "var", "EOF"
};

static Token* peek(Parser* parser){
    return &parser->tokenizer->tokens[parser->current_token];
}

static bool check(Parser* parser, TokenType type){
    if (peek(parser)->type == ENDFILE) return false;
    return peek(parser)->type == type;
}

static Token* previous(Parser* parser){
    return &parser->tokenizer->tokens[parser->current_token-1];
}

static Token* advance(Parser* parser){
    if (peek(parser)->type != ENDFILE) parser->current_token++;
    return previous(parser);
}

int get_column(Token* tok, char* source){
    int col;
    size_t cur = tok->start;
    for (col = 0; cur-col > 0 && source[cur-col] != '\n'; col++);
    return col;
}

// static void synchronize(Parser* parser){
//     while(!check(parser, SEMICOLON)) advance(parser);
// }

static void expect(Parser* parser, TokenType type){
    if (!check(parser, type)){
        plerror(peek(parser)->line, get_column(peek(parser), parser->tokenizer->source), PARSE_ERR, "Expected '%s', but got '%s'", 
            token_strings[type], token_strings[peek(parser)->type]);
    }
    advance(parser);
}

static bool match(Parser* parser, TokenType* types, int size){
    for (int i = 0; i < size; i++){
        if (check(parser, types[i])){
            advance(parser);
            return true;
        }
    }
    return false;
}

#pragma endregion parse_utils

#pragma region Grammar
// parser
void free_parser(Parser* parser){
    for (size_t i = 0; i < parser->stmt_list->index; i++){
        free_stmt(parser->stmt_list->statements[i]);
    }
    free(parser->stmt_list->statements);
    parser->stmt_list->statements = NULL;
    free(parser->stmt_list);
    parser->stmt_list = NULL;
    free(parser);
}

Parser* create_parser(Tokenizer* tokenizer){
    Parser* p = (Parser*)malloc(sizeof(Parser));
    if (p == NULL){
        plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for parser object");
        exit(1);
    }
    p->stmt_list = init_stmt_list();
    p->current_token = 0;
    p->tokenizer = tokenizer;
    return p;
}

void parse(Parser* parser){
    while(peek(parser)->type != ENDFILE){
        add_statement(parser->stmt_list, declaration(parser));
    }
}

static Stmt declaration(Parser* parser){
    if (check(parser, VAR)){
        advance(parser);
        return var_decl(parser);
    }
    return statement(parser);
}

static Stmt var_decl(Parser* parser){
    expect(parser, IDENTIFIER);
    Token* id = previous(parser);
    Expr* initializer = NULL;
    if (check(parser, EQUAL)){
        advance(parser);
        initializer = expression(parser);
    }
    expect(parser, SEMICOLON);
    return declStmt(id, initializer);
}

static Stmt statement(Parser* parser){
    if (check(parser, PRINT)){
        advance(parser);
        Expr* expr = expression(parser);
        expect(parser, SEMICOLON);
        return printStmt(expr);

    } else if (check(parser, IF)){
        advance(parser);
        expect(parser, LEFT_PAREN);
        Expr* cond = expression(parser);
        expect(parser, RIGHT_PAREN);
        Stmt* trueBranch = (Stmt*)malloc(sizeof(Stmt));
        Stmt* falseBranch = NULL;
        
        *trueBranch = statement(parser);
        if (check(parser, ELSE)){
            advance(parser);
            falseBranch = (Stmt*)malloc(sizeof(Stmt));
            *falseBranch = statement(parser);
        }
        return ifStmt(cond, trueBranch, falseBranch);

    } else if (check(parser, WHILE)){
        advance(parser);
        expect(parser, LEFT_PAREN);
        Expr* cond = expression(parser);
        expect(parser, RIGHT_PAREN);
        Stmt* body = (Stmt*)malloc(sizeof(Stmt));
        *body = statement(parser);
        return whileStmt(cond, body);

    } else if (check(parser, FOR)){
        advance(parser);
        expect(parser, LEFT_PAREN);
        Stmt decl = {.type=NULL_STMT};
        if (check(parser, VAR)){
            advance(parser);
            decl = var_decl(parser);
        } else if (check(parser, SEMICOLON)){
            advance(parser);
        } else {
            decl = exprStmt(expression(parser));
            expect(parser, SEMICOLON);
        }
        Expr* cond = NULL;
        if (check(parser, SEMICOLON)){
            advance(parser);
        } else {
            cond = expression(parser);
            expect(parser, SEMICOLON);
        }

        Expr* incr = NULL;
        if (check(parser, RIGHT_PAREN)){
            advance(parser);
        } else {
            incr = expression(parser);
            expect(parser, RIGHT_PAREN);
        }

        Stmt loop_body = statement(parser);
        StmtList* body = init_stmt_list();
        add_statement(body, loop_body);

        if (incr != NULL)
            add_statement(body, exprStmt(incr));
        StmtList* list = init_stmt_list();
        if (decl.type != NULL_STMT) 
            add_statement(list, decl);

        if (cond == NULL) {
            cond = literal_expr(BOOL_T);
            cond->as.literal.as.boolean = true;
        }
        Stmt* body_block = (Stmt*)malloc(sizeof(Stmt));
        *body_block = blockStmt(body);
        Stmt loop = whileStmt(cond, body_block);
        add_statement(list, loop);
        return blockStmt(list);

    } else if (check(parser, LEFT_BRACE)) {
        advance(parser);
        StmtList* list = init_stmt_list();
        while (!check(parser, RIGHT_BRACE) && peek(parser)->type != ENDFILE){
            add_statement(list, declaration(parser));
        }
        expect(parser, RIGHT_BRACE);
        return blockStmt(list);

    } else {
        Expr* expr = expression(parser);
        expect(parser, SEMICOLON);
        return exprStmt(expr);
    }
}

static Expr* expression(Parser* parser){
    Expr* expr = or(parser);
    if (check(parser, EQUAL)){
        Token* equal = previous(parser);
        advance(parser);
        Expr* value = expression(parser);
        if (expr->type == VAREXPR){
            Token* name = expr->as.var.name;
            return assign_expr(name, value);
        }
        plerror(equal->line, get_column(peek(parser), parser->tokenizer->source), PARSE_ERR, "Invalid assignment target");
    } else {
        while(check(parser, QMARK)){
            advance(parser);
            Expr* tbranch = expression(parser);
            expect(parser, COLON);
            Expr* fbranch = expression(parser);
            expr = ternary_expr(expr, tbranch, fbranch);
        }
    } 
    return expr;
}

static Expr* or(Parser* parser){
    Expr* left = and(parser);
    TokenType types[1] = {OR};
    while(match(parser, types, 1)){
        Token* op = previous(parser);
        Expr* right = and(parser);
        left = binary_expr(op, left, right);
    }
    return left;
}

static Expr* and(Parser* parser){
    Expr* left = equality(parser);
    TokenType types[1] = {AND};
    while(match(parser, types, 1)){
        Token* op = previous(parser);
        Expr* right = equality(parser);
        left = binary_expr(op, left, right);
    }
    return left;
}

static Expr* equality(Parser* parser){

    Expr* left = comparison(parser);
    TokenType types[2] = {BANG_EQUAL, EQUAL_EQUAL};
    while(match(parser, types, 2)){
        Token* op = previous(parser);
        Expr* right = comparison(parser);
        left = binary_expr(op, left, right);
    }
    return left;
}

static Expr* comparison(Parser* parser){

    Expr* left = term(parser);
    TokenType types[4] = {GREATER, GREATER_EQUAL, LESS, LESS_EQUAL};
    while(match(parser, types, 4)){
        Token* op = previous(parser);
        Expr* right = term(parser);
        left = binary_expr(op, left, right);
    }
    return left;
}

static Expr* term(Parser* parser){

    Expr* left = factor(parser);
    TokenType types[2] = {PLUS, MINUS};
    while(match(parser, types, 2)){
        Token* op = previous(parser);
        Expr* right = factor(parser);
        left = binary_expr(op, left, right);
    }
    return left;
}

static Expr* factor(Parser* parser){

    Expr* left = unary(parser);
    TokenType types[2] = {SLASH, STAR};
    while(match(parser, types, 2)){
        Token* op = previous(parser);
        Expr* right = unary(parser);
        left = binary_expr(op, left, right);
    }
    return left;
}

static Expr* unary(Parser* parser){
    Expr* result;
    TokenType types[2] = {BANG, MINUS};
    if (match(parser, types, 2)){
        Token* op = previous(parser);
        Expr* right = unary(parser);
        result = unary_expr(op, right);
    } else {
        result = primary(parser);
    }
    return result;
}

static Expr* primary(Parser* parser){
    Expr* result;
    if (check(parser, NUMBER)){
        result = literal_expr(NUM_T);
        result->as.literal.as.number = peek(parser)->lit.number;
    } else if (check(parser, STRING)){
        result = literal_expr(STR_T);
        result->as.literal.as.string = peek(parser)->lit.string;
    } else if (check(parser, IDENTIFIER)){
        result = var_expr(peek(parser));
    } else if (check(parser, FALSE)){
        result = literal_expr(BOOL_T);
        result->as.literal.as.boolean = false;
    } else if (check(parser, TRUE)){
        result = literal_expr(BOOL_T);
        result->as.literal.as.boolean = true;
    } else if (check(parser, NIL)){
        result = literal_expr(NIL_T);
    } else if (check(parser, LEFT_PAREN)){
        advance(parser);
        Expr* e = expression(parser);
        expect(parser, RIGHT_PAREN);
        result = group_expr(e);
    } else if (check(parser, SEMICOLON)){
        result = literal_expr(NIL_T);
        return result;
    } else {
        plerror(peek(parser)->line, get_column(peek(parser), parser->tokenizer->source), PARSE_ERR, "unhandled value, got '%s'", token_strings[peek(parser)->type]);
    }

    advance(parser);
    return result;
}
#pragma endregion Grammar

#pragma region AST
// AST methods

void print_lexeme(Parser* parser, Token* token){
    for (size_t i = 0; i < token->count - token->start; i++){
        printf("%c", parser->tokenizer->source[token->start+i]);
    }
}

void expression_printer(Parser* parser, Expr* expr){
    
    switch (expr->type)
    {
    case BINARY: {
        printf("( %s ", token_strings[expr->as.binary.op->type]);
        expression_printer(parser, expr->as.binary.left);
        expression_printer(parser, expr->as.binary.right);
        printf(" )");
    } break;
    case TERNARY: {
        printf("( ternary ");
        expression_printer(parser, expr->as.ternary.cond);
        printf(" ? ");
        expression_printer(parser, expr->as.ternary.trueBranch);
        printf(" : ");
        expression_printer(parser, expr->as.ternary.falseBranch);
        printf(" )");
    } break;
    case UNARY: {
        printf("( %s ", token_strings[expr->as.unary.op->type]);
        expression_printer(parser, expr->as.unary.right);
        printf(" )");
    } break;
    case LITERAL: {
        switch (expr->as.literal.type){
            case NUM_T: printf(" %f", expr->as.literal.as.number); break;
            case BOOL_T: printf(expr->as.literal.as.boolean ? " true" : " false"); break;
            case NIL_T: printf(" nil"); break;
            case STR_T: printf(" \"%s\"", expr->as.literal.as.string); break;
            default: break;
        }
    } break;
    case GROUPING: {
        printf("( group ");
        expression_printer(parser, expr->as.group.expression);
        printf(" )");
    } break;
    case VAREXPR: {
        printf("( id ");
        print_lexeme(parser, expr->as.var.name);
        printf(" )"); 
    } break;
    case ASSIGN: {
        printf("( assign ");
        print_lexeme(parser, expr->as.assign.name);
        printf(" ");
        expression_printer(parser, expr->as.assign.value);
        printf(" )");
    } break;
    default: break;
    }
}

void statement_printer(Parser* parser, Stmt stmt){
    switch(stmt.type)
    {
    case EXPR_STMT: {
        printf("( expr ");
        expression_printer(parser, stmt.as.expr.expression);
        printf(" )");
    } break;
    case PRINT_STMT: {
        printf("( print ");
        expression_printer(parser, stmt.as.expr.expression);
        printf(" )");
    } break;
    case VAR_DECL_STMT: {
        printf("( var decl ");
        print_lexeme(parser, stmt.as.var.name);
        if (stmt.as.var.initializer != NULL)
            expression_printer(parser, stmt.as.var.initializer);
        printf(" )");
    } break;
    case IF_STMT: {
        printf("( if ");
        expression_printer(parser, stmt.as.if_stmt.cond);
        printf(" then ");
        statement_printer(parser, *stmt.as.if_stmt.trueBranch);
        if (stmt.as.if_stmt.falseBranch != NULL){
            printf(" else ");
            statement_printer(parser, *stmt.as.if_stmt.falseBranch);
        }
        printf(" )");
    } break;
    case WHILE_STMT: {
        printf("( while ");
        expression_printer(parser, stmt.as.while_stmt.cond);
        printf(" then ");
        statement_printer(parser, *stmt.as.while_stmt.body);
        printf(" )");
    } break;
    case BLOCK_STMT: {
        printf("( block [ \n");
        for (size_t i = 0; i < stmt.as.block.list->index; i++){
            statement_printer(parser, stmt.as.block.list->statements[i]);
        }
        printf(" ] )\n");
    } break;
    default: break;
    }
}

void print_statements(Parser* parser){
    for (size_t i = 0; i < parser->stmt_list->index; i++){
        statement_printer(parser, parser->stmt_list->statements[i]);
    }
    printf("\n");
}

#pragma endregion AST