#include "tokenizer.h"
#define UTILS_IMPLEMENT
#include "utils.h"

char* read_source_file(const char* file_path){
    FILE* source_file = fopen(file_path, "r");
    if (source_file == NULL) {
        plerror(-1, -1, MEMORY_ERR, "Couldn't read source file: %s", file_path);
        exit(1);
    }
    fseek(source_file, 0L, SEEK_END);
    long size = ftell(source_file);
    fseek(source_file, 0L, SEEK_SET);
    char* source = (char*)malloc(sizeof(char) * size + 1);
    if (source == NULL) {
        plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for source file");
        exit(1);
    }
    fread(source, size, 1, source_file);
    source[size] = '\0';
    fclose(source_file);
    return source;
}

unsigned int hash(char* s){
    unsigned long hash = 5381;
    for (size_t i = 0; s[i] != '\0'; i++){
        hash = ((hash << 5) + hash) + (int)s[i]; /* hash * 33 + c */
    }
    return hash % HASHTABLE_SIZE;
}

Node* lookup(Node* hashtable[HASHTABLE_SIZE], char* s){
    Node* h_list;
    for (h_list = hashtable[hash(s)]; h_list != NULL; h_list = h_list->next){
        if (strcmp(h_list->key, s) == 0){
            return h_list;
        }
    }
    return NULL;
}

void put(Node* hashtable[HASHTABLE_SIZE], char* key, TokenType val){

    Node* h_list;
    if ((h_list = lookup(hashtable, key)) == NULL){
        h_list = (Node*)malloc(sizeof(Node));
        if (h_list == NULL) {
            plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for hashtable node: %s", key);
            exit(1);
        }
        h_list->key = malloc(sizeof(char) * strlen(key) + 1);
        if (h_list->key == NULL) {
            plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for hashtable node key");
            exit(1);
        } 
        strcpy(h_list->key, key);
        unsigned int h_val = hash(key);
        h_list->next = hashtable[h_val];
        hashtable[h_val] = h_list;
    }
    h_list->value = val;
}

Tokenizer* create_tokenizer(char* text){
    Tokenizer* tokenizer = (Tokenizer*)malloc(sizeof(*tokenizer));
    if (tokenizer == NULL) {
        plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for tokenization");
        exit(1);
    }
    tokenizer->source = text;
    tokenizer->source_len = strlen(text);
    
    tokenizer->tokens = (Token*)malloc(sizeof(Token) * INITIAL_TOKENLIST_SIZE);
    if (tokenizer->tokens == NULL){
        free(tokenizer);
        plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for token list");
        exit(1);
    }
    tokenizer->list_index = 0;
    tokenizer->max_size = INITIAL_TOKENLIST_SIZE;

    tokenizer->current_line = 1;
    tokenizer->start_char = 0;
    tokenizer->current_char = 0;
    
    memset(tokenizer->hashtable, 0, sizeof(Node*) * HASHTABLE_SIZE);
    put(tokenizer->hashtable, "and",    AND);
    put(tokenizer->hashtable, "or",     OR);
    put(tokenizer->hashtable, "print",  PRINT);
    put(tokenizer->hashtable, "if",     IF);
    put(tokenizer->hashtable, "else",   ELSE);
    put(tokenizer->hashtable, "true",   TRUE);
    put(tokenizer->hashtable, "false",  FALSE);
    put(tokenizer->hashtable, "nil",    NIL);
    put(tokenizer->hashtable, "for",    FOR);
    put(tokenizer->hashtable, "while",  WHILE);
    put(tokenizer->hashtable, "fun",    FUN);
    put(tokenizer->hashtable, "return", RETURN);
    put(tokenizer->hashtable, "class",  CLASS);
    put(tokenizer->hashtable, "super",  SUPER);
    put(tokenizer->hashtable, "this",   THIS);
    put(tokenizer->hashtable, "var",    VAR);

    return tokenizer;
}

void free_tokenizer(Tokenizer* tokenizer){
    for (size_t i = 0; i < tokenizer->list_index; i++){
        // printf("%s\n", tokenizer->tokens[i].lit.string);
        if (tokenizer->tokens[i].type == STRING){
            free(tokenizer->tokens[i].lit.string);
        }
    }
    free(tokenizer->tokens);
    tokenizer->tokens = NULL;

    for (int i = 0; i < HASHTABLE_SIZE; i++){
        Node* h = tokenizer->hashtable[i];
        Node* tmp;
        while(h != NULL){
            tmp = h;
            h = h->next;
            free(tmp->key);
            free(tmp);
        }
    }
    free(tokenizer);
    tokenizer = NULL;
}

