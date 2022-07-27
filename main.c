#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "utils.h"

bool hadError = false;

void run(char* source, Env* env){
    TokenList* tokenlist = tokenize(source);
    // printTokenlist(tokenlist);

    StmtList* stmtList = parse(tokenlist);
    
    if (!hadError){
        // printf("\n");
        // for (size_t i = 0; i < stmtList->index; i++){
        //     statementPrinter(&stmtList->statements[i]);
        //     printf("\n");
        // }
        interpret(stmtList, env);
    }
    
    freeStmtList(stmtList);
    // freeTokenList(tokenlist);
}

void runFile(const char* path){
    char* source = read_source_file(path);
    Env* env = createEnv(NULL);
    run(source, env);
    freeEnv(env);
    if (hadError) exit(1);
}

void runREPL(){
    char c;
    size_t size, index;
    char* line = malloc(100);
    Env* env = createEnv(NULL);
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
    freeEnv(env);
}

int main(int argc, char** argv){
    if (argc == 2) runFile(argv[1]);
    else runREPL();
    
    return 0;
}