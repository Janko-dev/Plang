#include "lexer.h"

// global variables (makes life easier and more readable)
bool hadError = false; 
long line = 1;
long current = 0; 
long start = 0;

char* source;
long source_len;

const char* token_strings[] = {   
    "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE", "COMMA", "DOT", "MINUS", "PLUS", "SEMICOLON", "SLASH", "STAR",
    "BANG", "BANG_EQUAL", "EQUAL", "EQUAL_EQUAL", "GREATER", "GREATER_EQUAL", "LESS", "LESS_EQUAL", "IDENTIFIER", "STRING", "NUMBER",
    "AND", "OR", "PRINT", "IF", "ELSE", "TRUE", "FALSE", "NIL", "FOR", "WHILE", "FUN", "RETURN", "CLASS", "SUPER", "THIS", "ENDFILE"
};

void plang_error(int line, const char* message){
    fprintf(stderr, "[Line %d] Error: %s\n", line, message);
    hadError = true;
}

char advance(){
    return source[current++];
}

bool match(char expected){
    if (current >= source_len) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

char peek(){
    if (current >= source_len) return '\0';
    return source[current];
}

void initTokenList(TokenList* list){
    list->tokens = (Token*)malloc(sizeof(Token) * INITIAL_TOKENLIST_SIZE);
    if (list->tokens == NULL){
        plang_error(line, "Malloc failed.");
    }
    list->index = 0;
    list->size = INITIAL_TOKENLIST_SIZE;
}

void addToken(TokenList* list, enum TokenTypes type, void* literal){
    if (list->index == list->size){
        list->size *= 2;
        list->tokens = realloc(list->tokens, sizeof(Token) * list->size);
    }
    char* lexeme;
    if (type != ENDFILE){
        lexeme = malloc(current - start);
        if (lexeme == NULL){
            plang_error(line, "Malloc failed.");
        }
        for (int i = start; i <= current; i++) lexeme[i-start] = source[i];
        lexeme[current-start] = '\0';    
    } else {
        lexeme = malloc(1);
        if (lexeme == NULL){
            plang_error(line, "Malloc failed.");
        }
        lexeme[0] = '\0';
    }
    list->tokens[list->index++] = (Token){.line=line, .type=type, .literal=literal, .lexeme=lexeme};
}

void addString(TokenList* list){
    while(peek() != '"' && current < source_len){
        if (peek() == '\n') line++;
        advance();
    }

    if (current >= source_len) {
        plang_error(line, "Unterminated string literal.");
        exit(1);
    }

    advance();
    char* literal = malloc(current - start - 2 + 1); // +1 for '\0'
    
    if (literal == NULL) {
        plang_error(line, "Malloc failed");
        exit(1);
    }
    int i;
    for (i = start+1; i < current-1; i++) literal[i-(start+1)] = source[i];
    literal[i-(start+1)] = '\0';

    addToken(list, STRING, literal);
}

void freeTokenList(TokenList* list){
    for (size_t i = 0; i < list->index; i++) free(list->tokens[i].lexeme);
    free(list->tokens);
    list->tokens = NULL; // don't want dangling values (use after free!)
    list->index = list->size = 0;
    free(list);
}

char* read_source_file(const char* file_path){
    FILE* source_file = fopen(file_path, "r");
    if (source_file == NULL) {
        plang_error(line, "Couldn't read source file.");
        exit(1);
    }
    fseek(source_file, 0L, SEEK_END);
    long size = ftell(source_file) - 2;
    fseek(source_file, 0L, SEEK_SET);
    char* source = malloc(size);
    if (source == NULL) {
        plang_error(line, "couldn't allocate memory for source file.");
        exit(1);
    }
    fread(source, 1, size, source_file);
    source[size] = '\0';
    fclose(source_file);
    return source;
}

TokenList* tokenize(const char* file_path){
    source = read_source_file(file_path);
    source_len = strlen(source);
    TokenList* tokenlist = (TokenList*)malloc(sizeof(TokenList));
    initTokenList(tokenlist);

    //printf("SOURCE (%ld):\n%s\n", source_len, source);
    while(current < source_len){
        start = current;

        char c = advance(source);
        switch(c){
            case '(': addToken(tokenlist, LEFT_PAREN, NULL); break;
            case ')': addToken(tokenlist, RIGHT_PAREN, NULL); break;
            case '{': addToken(tokenlist, LEFT_BRACE, NULL); break;
            case '}': addToken(tokenlist, RIGHT_BRACE, NULL); break;
            case ',': addToken(tokenlist, COMMA, NULL); break;
            case '.': addToken(tokenlist, DOT, NULL); break;
            case '-': addToken(tokenlist, MINUS, NULL); break;
            case '+': addToken(tokenlist, PLUS, NULL); break;
            case ';': addToken(tokenlist, SEMICOLON, NULL); break;
            case '*': addToken(tokenlist, STAR, NULL); break;
            case '!': addToken(tokenlist, match('=') ? BANG_EQUAL : BANG, NULL); break;
            case '=': addToken(tokenlist, match('=') ? EQUAL_EQUAL : EQUAL, NULL); break;
            case '<': addToken(tokenlist, match('=') ? LESS_EQUAL : LESS, NULL); break;
            case '>': addToken(tokenlist, match('=') ? GREATER_EQUAL: GREATER, NULL); break;
            case '/': {
                if (match('/'))
                {
                    // this is a comment lexeme
                    for (; peek() != '\n' && current < source_len; advance());
                } else addToken(tokenlist, SLASH, NULL);

                }; break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n': line++; break;

            case '"': addString(tokenlist); break;

            default: plang_error(line, "Unexpected character."); break;
        }
        // printf("\t[Line %d] %s, literal %s\n", 
        //     tokenlist->tokens[tokenlist->index-1].line,  
        //     tokenlist->tokens[tokenlist->index-1].lexeme,
        //     (char*)tokenlist->tokens[tokenlist->index-1].literal);
    }
    addToken(tokenlist, ENDFILE, NULL);
    free(source);

    return tokenlist;
}