int get_column(Token* tok){
    int col;
    size_t cur = tok->start;
    for (col = 0; cur-col > 0 && tok->source[cur-col] != '\n'; col++);
    return col;
}

static bool match(Tokenizer* tokenizer, char expected){
    if (tokenizer->current_char >= tokenizer->source_len) return false;
    if (tokenizer->source[tokenizer->current_char] != expected) return false;
    tokenizer->current_char++;
    return true;
}

static char advance(Tokenizer* tokenizer){
    return tokenizer->source[tokenizer->current_char++];
}

static char peek(Tokenizer* tokenizer){
    if (tokenizer->current_char >= tokenizer->source_len) return '\0';
    return tokenizer->source[tokenizer->current_char];
}

static char peek_next(Tokenizer* tokenizer){
    if (tokenizer->current_char + 1 >= tokenizer->source_len) return '\0';
    return tokenizer->source[tokenizer->current_char + 1];
}

void addToken(Tokenizer* tokenizer, TokenType type) {
    if (tokenizer->list_index == tokenizer->max_size){
        tokenizer->max_size *= 2;
        tokenizer->tokens = realloc(tokenizer->tokens, sizeof(Token) * tokenizer->max_size);
        if (tokenizer->tokens == NULL){
            plerror(-1, -1, MEMORY_ERR, "Reallocation of token list failed, couldn't allocate memory");
            exit(1);
        }
    }

    tokenizer->tokens[tokenizer->list_index] = (Token){
        .start = tokenizer->start_char,
        .count = tokenizer->current_char,
        .line = tokenizer->current_line,
        .type = type,
        .source = tokenizer->source
    };

    char* literal = NULL;
    if (type == NUMBER){
        size_t n = tokenizer->current_char - tokenizer->start_char;
        literal = malloc(n + 1);
        if (literal == NULL) {
            plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for number literal");
            exit(1);
        }
        for (size_t i = 0; i < n; i++) 
            literal[i] = tokenizer->source[tokenizer->start_char+i];
        literal[n] = '\0';
        tokenizer->tokens[tokenizer->list_index].lit.number = atof(literal);
        free(literal);
    } else if (type == STRING){
        size_t n = tokenizer->current_char - tokenizer->start_char - 2;
        literal = malloc(n + 1); // -2 for quotes and +1 for '\0'
        if (literal == NULL) {
            plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for string literal");
            exit(1);
        }
        for (size_t i = 0; i < n; i++) 
            literal[i] = tokenizer->source[tokenizer->start_char+i+1];
        literal[n] = '\0';
        tokenizer->tokens[tokenizer->list_index].lit.string = literal;
    }
    tokenizer->list_index++;
}

void addString(Tokenizer* tokenizer){
    while(peek(tokenizer) != '"' && tokenizer->current_char < tokenizer->source_len){
        if (peek(tokenizer) == '\n') {
            plerror(tokenizer->current_line, get_column(&tokenizer->tokens[tokenizer->list_index]), TOKEN_ERR, "Unterminated string literal");
            return;
        }
        advance(tokenizer);
    }

    if (tokenizer->current_char >= tokenizer->source_len) {
        plerror(tokenizer->current_line, get_column(&tokenizer->tokens[tokenizer->list_index]), TOKEN_ERR, "Unterminated string literal");
        return;
    }

    advance(tokenizer);
    addToken(tokenizer, STRING);
}

void addNumber(Tokenizer* tokenizer){
    while (isdigit(peek(tokenizer))) advance(tokenizer);

    if (peek(tokenizer) == '.' && isdigit(peek_next(tokenizer))){
        advance(tokenizer); // consume dot '.'
        while (isdigit(peek(tokenizer))) advance(tokenizer);
    }
    addToken(tokenizer, NUMBER);
}

void addIdentifier(Tokenizer* tokenizer){
    while(isalnum(peek(tokenizer))) advance(tokenizer);
    size_t n = tokenizer->current_char - tokenizer->start_char;
    char* buf = (char*)malloc(n*sizeof(char)+1);
    if (buf == NULL) {
        plerror(-1, -1, MEMORY_ERR, "Couldn't allocate memory for string literal\n");
        exit(1);
    }
    strncpy(buf, tokenizer->source + tokenizer->start_char, n);
    buf[n] = '\0';
    Node* type = lookup(tokenizer->hashtable, buf);
    free(buf);
    if (type == NULL) {
        addToken(tokenizer, IDENTIFIER);
    } else {
        addToken(tokenizer, type->value);
    }
}

