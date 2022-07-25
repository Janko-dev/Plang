#include "lexer.h"
#include "parser.h"

void run(char* source){
    TokenList* tokenlist = tokenize(source);
    printTokenlist(tokenlist);

    Expr* expr = parse(tokenlist);
    
    printf("\n");
    AstPrinter(expr);
    printf("\n");

    LiteralExpr val = evaluate(expr);
    switch (val.type)
    {
    case NUM_T: printf("VAL NUM: %f\n", *(double*)val.value); break;
    case NIL_T: printf("VAL NIL: %d\n", val.value); break;
    case BOOL_T: printf("VAL BOOL: %d\n", val.value); break;
    case STR_T: printf("VAL STR: %s\n", (char*)val.value); break;
    default:
        break;
    }
    
    freeAst(expr);
    freeTokenList(tokenlist);
}

void runFile(const char* path){
    char* source = read_source_file(path);
    run(source);
}

void runREPL(){
    char c;
    size_t size, index;
    char* line = malloc(100);
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
        
        run(line);
    }
}

int main(int argc, char** argv){
    if (argc == 2) runFile(argv[1]);
    else runREPL();
    
    return 0;
}