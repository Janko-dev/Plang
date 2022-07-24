#ifndef _LEXER_H
#define _LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

enum TokenTypes {
    // single-character tokens
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,
    
    // one or two character tokens
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,

    // literals
    IDENTIFIER, STRING, NUMBER,

    // keywords
    AND, OR, PRINT, IF, ELSE, TRUE, FALSE, NIL,
    FOR, WHILE, FUN, RETURN, CLASS, SUPER, THIS, VAR,
    ENDFILE,
};

typedef struct {
    enum TokenTypes type;
    int line;
    char* lexeme;
    void* literal;
} Token;

#define INITIAL_TOKENLIST_SIZE 20
typedef struct {
    Token* tokens;
    size_t index;
    size_t size;
} TokenList;

TokenList* tokenize(const char* file_path);
void printTokenlist(TokenList* list);
void freeTokenList(TokenList* list);

#endif //_LEXER_H
