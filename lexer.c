#include "lexer.h"
#define UTILS_IMPLEMENT
#include "utils.h"

// global variables (makes life easier and more readable)
long line = 1;
long current = 0; 
long start = 0;

char* source;
long source_len;

const char* token_strings[] = {   
    "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE", "COMMA", "DOT", "MINUS", "PLUS", "SEMICOLON", "SLASH", "STAR", "QMARK", "COLON",
    "BANG", "BANG_EQUAL", "EQUAL", "EQUAL_EQUAL", "GREATER", "GREATER_EQUAL", "LESS", "LESS_EQUAL", "IDENTIFIER", "STRING", "NUMBER",
    "AND", "OR", "PRINT", "IF", "ELSE", "TRUE", "FALSE", "NIL", "FOR", "WHILE", "FUN", "RETURN", "CLASS", "SUPER", "THIS", "VAR", "ENDFILE"
};

#pragma region Keyword_hashmap
// hashmap
Hashlist* hashtab[HASH_SIZE];

unsigned int hash(char* s){
    unsigned int hashval;
    for (hashval = 0; *s != '\0'; s++){
        hashval = *s + 31 * hashval;
    }

    return hashval % HASH_SIZE;
}

char* strdup(char* s){
    char* p = (char*)malloc(strlen(s) + 1);
    if (p != NULL) strcpy(p, s);
    return p;
}

Hashlist* lookup(char* s){
    Hashlist* h;
    for (h = hashtab[hash(s)]; h != NULL; h = h->next){
        if (strcmp(s, h->key) == 0){
            return h;
        }
    }
    return NULL;
}

Hashlist* put(char* key, int val){
    Hashlist* h;
    unsigned int hashval;
    if ((h = lookup(key)) == NULL){
        h = (Hashlist*)malloc(sizeof(*h));
        if (h == NULL || (h->key = strdup(key)) == NULL) return NULL;
        hashval = hash(key);
        h->next = hashtab[hashval];
        hashtab[hashval] = h;
    }
    h->val = val;
    return h;
}
#pragma endregion Keyword_hashmap

#pragma region Lexer_utils
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

char peekNext(){
    if (current + 1 >= source_len) return '\0';
    return source[current + 1];
}
#pragma endregion Lexer_utils

#pragma region Tokenlist
void initTokenList(TokenList* list){
    list->tokens = (Token*)malloc(sizeof(Token) * INITIAL_TOKENLIST_SIZE);
    if (list->tokens == NULL){
        plerror(line, LEX_ERROR, "Malloc failed at init token list");
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
            plerror(line, LEX_ERROR, "Malloc failed in lexeme with type: %s", token_strings[type]);
        }
        for (int i = start; i <= current; i++) lexeme[i-start] = source[i];
        lexeme[current-start] = '\0';
    } else {
        lexeme = NULL;
    }
    list->tokens[list->index++] = (Token){.line=line, .type=type, .literal=literal, .lexeme=lexeme};
}

void addString(TokenList* list){
    while(peek() != '"' && current < source_len){
        if (peek() == '\n') line++;
        advance();
    }

    if (current >= source_len) {
        plerror(line, LEX_ERROR, "Unterminated string literal.");
    }

    advance();
    char* literal = malloc(current - start - 2 + 1); // -2 for quotes and +1 for '\0'
    
    if (literal == NULL) {
        plerror(line, LEX_ERROR, "Malloc failed in string tokenization.");
    }
    int i;
    for (i = start+1; i < current-1; i++) literal[i-(start+1)] = source[i];
    literal[i-(start+1)] = '\0';

    addToken(list, STRING, literal);
}

void addNumber(TokenList* list){
    while (isdigit(peek())) advance();

    if (peek() == '.' && isdigit(peekNext())){
        advance(); // consume dot '.'
        while (isdigit(peek())) advance();
    }
    char* strliteral = malloc(current - start + 1);
    double* literal = malloc(sizeof(double)); 
    int i;
    for (i = start; i < current; i++) strliteral[i-start] = source[i];
    strliteral[i-start] = '\0';
    *literal = atof(strliteral);
    addToken(list, NUMBER, literal);
}

