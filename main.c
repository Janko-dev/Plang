// #include "lexer.h"
#include "tokenizer.h"
#include "parser.h"
#include "interpreter.h"
#include "utils.h"

bool hadError = false;

void run(char* source, Env* env){

    Tokenizer* tokenizer = create_tokenizer(source);
    tokenize(tokenizer);

    Parser* parser = create_parser(tokenizer);
    if (!hadError) print_tokens(parser->tokenizer);

    parse(parser);
    print_statements(parser);

    interpret(parser->stmt_list, env, source);

    free_parser(parser);
    // free_tokenizer(tokenizer);
    // free(source);

    // printf("TEST\n");
    // Tokenizer* tokenizer2 = create_tokenizer(source);
    // printf("TEST2\n");
}

void runFile(const char* path){
    char* source = read_source_file(path);
    Env* env = create_env(NULL);
    run(source, env);
    free_env(env);
    if (hadError) exit(1);
}

void runREPL(){
    char c;
    size_t size, index;
    char* line = malloc(100);
    Env* env = create_env(NULL);
    printf("Welcome to the REPL (Read, Evaluate, Print, Loop) environment\n");
    while (true){
        size = 100;
        index = 0;
        printf("> ");
        while ((c = fgetc(stdin)) != EOF){
            if (c == '\n') break;
            if (index == size) {
                size *= 2;
                line = realloc(line, size);
            }
            line[index++] = c;
        }
        line[index] = '\0';
        
        run(line, env);
        hadError = false;
    }
    free_env(env);
}

int main(int argc, char** argv){
    if (argc == 2) runFile(argv[1]);
    else runREPL();
    return 0;
}