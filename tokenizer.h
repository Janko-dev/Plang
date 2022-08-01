#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef enum {
    // single-character tokens
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,
    QMARK, COLON,
    
    // one or two character tokens
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,

    // literals
    IDENTIFIER, STRING, NUMBER,

    // keywords
    AND, OR, PRINT, IF, ELSE, TRUE, FALSE, NIL,
    FOR, WHILE, FUN, RETURN, CLASS, SUPER, THIS, VAR,

    ENDFILE,
} TokenType;

#define INITIAL_TOKENLIST_SIZE 20

typedef struct {
    TokenType type;
    size_t line;
    size_t start;
    size_t count;
    union {
        char* string;
        double number;
    } lit;
} Token;

#define HASHTABLE_SIZE 100
typedef struct node_t Node;
struct node_t {
    Node* next;
    char* key;
    TokenType value;
};

typedef struct {
    Token* tokens;
    size_t list_index;
    size_t max_size;
    size_t current_line;
    size_t start_char;
    size_t current_char;
    char* source;
    size_t source_len;

    Node* hashtable[HASHTABLE_SIZE];
} Tokenizer;

char* read_source_file(const char* file_path);
Tokenizer* create_tokenizer(char* text);
void free_tokenizer(Tokenizer* tokenizer);
void tokenize(Tokenizer* tokenizer);
void print_tokens(Tokenizer* tokenizer);

#endif //_TOKENIZER_H