void addIdentifier(TokenList* list){
    while(isalnum(peek())) advance();

    char* text = malloc(current - start + 1);
    int i;
    
    for (i = start; i < current; i++) text[i-start] = source[i];
    text[i-start] = '\0';

    Hashlist* h = lookup(text);
    free(text);
    
    int type;
    if (h == NULL) {
        type = IDENTIFIER;
    } else {
        type = h->val; 
    }
    addToken(list, type, NULL);
}

void freeTokenList(TokenList* list){
    for (size_t i = 0; i < list->index; i++) {
        free(list->tokens[i].lexeme);
        list->tokens[i].lexeme = NULL;
        free(list->tokens[i].literal);
        list->tokens[i].literal = NULL;
    }
    free(list->tokens);
    list->tokens = NULL; // don't want dangling values (use after free!)
    list->index = list->size = 0;
    free(list);
    list = NULL;
}

void printTokenlist(TokenList* list){
    printf("SOURCE (chars %ld):\n%s\n", source_len, source);
    printf("\nTokens: \n");
    for (size_t i = 0; i < list->index; i++){
        enum TokenTypes t = list->tokens[i].type;

        printf("[Line %d] %11s: %7s | Literal: ", 
            list->tokens[i].line, 
            token_strings[t], 
            list->tokens[i].lexeme);

        t == NUMBER ? 
            printf("%f\n", *(double*)list->tokens[i].literal) : 
            printf("%s\n", (char*)list->tokens[i].literal);
    }
}

#pragma endregion Tokenlist

char* read_source_file(const char* file_path){
    FILE* source_file = fopen(file_path, "r");
    if (source_file == NULL) {
        plerror(line, LEX_ERROR, "Couldn't read source file.");
        exit(1);
    }
    fseek(source_file, 0L, SEEK_END);
    long size = ftell(source_file);
    fseek(source_file, 0L, SEEK_SET);
    char* source = malloc(size);
    if (source == NULL) {
        plerror(line, LEX_ERROR, "couldn't allocate memory for source file.");
        exit(1);
    }
    fread(source, size, 1, source_file);
    source[size] = '\0';
    fclose(source_file);
    return source;
}

TokenList* tokenize(char* source_bytes){
    line = 1;
    current = 0;
    start = 0;
    source = source_bytes;
    source_len = strlen(source_bytes);
    TokenList* tokenlist = (TokenList*)malloc(sizeof(TokenList));
    initTokenList(tokenlist);

    memset(hashtab, 0, sizeof(hashtab));
    put("and",    AND);
    put("or",     OR);
    put("print",  PRINT);
    put("if",     IF);
    put("else",   ELSE);
    put("true",   TRUE);
    put("false",  FALSE);
    put("nil",    NIL);
    put("for",    FOR);
    put("while",  WHILE);
    put("fun",    FUN);
    put("return", RETURN);
    put("class",  CLASS);
    put("super",  SUPER);
    put("this",   THIS);
    put("var",    VAR);

    while(current < source_len){
        start = current;

        char c = advance();
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
            case '?': addToken(tokenlist, QMARK, NULL); break;
            case ':': addToken(tokenlist, COLON, NULL); break;
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
                } else if (match('*')){
                    // this is a multiline comment lexeme
                    for (; peek() != '*' && peekNext() != '/' && current < source_len; advance());
                    advance();
                    advance();
                } else addToken(tokenlist, SLASH, NULL);

                }; break;
            
            case ' ':
            case '\r':
            case '\t':
                break;

            case '\n': line++; break;

            case '"': addString(tokenlist); break;
            case '\0': break;
            default: {
                if (isdigit(c)){
                    addNumber(tokenlist);
                } else if (isalpha(c)) {
                    addIdentifier(tokenlist);
                } else {
                    plerror(line, LEX_ERROR, "Unexpected character '%c'", c);
                }
            } break;
        }
    }
    addToken(tokenlist, ENDFILE, NULL);

    return tokenlist;
}