void tokenize(Tokenizer* tokenizer){

    while(tokenizer->current_char < tokenizer->source_len){
        tokenizer->start_char = tokenizer->current_char;
        char c = advance(tokenizer);
        switch (c){
            case '(': addToken(tokenizer, LEFT_PAREN); break;
            case ')': addToken(tokenizer, RIGHT_PAREN); break;
            case '{': addToken(tokenizer, LEFT_BRACE); break;
            case '}': addToken(tokenizer, RIGHT_BRACE); break;
            case ',': addToken(tokenizer, COMMA); break;
            case '.': addToken(tokenizer, DOT); break;
            case '-': addToken(tokenizer, MINUS); break;
            case '+': addToken(tokenizer, PLUS); break;
            case ';': addToken(tokenizer, SEMICOLON); break;
            case '?': addToken(tokenizer, QMARK); break;
            case ':': addToken(tokenizer, COLON); break;
            case '*': addToken(tokenizer, STAR); break;
            case '!': addToken(tokenizer, match(tokenizer, '=') ? BANG_EQUAL : BANG); break;
            case '=': addToken(tokenizer, match(tokenizer, '=') ? EQUAL_EQUAL : EQUAL); break;
            case '<': addToken(tokenizer, match(tokenizer, '=') ? LESS_EQUAL : LESS); break;
            case '>': addToken(tokenizer, match(tokenizer, '=') ? GREATER_EQUAL: GREATER); break;
            case '/': {
                if (match(tokenizer, '/'))
                {
                    // this is a comment lexeme
                    for (; peek(tokenizer) != '\n' && tokenizer->current_char < tokenizer->source_len; advance(tokenizer));
                } else if (match(tokenizer, '*')){
                    // this is a multiline comment lexeme
                    for (; peek(tokenizer) != '*' && peek_next(tokenizer) != '/' && 
                            tokenizer->current_char < tokenizer->source_len; advance(tokenizer));
                    advance(tokenizer);
                    advance(tokenizer);
                } else addToken(tokenizer, SLASH);
            }; break;
            
            case ' ':
            case '\r':
            case '\t':
                break;

            case '\n': tokenizer->current_line++; break;

            case '"': addString(tokenizer); break;
            case '\0': break;
            default: {
                if (isdigit(c)){
                    addNumber(tokenizer);
                } else if (isalpha(c)) {
                    addIdentifier(tokenizer);
                } else {
                    plerror(tokenizer->current_line, get_column(&tokenizer->tokens[tokenizer->list_index]), TOKEN_ERR, "Unexpected character '%c'", c);
                }
            } break;
        }
    }
    addToken(tokenizer, ENDFILE);
}

const char* token_strings[] = {   
    "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE", "COMMA", "DOT", "MINUS", "PLUS", "SEMICOLON", "SLASH", "STAR", "QMARK", "COLON",
    "BANG", "BANG_EQUAL", "EQUAL", "EQUAL_EQUAL", "GREATER", "GREATER_EQUAL", "LESS", "LESS_EQUAL", "IDENTIFIER", "STRING", "NUMBER",
    "AND", "OR", "PRINT", "IF", "ELSE", "TRUE", "FALSE", "NIL", "FOR", "WHILE", "FUN", "RETURN", "CLASS", "SUPER", "THIS", "VAR", "ENDFILE"
};

void print_tokens(Tokenizer* tokenizer){
    printf("SOURCE (chars %ld):\n%s\n", tokenizer->source_len, tokenizer->source);
    printf("\nTokens: \n");
    for (size_t i = 0; i < tokenizer->list_index; i++){
        TokenType t = tokenizer->tokens[i].type;

        printf("[Line %d] %11s: ", 
            tokenizer->tokens[i].line, 
            token_strings[t]);
        
        for (size_t j = tokenizer->tokens[i].start; t != ENDFILE && j < tokenizer->tokens[i].count; j++){
            printf("%c", tokenizer->source[j]);
        }

        t == NUMBER ? 
            printf(" | literal: %f\n", tokenizer->tokens[i].lit.number) : 
            printf(" | literal: %s\n", tokenizer->tokens[i].lit.string);
    }
}