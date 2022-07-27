#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "utils.h"

bool hadError = false;

void run(char* source, Env* env[ENV_SIZE]){
    TokenList* tokenlist = tokenize(source);
    //printTokenlist(tokenlist);

    StmtList* stmtList = parse(tokenlist);
    
    if (!hadError){
        // printf("\n");
        // for (size_t i = 0; i < stmtList->index; i++){
        //     // statementPrinter(&stmtList->statements[i]);
        //     // printf("\n");
        // }
        interpret(stmtList, env);
    }
    // double val = 10.0;
    // char* val = "hello world";
    // define(env, "x", val);

    // void* x = get(env, &(Token){.lexeme="x"});
    // printf("TEST RES: %s\n", (char*)x);

    // LiteralExpr val = evaluate(statements);
    // if (!hadError){
    //     switch (val.type)
    //     {
    //         case NUM_T:  printf("VAL NUM: %f\n", *(double*)val.value); break;
    //         case NIL_T:  printf("VAL NIL: %d\n", val.value); break;
    //         case BOOL_T: printf("VAL BOOL: %d\n", *(int*)val.value); break;
    //         case STR_T:  printf("VAL STR: %s\n", (char*)val.value); break;
    //         default: break;
    //     }
    // }
    
    freeStmtList(stmtList);
    freeTokenList(tokenlist);
}

void runFile(const char* path){
    char* source = read_source_file(path);
    Env* env[ENV_SIZE];
    memset(env, 0, sizeof(env));
    run(source, env);
    freeEnv(env);
    if (hadError) exit(1);
}

void runREPL(){
    char c;
    size_t size, index;
    char* line = malloc(100);
    Env* env[ENV_SIZE];
    memset(env, 0, sizeof(env));
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